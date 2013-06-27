#include "config.h"
#include "../lib/vodlog.h"
#include "server.h"

VodLog serverLog;

int main() {
    ConfigType configInfo;
    read_vodspread_config("config/simulator.ini",configInfo);
    LBServer *serverMaster = new LBServer(&configInfo);
    std::ostringstream logStr;
    logStr <<configInfo.logFile<< "server" << "_" << configInfo.spreadAlgorithm;
    if (configInfo.spreadAlgorithm.compare("LRFU") == 0) {
        logStr << "_" << configInfo.lambda << "_" << configInfo.loadThresh;
    } else if (configInfo.spreadAlgorithm.compare("DR") == 0) {
        logStr << "_" << configInfo.DRBeta << "_" << configInfo.period;
    } else if (configInfo.spreadAlgorithm.compare("LFRU") == 0) {
        logStr << "_" << configInfo.period << "_" << configInfo.loadThresh;
    } else if (configInfo.spreadAlgorithm.compare("DW") == 0) {
        logStr << "_" << configInfo.period << "_" << configInfo.loadThresh;
    }
    logStr << "_";
    time_t now = time(0);
    tm* localtm = localtime(&now);
    logStr << localtm->tm_mon + 1 << localtm->tm_mday << localtm->tm_hour << localtm->tm_min << ".log";
    //initialize the log file 
    serverLog.initLogSystem(logStr.str());
    srandom(time(NULL));
    string startLog = "start server side,create log file " + logStr.str();
    serverLog.writeSystemLog(startLog);
    serverMaster->startServerSide();
    
    return 0;
}
