
#include"lfruManager.h"
#include "../lib/vodlog.h"
#include "server.h"
#include<sstream>
/***************************************************************************
 **this part for LFRU Manager
 ***************************************************************************/
LFRUManager::LFRUManager(LBServer* _lb, ConfigType *serverConfig) : ResourceManager(_lb, serverConfig) {
    lb = _lb;
    subServerNum = serverConfig->subServerNum;
    resourceNum = serverConfig->resourceNumber;
    capacity = serverConfig->maxCapacity;
    period = serverConfig->period;
    std::ostringstream logStr;
    logStr << "set LFRU period = " << period ;
    serverLog.writeResourceLog(logStr.str());
    initResource();
    //set t0
    gettimeofday(&t0, NULL);
}

int
LFRUManager::countReset() {
    string logStr;
    while (true) {
        //reset the counts of each file when a period passed
        sleep(period);
        logStr = "LFRU reset the counts of all file in all server";
        serverLog.writeResourceLog(logStr);
        for (unsigned int i = 0; i < LFRUFileList.size(); i++) {
            LFRUFileList[i]->count = 1;
        }
        gettimeofday(&t0, NULL);
    }
    return 0;

}

int
LFRUManager::getSubServerList(unsigned int fileId, vector<unsigned int> &containList) {
    containList.clear();
    for (unsigned int i = 0; i < LFRUFileList.size(); i++) {
        if (LFRUFileList[i]->fileId == fileId) {
            containList.push_back(LFRUFileList[i]->serverId);
        }
    }
    return 0;
}

int
LFRUManager::readFile(unsigned int fileId, unsigned int serverId) {
    string logStr;
    for (unsigned int i = 0; i < LFRUFileList.size(); i++) {
        if (LFRUFileList[i]->fileId == fileId && LFRUFileList[i]->serverId == serverId) {
            LFRUFileList[i]->count++;
            gettimeofday(&(LFRUFileList[i]->vtime), NULL);
            logStr = " the count of file " + numToString(fileId) + " in server[" + numToString(serverId) + "] is "
                    + numToString(LFRUFileList[i]->count);
            break;
        }
    }
    serverLog.writeResourceLog(logStr);
    return 0;
}

int
LFRUManager::initResource() {
    string logStr = "initialize the LFRUManager::LFRUFileList ";
    serverLog.writeResourceLog(logStr);
    unsigned int targetServer;
    for (unsigned int i = 0; i < resourceNum; i++) {
        targetServer = i % subServerNum;
        LFRUFileInfo *fileInfo = new LFRUFileInfo(i, targetServer);

        if (fileInfo != NULL)
            LFRUFileList.push_back(fileInfo);
    }
    return 0;
}

int
LFRUManager::resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen) {
    //first get the information of fileId
    string logStr = "client [" + numToString(clientId) + "] request file " + numToString(fileId);
    serverLog.writeResourceLog(logStr);
    //get the sub servers that contain the fileId
    vector<unsigned int >containList;
    filePlayLen = fileList[fileId]->filePlayLen;
    logStr = "file " + numToString(fileId) + " can play " + numToString(filePlayLen) + " secs";
    serverLog.writeResourceLog(logStr);
    containList.clear();
    getServerList(fileId, containList);
    //output the containList
    ostringstream oslogStr;
    oslogStr << "the containList of file " << fileId << ":";
    for (unsigned int i = 0; i < containList.size(); i++) {
        oslogStr << containList[i] << " ";
    }
    logStr = oslogStr.str();
    serverLog.writeResourceLog(logStr);
    //then find the best sub server to serve it
    int targetServer;
    targetServer = lb->getMinLoadServer(containList);
    if (targetServer < 0) {
        logStr = "can't find a sub server to serve client " + numToString(clientId);
        serverLog.writeResourceLog(logStr);
        return targetServer;
    }
    logStr = "the LFRUManager chose server[" + numToString(targetServer) + "] to server it";
    serverLog.writeResourceLog(logStr);
    //and then change the attribute of fileId and target server
    readFile(fileId, targetServer);
    lb->increaseLoad(targetServer);
    return targetServer;
}

