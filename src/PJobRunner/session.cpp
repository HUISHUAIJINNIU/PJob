#include "session.h"
#include <iostream>
#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QTcpSocket>
#include "PJobFileError.h"
#include "dataconnectionthread.h"
#include <QtCore/QDateTime>
#include <QtServiceBase>
#include <QHostAddress>
#include "pjobrunnerservice.h"
#include <QtNetwork/QHostInfo>

Session::Session(QTcpSocket* socket) : m_pjob_file(0), m_script_engine(0), m_wants_shutdown(false), m_socket(socket), m_data_to_send(0), m_data_receive_connection(0), m_data_push_connection(0)
{
    QDir temp = QDir::temp();
    QString random = QDateTime::currentDateTime().toString("yyyyMMdd_hhmm_ss_zzz");;
    while(temp.exists(random)) random.append("_");
    temp.mkdir(random);
    temp.cd(random);
    m_temp_dir = temp.absolutePath();
    if(m_socket) QtServiceBase::instance()->logMessage(QString("Opening new session for peer %1 over port %2 with temporary directory %3.").arg(socket->peerAddress().toString()).arg(socket->localPort()).arg(m_temp_dir));
}

Session::~Session(){
    delete m_script_engine;
    delete m_pjob_file;
    delete m_data_receive_connection;
}

Session& Session::global_instance(){
    static Session c;
    return c;
}

ScriptEngine& Session::script_engine(){
    if(m_script_engine == 0) m_script_engine = new ScriptEngine(this);
    return *m_script_engine;
}

bool Session::wants_shutdown(){
    return m_wants_shutdown;
}

QString Session::hello(){
    return QString("This is %1 (%3) version %2 running on %4 (%5).\nNice to meet you.").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()).arg(PJobRunnerService::instance()->serviceDescription()).arg(QHostInfo::localHostName()).arg(platform());
}

QString Session::platform(){
#ifdef Q_OS_WIN32
    return "Windows";
#endif
#ifdef Q_OS_MAC
    return "MacOSX";
#endif
#ifdef Q_OS_UNIX
    return "Unix";
#endif
}

void Session::open_local_pjob_file(QString filename){
    if(m_pjob_file) delete m_pjob_file;
    try{
        m_pjob_file = new PJobFile(filename);
    }catch(PJobFileError &e){
        m_pjob_file = 0;
        output(e.msg());
        return;
    }
    output(QString("File %1 opened!").arg(filename));
    QtServiceBase::instance()->logMessage(QString("Opened local pjob file %1 for peer %2.").arg(filename).arg(m_socket->peerAddress().toString()), QtServiceBase::Information);
    m_application = m_pjob_file->defaultApplication();
    foreach(PJobFileParameterDefinition d, m_pjob_file->parameterDefinitions()){
        m_parameters[d.name()] = d.defaultValue();
    }
}

quint32 Session::prepare_push_connection(){
    if(m_data_receive_connection) delete m_data_receive_connection;
    m_received_data.clear();
    m_data_receive_connection = new DataReceiveConnection(m_received_data,this);
    quint32 port = m_data_receive_connection->open_data_port();
    m_data_receive_connection->start();
    QtServiceBase::instance()->logMessage(QString("Prepared push connection on port %1 for peer %2.").arg(port).arg(m_socket->peerAddress().toString()), QtServiceBase::Information);
    return port;
}

quint32 Session::prepare_pull_connection_for_results(){
    if(!m_pjob_file){
        output("Can't prepare pull connection! No PJob file openend!");
        return 0;
    }
    if(m_data_push_connection) delete m_data_push_connection;
    if(m_data_to_send) delete m_data_to_send;
    m_data_to_send = m_pjob_file->get_result_files_raw();
    m_data_push_connection = new DataPushConnection(*m_data_to_send,this);
    quint32 port = m_data_push_connection->open_data_port();
    m_data_push_connection->start();
    QtServiceBase::instance()->logMessage(QString("Prepared pull connection on port %1 for peer %2.").arg(port).arg(m_socket->peerAddress().toString()), QtServiceBase::Information);
    return port;
}

void Session::open_pjob_from_received_data(){
    if(!m_data_receive_connection || !m_data_receive_connection->data_received()){
        output("No data received! Can't open pjob file!");
        return;
    }
    try{
        m_pjob_file = new PJobFile(m_received_data);
    }catch(PJobFileError &e){
        m_pjob_file = 0;
        output(e.msg());
    }
    m_application = m_pjob_file->defaultApplication();
    foreach(PJobFileParameterDefinition d, m_pjob_file->parameterDefinitions()){
        m_parameters[d.name()] = d.defaultValue();
    }
    output("pjob file opened from received data.");
    QtServiceBase::instance()->logMessage(QString("Opened received pjob for peer %1.").arg(m_socket->peerAddress().toString()));
}

