
#include"dwManager.h"
#include "../lib/vodlog.h"
#include "server.h"
#include<sstream>

//this part for DWManager

DWManager::DWManager(LBServer *_lb, ConfigType *serverConfig) : ResourceManager(_lb, serverConfig) {
    string logStr = "set DWPeriod = " + numToString(period);
    serverLog.writeResourceLog(logStr);
    initResource();
}

//reset the count ,should run in a single thread

int
DWManager::countReset() {
    string logStr;
    while (true) {
        //reset the counts of each file when a period passed
        sleep(period);
        logStr = "DW reset the counts of all file in all server";
        serverLog.writeResourceLog(logStr);
        for (unsigned int i = 0; i < DWFileList.size(); i++) {
            DWFileList[i]->c0 = DWFileList[i]->c1;
            DWFileList[i]->c1 = 0;
        }
    }
    return 0;
}

int
DWManager::getSubServerList(unsigned int fileId, vector<unsigned int> &containList) {
    containList.clear();
    for (unsigned int i = 0; i < DWFileList.size(); i++) {
        if (DWFileList[i]->fileId == fileId) {
            containList.push_back(DWFileList[i]->serverId);
        }
    }
    return 0;
}

int
DWManager::initResource() {
    string logStr = "init the DWManager::DWFileList ,FileInfo::serverList and SubServer::fileList";
    serverLog.writeResourceLog(logStr);
    unsigned int targetServer;
    for (unsigned int i = 0; i < resourceNum; i++) {
        targetServer = i % subServerNum;
        DWFileInfo *fileInfo = new DWFileInfo(i, targetServer);
        if (fileInfo != NULL)
            DWFileList.push_back(fileInfo);
    }
    return 0;
}

int
DWManager::increaseDWCount(unsigned int fileId, unsigned int serverId) {
    string logStr;
    for (unsigned int i = 0; i < DWFileList.size(); i++) {
        if (DWFileList[i]->fileId == fileId && DWFileList[i]->serverId == serverId) {
            DWFileList[i]->c1++;
            logStr = " the count of file " + numToString(fileId) + " in server[" + numToString(serverId) + "] is "
                    + numToString(DWFileList[i]->c1);
            break;
        }
    }
    serverLog.writeResourceLog(logStr);
    return 0;
}

int
DWManager::getTotalCount(unsigned int fileId) {
    unsigned int counts;
    counts = DWFileList[fileId]->c1 + DWFileList[fileId]->c0;
    return counts;
}
//return the serverId
int
DWManager::resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen) {
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
    //choose one by random
    int targetServer;
    targetServer = lb->getMinLoadServer(containList);
    if (targetServer < 0) {
        return targetServer;
    }
    logStr = "the DWManager chose server[" + numToString(targetServer) + "] to server it";
    serverLog.writeResourceLog(logStr);
    //change the count of fileId
    increaseDWCount(fileId, targetServer);
    //change the load of the chose sub server
    lb->increaseLoad(targetServer);
    //return the id of the chose sub server
    return targetServer;
}


int
DWManager::duplicateMethod(unsigned int serverId) {
    unsigned int needSpreadFile;
    unsigned int maxCount, curCount;
    maxCount = 0;
    string logStr = "server[" + numToString(serverId) + "] is overload";
    serverLog.writeSystemLog(logStr);
    //get the file need to spread
    for (unsigned int i = 0; i < DWFileList.size(); i++) {
        if (DWFileList[i]->serverId != serverId)
            continue;
        curCount = getTotalCount(i);
        logStr = "in server[" + numToString(serverId) + "],the count of file " + numToString(DWFileList[i]->fileId) + " is " + numToString(curCount);
        serverLog.writeResourceLog(logStr);
        if (maxCount < curCount) {
            maxCount = curCount;
            needSpreadFile = DWFileList[i]->fileId;
        }
    }
    logStr = "server[" + numToString(serverId) + "] chosed file " + numToString(needSpreadFile) + " to spread";
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
    DWFileInfo *fileInfo = new DWFileInfo(needSpreadFile, targetServer);
    if (fileInfo != NULL)
        DWFileList.push_back(fileInfo);
    lb->printFileList();
    //reset the count of the chosed file
    for (unsigned int i = 0; i < DWFileList.size(); i++) {
        if (DWFileList[i]->fileId == needSpreadFile) {
            DWFileList[i]->c0 = 0;
            DWFileList[i]->c1 = 0;
        }
    }
    logStr = "reset the count of file " + numToString(needSpreadFile) + " to 0 in all server";
    serverLog.writeResourceLog(logStr);
    return 0;
}

int
DWManager::deleteMethod(unsigned int serverId) {
    unsigned int needDeleteFile;
    double minCount = 100000.0;
    unsigned int curCount;
    for (unsigned int i = 0; i < DWFileList.size(); i++) {
        if (DWFileList[i]->serverId != serverId) {
            continue;
        }
        curCount = getTotalCount(i);
        if (curCount < minCount) {
            minCount = curCount;
            needDeleteFile = DWFileList[i]->fileId;
        }
    }
    string logStr = "server[" + numToString(serverId) + "] chose file " + numToString(needDeleteFile) + " to delete";
    serverLog.writeResourceLog(logStr);
    deleteServer(needDeleteFile, serverId);
    lb->deleteFileFromSubServer(needDeleteFile, serverId);

    return 0;
}

