#include "vodlog.h"
#include "util.h"
VodLog::VodLog(){
    pthread_mutex_init(&log_mutex,NULL);
}

VodLog::~VodLog(){
    _ofs.close();
    pthread_mutex_destroy(&log_mutex);
}

int
VodLog::initLogSystem(string logFile){
    _ofs.open(logFile.c_str(),fstream::trunc);
    if(_ofs.is_open() == false){
        cout<<"open log file "<<logFile<<" error"<<endl;
        exit(1);
    }
    return 0;
}

int
VodLog::writeLog(unsigned int logType,string info){
    pthread_mutex_lock(&log_mutex);
    switch(logType){
        case SYSTEM:
            _ofs<<"SYSTEM> ";
            break;
        case SYSTEMSTATE:
            _ofs<<"SYSTEMSTATE> ";
            break;
        case USERSTATE:
            _ofs<<"USERSTATE> ";
            break;
        case SERVE:
            _ofs<<"SERVE> ";
            break;
        case RESOURCE:
            _ofs<<"RESOURCE> ";
            break;
        default:
            _ofs<<"UNEXPECTED> ";
            break;
    }
    _ofs<<info<<endl;
    pthread_mutex_unlock(&log_mutex);
//    cout<<info<<endl;
    return 0;
}
int
VodLog::writeSystemLog(string info){
    return writeLog(SYSTEM,info);
}

int
VodLog::writeSystemStateLog(string info){
    return writeLog(SYSTEMSTATE,info);
}

int
VodLog::writeUserStateLog(unsigned int clientId,string info){
    info = "client ["+ numToString(clientId)+"] "+ info;
    return writeLog(USERSTATE,info);
}

int
VodLog::writeServeLog(unsigned int serverId,unsigned int clientId,string info){
    info = "server ["+numToString(serverId)+"] <------> client ["+numToString(clientId)+"]" + info;
    return writeLog(SERVE,info);
}

int
VodLog::writeServeLog(string info){
    return writeLog(SERVE,info);
}

int
VodLog::writeResourceLog(string info){
    return writeLog(RESOURCE,info);
}