void Session::set_temp_dir(QString path){
    m_temp_dir = path;
}

void Session::set_parameter(QString name, double value){
    m_parameters[name] = value;
}

void Session::set_application(QString app_name){
    m_application = app_name;
}

bool removeDir(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
}

void Session::run_job(){
    if(m_pjob_file == 0){
        output("Can't run job! No pjob file opened!");
        return;
    }
    PJobFileApplication app = m_pjob_file->applicationByName(m_application);
#ifdef Q_OS_WIN32
    if(app.platform != PJobFileApplication::Win32  && app.platform != PJobFileApplication::Win64){
        output(QString("Can't run %1. No Application build for Windows!").arg(app.name));
        return;
    }
#endif
#ifdef Q_OS_MAC
    if(app.platform != PJobFileApplication::MacOSX){
        output(QString("Can't run %1. No Application build for MacOSX!").arg(app.name));
        return;
    }
#endif
#ifdef Q_OS_UNIX
    if(app.platform != PJobFileApplication::Linux){
        output(QString("Can't run %1. No Application build for Unix-like Systems!").arg(app.name));
        return;
    }
#endif

    QtServiceBase::instance()->logMessage(QString("Running job for peer %1 in temp dir %2.").arg(m_socket->peerAddress().toString()).arg(m_temp_dir));

    QString temp_dir = QFileInfo(m_temp_dir).absoluteFilePath();

    output(QString("Clearing temporary directory %1").arg(temp_dir));
    removeDir(temp_dir);

    QString resources_directory = QString("%1/Resources/").arg(temp_dir);
    m_pjob_file->export_application(app.name, temp_dir);
    m_pjob_file->export_resources(temp_dir);
    QString executable = temp_dir + "/" + app.name + "/" + app.executable;
    QFile::setPermissions(executable, QFile::ExeUser);
    QProcess process;
    process.setWorkingDirectory(resources_directory);
    output(QString("Starting process: %1").arg(executable));
    output(QString("With arguments:"));
    foreach(QString arg, create_commandline_arguments_for_app(app)){
        output(arg);
    }

    process.start(executable, create_commandline_arguments_for_app(app));
    process.waitForStarted(-1);
    output("-------------------");
    output("Process std output:");
    output("-------------------");
    do{
        output(process.readAllStandardOutput());
    }while(!process.waitForFinished(100));

    switch(process.exitStatus()){
    case QProcess::NormalExit:
        output("Process exited normally.");
        m_pjob_file->import_run_directory(resources_directory);
        output("Created files imported into run directory.");
        break;
    case QProcess::CrashExit:
        output("Process crashed!");
        switch(process.error()){
        case QProcess::FailedToStart:
            output("Process failed to start!");
            break;
        case QProcess::Crashed:
            output("Process crashed some time after starting successfully!");
            break;
        default:
            break;
        }
        output("---------------------");
        output("Process error output:");
        output("---------------------");
        output(process.readAllStandardError());
    }
    m_pjob_file->save();
}

QStringList Session::create_commandline_arguments_for_app(const PJobFileApplication& app){
    QStringList params;
    QMapIterator<QString, double> it(m_parameters);
    while(it.hasNext()){
        it.next();
        QString arguments = app.parameter_pattern;
        arguments.replace(QString("<param>"), it.key());
        arguments.replace(QString("<value>"), QString::number(it.value()));
        params.append(arguments);
    }
    QStringList result = app.arguments.split(" ");
    int params_index = result.indexOf("<parameters>");
    result.removeAt(params_index);
    while(!params.empty())
        result.insert(params_index, params.takeLast());
    return result;
}



void Session::exit(){
    m_wants_shutdown = true;
}

QStringList Session::run_directories(){
    return m_pjob_file->runDirectoryEntries();
}

void Session::output(const QString& msg){
    if(m_socket){
        m_socket->write((msg + "\n").toAscii());
        m_socket->flush();
    }
    else std::cout << msg.toStdString() << std::endl;
}

const QByteArray& Session::received_data(){
    return m_received_data;
}

void Session::write_received_data_to_file(QString path){
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)){
        output(QString("Could not open %1 for writing!"));
        return;
    }
    file.write(m_received_data);
    file.close();
    QtServiceBase::instance()->logMessage(QString("Wrote received data to file %1 for peer %2.").arg(path).arg(m_socket->peerAddress().toString()), QtServiceBase::Information);
}
