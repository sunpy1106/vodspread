#include "../include/global.h"
#include "client.h"
#include "../lib/vodlog.h"
#include <sstream>
VodLog clientLog;

int main() {
    ConfigType configInfo;
    read_vodspread_config("config/simulator.ini",configInfo);
    ClientManager *clientMaster = new ClientManager(&configInfo);
    //get the file name of log file 
    std::ostringstream logStr;
    logStr <<configInfo.logFile<< "client" << "_" << configInfo.spreadAlgorithm;
    if (configInfo.spreadAlgorithm.compare("LRFU") == 0) {
        logStr << "_" << configInfo.lambda << "_" << configInfo.loadThresh;
    } else if (configInfo.spreadAlgorithm.compare("DR") == 0) {
        logStr << "_" << configInfo.DRBeta << "_" << configInfo.period;
    } else if (configInfo.spreadAlgorithm.compare("LFRU") == 0) {
        logStr << "_" << configInfo.period << "_" << configInfo.loadThresh;
    } else if (configInfo.spreadAlgorithm.compare("DW") == 0) {
        logStr << "_" << configInfo.period << "_" << configInfo.loadThresh;
    }
    logStr<<"_";
    time_t now = time(0);
    tm* localtm = localtime(&now);
    logStr << localtm->tm_mon + 1 << localtm->tm_mday << localtm->tm_hour << localtm->tm_min << ".log";
    //initialize the log file 
    clientLog.initLogSystem(logStr.str());
    srandom(time(NULL));
    string startLog = "start client side ,create log file " + logStr.str();
    clientLog.writeSystemLog(startLog);
    clientMaster->startClientSide();
   
    return 0;
}
