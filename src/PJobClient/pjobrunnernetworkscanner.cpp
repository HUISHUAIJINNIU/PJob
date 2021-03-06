#include "pjobrunnernetworkscanner.h"
#include <QtNetwork>
#include "pjobrunnersessionwrapper.h"
#include <iostream>

QList<PJobRunnerSessionWrapper*> PJobRunnerNetworkScanner::s_found_sessions;
bool PJobRunnerNetworkScanner::s_blocking_scan = false;

PJobRunnerNetworkScanner::PJobRunnerNetworkScanner()
    : m_port(23023), m_want_stop(false)
{
    qRegisterMetaType<QHostAddress>("QHostAddress");
}

void PJobRunnerNetworkScanner::run(){
    scan(m_interface);
}

void PJobRunnerNetworkScanner::stop_scan(){
    m_want_stop = true;
}

QList<PJobRunnerSessionWrapper*> PJobRunnerNetworkScanner::do_blocking_scan(const QNetworkInterface &interface){
    s_found_sessions.clear();
    s_blocking_scan = true;
    PJobRunnerNetworkScanner scanner;
    scanner.set_network_interface(interface);
    scanner.scan(interface);
    s_blocking_scan = false;
    return s_found_sessions;
}
void PJobRunnerNetworkScanner::set_network_interface(const QNetworkInterface& interface){
    m_interface=interface;
}

void PJobRunnerNetworkScanner::scan(const QNetworkInterface& interface){
    foreach(const QNetworkAddressEntry& address_entry, interface.addressEntries()){
        if(m_want_stop) break;
        quint32 netmask = address_entry.netmask().toIPv4Address();
        quint32 local_ip = address_entry.ip().toIPv4Address();
        quint32 address_to_try = (local_ip & netmask) + 1;
        //quint32 inv_netmask = ~netmask;

        if(interface.humanReadableName() == "lo"){
            std::cout << "Skipping interface \"lo\"." << std::endl;
            continue;
        }

        if(!interface.isValid()){
            std::cout << "Skipping interface \"" << interface.humanReadableName().toStdString() << "\"." << std::endl;
            continue;
        }

        if(address_entry.ip().protocol() != QAbstractSocket::IPv4Protocol){
            std::cout << "Skipping non-IPv4 interface." << std::endl;
            continue;
        }

        if(address_entry.ip() == QHostAddress::LocalHost){
            std::cout << "Skipping loopback interface." << std::endl;
            continue;
        }

        if(address_entry.ip().isNull()){
            continue;
        }

        std::cout << "Probing interface " << interface.humanReadableName().toStdString() << std::endl;
        std::cout << "Local address is " << interface.hardwareAddress().toStdString() << std::endl;
        std::cout << "Netmask: " << address_entry.netmask().toString().toStdString() << std::endl;
        std::cout << "Searching local network..." << std::endl;
        while((address_to_try & netmask) == (local_ip & netmask)){
            if(m_want_stop) break;
            std::cout << "\r" << QHostAddress(address_to_try).toString().toStdString();
            std::cout.flush();
            emit probing_host(QHostAddress(address_to_try));
            PJobRunnerSessionWrapper* session = new PJobRunnerSessionWrapper(QHostAddress(address_to_try), 50);
            if(session->is_valid()) found(session);
            else delete session;
            address_to_try++;
        }
    }
    emit finished_scanning();
    m_want_stop = false;
}

void PJobRunnerNetworkScanner::found(PJobRunnerSessionWrapper* session){
    emit found_pjob_runner(session);
    if(s_blocking_scan){
        s_found_sessions.append(session);
        std::cout << "Found PJobRunner on " << session->peer().toString().toStdString() << "!" << std::endl;
    }
}
