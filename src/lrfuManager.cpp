
#include"lrfuManager.h"
#include "../lib/vodlog.h"
#include "server.h"
#include<sstream>
/*****************************************************************************
 *this part for LRFU Manager
 **************************************************************************/
LRFUManager::LRFUManager(LBServer* _lb, ConfigType *serverConfig) :
ResourceManager(_lb, serverConfig) {
    std::ostringstream logStr;
    lambda = serverConfig->lambda;
    logStr << "set LRFU lambda = " << lambda << endl;
    serverLog.writeResourceLog(logStr.str());
    initResource();
}

int
LRFUManager::getSubServerList(unsigned int fileId, vector<unsigned int> &containList) {
    containList.clear();
    for (unsigned int i = 0; i < LRFUFileList.size(); i++) {
        if (LRFUFileList[i]->fileId == fileId) {
            containList.push_back(LRFUFileList[i]->serverId);
        }
    }
    return 0;
}

int
LRFUManager::readFile(unsigned int fileId, unsigned int serverId) {
    int curTime,lastTime;
    for (unsigned int i = 0; i < LRFUFileList.size(); i++) {
        if (LRFUFileList[i]->fileId == fileId && LRFUFileList[i]->serverId == serverId) {
            curTime =  lb->getTimeSlot(serverId);
            lastTime = LRFUFileList[i]->vtime;
            LRFUFileList[i]->crfValue = LRFUFileList[i]->crfValue*
                    pow(0.5,(curTime-lastTime)*lambda)+1.0;
            LRFUFileList[i]->vtime = curTime;
            break;
        }
    }
    lb->increaseTimeSlot(serverId);
    return 0;
}

int
LRFUManager::initResource() {
    string logStr = "initialize the LRFUManager::LRFUFileList ";
    serverLog.writeResourceLog(logStr);
    unsigned int targetServer;
    for (unsigned int i = 0; i < resourceNum; i++) {
        targetServer = i % subServerNum;
        LRFUFileInfo *fileInfo = new LRFUFileInfo(i, targetServer);
        if (fileInfo != NULL)
            LRFUFileList.push_back(fileInfo);
    }
    return 0;
}

int
LRFUManager::duplicateMethod(unsigned int serverId) {
    ostringstream logStr;
    unsigned int needSpreadFile;
    double maxCRF;
    maxCRF = 0.0;
    logStr << "server[ " << serverId << " ] is overload";
    serverLog.writeSystemLog(logStr.str());
    logStr.str("");
    //get the file need to spread
    for (unsigned int i = 0; i < LRFUFileList.size(); i++) {
        if (LRFUFileList[i]->serverId != serverId)
            continue;
        logStr<<"in sub server " << serverId << ",file " << LRFUFileList[i]->fileId;
        LRFUFileList[i]->crfValue = LRFUFileList[i]->crfValue *
                pow(0.5, (lb->getTimeSlot(serverId) - LRFUFileList[i]->vtime) * lambda);
        logStr <<",timeSlot = " << lb->getTimeSlot(serverId) << ",vtime = " <<
                LRFUFileList[i]->vtime << ",CRF = " << LRFUFileList[i]->crfValue;
        
        if (maxCRF < LRFUFileList[i]->crfValue) {
            maxCRF = LRFUFileList[i]->crfValue;
            needSpreadFile = LRFUFileList[i]->fileId;

        }
        logStr << ",maxCRF = " << maxCRF << ",need spread file = " << needSpreadFile<<endl;
    }
    serverLog.writeResourceLog(logStr.str());
    logStr.str("");
    logStr << "server[ " << serverId << "] chose file [" << needSpreadFile << "] to spread";
    serverLog.writeResourceLog(logStr.str());
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
        logStr << "no server to duplicate";
        serverLog.writeResourceLog(logStr.str());
        return -1;
    }
    logStr << "the spread target server is server[" << targetServer << "]";
    serverLog.writeResourceLog(logStr.str());
    //should have some transport time
    lb->addFileToSubServer(needSpreadFile, targetServer);
    addNewServer(needSpreadFile, targetServer);
    LRFUFileInfo *fileInfo = new LRFUFileInfo(needSpreadFile, targetServer);
    if (fileInfo != NULL){
        fileInfo->vtime = lb->getTimeSlot(serverId);
        LRFUFileList.push_back(fileInfo);
    }
    lb->increaseTimeSlot(serverId);
    lb->printFileList();
    return 0;

}

int
LRFUManager::resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen) {
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
        return targetServer;
    }
    logStr = "the LRFUManager chose server[" + numToString(targetServer) + "] to server it";
    serverLog.writeResourceLog(logStr);
    //and then change the attribute of fileId and target server
    readFile(fileId, targetServer);
    lb->increaseLoad(targetServer);
    return targetServer;
}

int
LRFUManager::deleteMethod(unsigned int serverId) {
    return 0;
}
