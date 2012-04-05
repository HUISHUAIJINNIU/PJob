#include <QtCore/QCoreApplication>
#include "session.h"
#include <QtCore/QFileInfo>
#include <QtCore/QFile>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Session::instance().set_temp_dir(QFileInfo(argv[0]).absolutePath() + "/PJobRunner_temp");

    if(argc == 2){
        QFileInfo file_info(argv[1]);
        if(file_info.exists() && file_info.isFile()){
            std::cout << QString("Interpreting file %1...").arg(file_info.absolutePath()).toStdString() << std::endl;
            QFile file(file_info.absoluteFilePath());
            file.open(QFile::ReadOnly);
            Session::instance().script_engine().evaluate(file.readAll());

            return 0;
        }
    }

    if(argc > 1) std::cout << "Interpreting command line arguments..." << std::endl;
    for(int i=1;i<argc;i++){
        Session::instance().output(argv[i]);
        Session::instance().script_engine().evaluate(argv[i]);
    }

    char buffer[1024];

    std::cout << "Entering interactive mode..." << std::endl;
    while(!Session::instance().wants_shutdown()){

        std::cin.getline(buffer,1024);
        Session::instance().script_engine().evaluate(buffer);
    }
    std::cout << "Exiting..." << std::endl;

    return 0;
}
