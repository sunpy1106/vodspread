#ifndef VODLOG_H
#define VODLOG_H
#include<iostream>
#include<fstream>
#include<string>
#include<stdlib.h>
using namespace std;

typedef enum LogType {
    SYSTEM,
    SYSTEMSTATE,
    USERSTATE,
    SERVE,
    RESOURCE
}LogType;

class VodLog{
    public:
        VodLog();
        ~VodLog();
        int initLogSystem(string logFile);
    public:
        int writeLog(unsigned int logtype,string info);
        int writeSystemLog(string info);
        int writeSystemStateLog(string info);
        int writeUserStateLog(unsigned int clientId,string info);
        int writeServeLog(unsigned int serverId,unsigned int clientId,string info);
        int writeServeLog(string info);
        int writeResourceLog(string info);
    private:
        ofstream _ofs;
        pthread_mutex_t log_mutex;
};
extern VodLog serverLog;
extern VodLog clientLog;
#endif