int
LFRUManager::duplicateMethod(unsigned int serverId) {
    unsigned int needSpreadFile;
    double minWeight, curWeight;
    minWeight = 1000000.0;
    string logStr = "server[" + numToString(serverId) + "] is overload";
    serverLog.writeSystemLog(logStr);
    //get the file need to spread
    struct timeval callTime;
    gettimeofday(&callTime, NULL);
    ostringstream logStream;
    for (unsigned int i = 0; i < LFRUFileList.size(); i++) {
        if (LFRUFileList[i]->serverId != serverId)
            continue;
        logStream<<"for server "<<serverId<<",file "<<LFRUFileList[i]->fileId<<endl;
        logStream<<"\tcallTime:"<<callTime.tv_sec<<":"<<callTime.tv_usec<<" ";
        logStream<<"t0:"<<t0.tv_sec<<":"<<t0.tv_usec<<" ";
        logStream<<"vtime:"<<LFRUFileList[i]->vtime.tv_sec<<":"<<
                LFRUFileList[i]->vtime.tv_usec<<" ";
        logStream<<"count:"<<LFRUFileList[i]->count<<" ";
        double fk = getTimeSlips(&callTime, &t0) / (double(LFRUFileList[i]->count * 1000000.0));
        double rk = getTimeSlips(&callTime, &(LFRUFileList[i]->vtime)) / 1000000.0;
        curWeight = period - getTimeSlips(&callTime, &t0) / 1000000.0;
        curWeight = curWeight * rk / (double) period;
        curWeight += getTimeSlips(&callTime, &t0) * fk / (1000000.0 * period);
        logStream<<"Fk:"<<fk<<",rk:"<<rk<<",weight : "<<curWeight<<" ";
        //logStr = "in server[" + numToString(serverId) + "],the count of file " +
        //numToString(DWFileList[i]->fileId) + " is " + numToString(curCount);
        //serverLog.writeResourceLog(logStr);
        if (minWeight > curWeight) {
            minWeight = curWeight;
            needSpreadFile = LFRUFileList[i]->fileId;
        }
        logStream<<" minWeight = "<<minWeight<<",need spread file ="<<needSpreadFile<<endl;
    }
    serverLog.writeResourceLog(logStream.str());
    logStr = "server[" + numToString(serverId) + "] chose  file[" + numToString(needSpreadFile) + "] to spread";
    serverLog.writeResourceLog(logStr);
    //choose the machine that have the minLoad and don't have the file
    vector<unsigned int > containList;
    getServerList(needSpreadFile, containList);
    vector<unsigned int> noList;
    noList.clear();
    for (unsigned int i = 0; i < subServerNum; i++) {
        bool isContain = false;
        for (unsigned int j = 0; j < containList.size(); j++) {
            if (containList[j] == i) {
                isContain = true;
                break;
            }
        }
        if (isContain == false) {
            noList.push_back(i);
        }
    }
    int targetServer = lb->getMinLoadServer(noList);
    if (targetServer < 0) {
        logStr = "no server to duplicate";
        serverLog.writeResourceLog(logStr);
        return -1;
    }
    logStr = "the spread target server is server[" + numToString(targetServer) + "]";
    serverLog.writeResourceLog(logStr);
    //should have some transport time
    lb->addFileToSubServer(needSpreadFile, targetServer);
    addNewServer(needSpreadFile, targetServer);
    LFRUFileInfo *fileInfo = new LFRUFileInfo(needSpreadFile, targetServer);
    if (fileInfo != NULL)
        LFRUFileList.push_back(fileInfo);
    lb->printFileList();
    return 0;
}

int
LFRUManager::deleteMethod(unsigned int serverId) {
    return 0;
}

