#include"resourceManager.h"
#include "../lib/vodlog.h"
#include "server.h"
#include<sstream>

FileInfo::FileInfo(unsigned int _fileId, unsigned int _filePlayLen) {
    fileId = _fileId;
    filePlayLen = _filePlayLen;
    pthread_mutex_init(&file_mutex, NULL);
    serverList.clear();
}

FileInfo::~FileInfo() {
    pthread_mutex_destroy(&file_mutex);
}

int
FileInfo::addNewServer(unsigned int serverId) {
    pthread_mutex_lock(&file_mutex);
    serverList.push_back(serverId);
    std::sort(serverList.begin(),serverList.end());
    pthread_mutex_unlock(&file_mutex);
    return 0;
}

int
FileInfo::deleteServer(unsigned int serverId) {
    vector<unsigned int>::iterator it;
    pthread_mutex_lock(&file_mutex);
    for (it = serverList.begin(); it != serverList.end(); it++) {
        if (*it == serverId) {
            serverList.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&file_mutex);
    return 0;

}

int
FileInfo::getServerList(vector<unsigned int> &containList) {
    pthread_mutex_lock(&file_mutex);
    containList = serverList;
    pthread_mutex_unlock(&file_mutex);
    return 0;
}
void 
FileInfo::printServerList(){
    ostringstream logStr;
    logStr<<"file "<<this->fileId<<" exists in following sub servers:"<<endl;
    for(size_t i=0;i<this->serverList.size();i++){
        logStr<<this->serverList[i]<<" ";
    }
    logStr<<endl;
    serverLog.writeResourceLog(logStr.str());
}
//this part for resourceManager

ResourceManager::ResourceManager(LBServer *_lb, ConfigType *serverConfig) {
    lb = _lb;
    subServerNum = serverConfig->subServerNum;
    resourceNum = serverConfig->resourceNumber;
    capacity = serverConfig->maxCapacity;
    period = serverConfig->period;
    fileList.clear();
    string logStr = "init ResourceManager::fileList";
    serverLog.writeResourceLog(logStr);
    for (unsigned int i = 0; i < resourceNum; i++) {
        unsigned int filePlayLen = Randomi(serverConfig->minPlayLen, serverConfig->maxPlayLen);
        FileInfo *fileInfo = new FileInfo(i, filePlayLen);
        if (fileInfo != NULL)
            fileList.push_back(fileInfo);
    }
    initResource();
}

ResourceManager::~ResourceManager() {
    for (unsigned int i = 0; i < resourceNum; i++) {
        delete fileList[i];
    }
}

int
ResourceManager::getServerList(unsigned int fileId, vector<unsigned int> & serverList) {
    if (fileId >= fileList.size()) {
        cout << "unexpected fileId" << endl;
        exit(1);
    }
    fileList[fileId]->getServerList(serverList);
    return 0;
}

int
ResourceManager::addNewServer(unsigned int fileId, unsigned int serverId) {
    if (fileId < fileList.size()) {
        return fileList[fileId]->addNewServer(serverId);
    }
    return -1;
}

int
ResourceManager::deleteServer(unsigned int fileId, unsigned int serverId) {
    if (fileId < fileList.size()) {
        return fileList[fileId]->deleteServer(serverId);
    }
    return -1;
}

int
ResourceManager::initResource() {
    unsigned int targetServer;
    string logStr = "initialize the FileInfo::serverList and SubServer::fileList";
    serverLog.writeResourceLog(logStr);
    for (unsigned int i = 0; i < resourceNum; i++) {
        targetServer = i % subServerNum;
        fileList[i]->addNewServer(targetServer);
        lb->addNewFile(i, targetServer);
        logStr = "put file " + numToString(i) + " to server " + numToString(targetServer);
        serverLog.writeResourceLog(logStr);
    }
    return 0;
}
int
ResourceManager::printFileInfo(){
    for(size_t i = 0;i<fileList.size();i++){
        fileList[i]->printServerList();
    }
    return 0;
}

int 
ResourceManager::printServerInfo(){
    lb->printFileList();
    return 0;
}
int
ResourceManager::resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen) {
    string logStr = "client [" + numToString(clientId) + "] request file " + numToString(fileId);
    serverLog.writeResourceLog(logStr);
    //get the sub servers that contain the fileId
    vector<unsigned int >containList;
    filePlayLen = fileList[fileId]->filePlayLen;
    logStr = "file " + numToString(fileId) + " can play " + numToString(filePlayLen) + " secs";
    serverLog.writeResourceLog(logStr);
    containList.clear();
    containList.push_back(fileId % subServerNum);
    //choose one by random
    int targetServer;
    logStr = "find the best server to server client[" + numToString(clientId) + "]";
    serverLog.writeResourceLog(logStr);
    targetServer = lb->getMinLoadServer(containList);
    if (targetServer < 0) {
        logStr = "no server to server client[" + numToString(clientId) + "]";
        serverLog.writeResourceLog(logStr);
        return targetServer;
    }
    logStr = "the ResourceManager chose server[" + numToString(targetServer) + "] to server it";
    serverLog.writeResourceLog(logStr);
    //change the load of the chose  sub server
    lb->increaseLoad(targetServer);
    //return the id of the chose  sub server
    return targetServer;

}

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
            logStr = " the count of file[" + numToString(fileId) + "] in server[" + numToString(serverId) + "] is "
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
    logStr = "server[" + numToString(serverId) + "] chosed file[" + numToString(needSpreadFile) + "] to spread";
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
    logStr = "reset the count of file[" + numToString(needSpreadFile) + "] to 0 in all server";
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
    string logStr = "server[" + numToString(serverId) + "] chose file[" + numToString(needDeleteFile) + "] to delete";
    serverLog.writeResourceLog(logStr);
    deleteServer(needDeleteFile, serverId);
    lb->deleteFileFromSubServer(needDeleteFile, serverId);

    return 0;
}

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
            logStr = " the count of file[" + numToString(fileId) + "] in server[" + numToString(serverId) + "] is "
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

/**********************************************************************
//this part for DRManager
 **********************************************************************/

DRManager::DRManager(LBServer *_lb, ConfigType *serverConfig) : ResourceManager(_lb, serverConfig) {
    ostringstream oslogStr;
    DRBeta = serverConfig->DRBeta;
    totalCounts = 0;
    oslogStr << "period = " << period << endl;
    oslogStr << "beta = " << DRBeta << endl;
    serverLog.writeResourceLog(oslogStr.str());
    string logStr = "initialize  the probability matrix";
    serverLog.writeResourceLog(logStr);
    proMatrix = new double *[subServerNum];
    for (unsigned int i = 0; i < subServerNum; i++) {
        proMatrix[i] = new double[resourceNum];
    }
    for (unsigned int i = 0; i < subServerNum; i++) {
        for (unsigned int j = 0; j < resourceNum; j++) {
            proMatrix[i][j] = 0.0;
        }
    }
    initResource();
}

int
DRManager::initResource() {
    DRFileList.clear();
    unsigned int targetServer;
    string logStr;
    cout << "resourceNum = " << resourceNum << endl;
    for (unsigned int i = 0; i < resourceNum; i++) {
        DRFileInfo *fileInfo = new DRFileInfo(i);
        fileInfo->forecastRate = 1.0 / resourceNum;
        targetServer = i % subServerNum;
        if (fileInfo != NULL) {
            DRFileList.push_back(fileInfo);
        }
    }
    logStr = "init the DRManager::DRFileList";
    serverLog.writeResourceLog(logStr);
    for (unsigned int i = 0; i < subServerNum; i++) {
        subSerPlayRate.push_back(0.0);
    }
    logStr = "init the subSerPlayRate";
    serverLog.writeResourceLog(logStr);
    logStr = "init the probability matrix";
    serverLog.writeResourceLog(logStr);
    proAllocate();
    return 0;
}

/*@desc:sort the file according to such rules:1.the copys of i is fewer than the copys of file j,then i <j;
 * 2.if c(i) == c(j) and pro(i)> pro(j),then i<j;the sort algorithm is select sort.
 * @arg:save the ordered list in orderedList
 */
int
DRManager::getOrderedList(vector<unsigned int > &orderedList) {
    orderedList.clear();
    vector<bool> beChosed(resourceNum + 1); //show one files in already sort or not.
    for (unsigned int i = 0; i < resourceNum; i++) {
        beChosed[i] = false;
    }
    for (unsigned int i = 0; i < resourceNum; i++) {
        unsigned int minCopys = subServerNum + 1;
        unsigned int chosedId;
        double forecastPro = 1.0;
        for (unsigned int j = 0; j < resourceNum; j++) {
            if (beChosed[j] == true)
                continue;
            if (DRFileList[j]->copys < minCopys) {
                minCopys = DRFileList[j]->copys;
                chosedId = j;
                forecastPro = DRFileList[j]->forecastRate;
            } else if (DRFileList[j]->copys == minCopys) {
                if (forecastPro < DRFileList[j]->forecastRate) {
                    chosedId = j;
                    forecastPro = DRFileList[j]->forecastRate;
                }
            }
        }
        orderedList.push_back(chosedId);
        beChosed[chosedId] = true;
    }
    //output the orderedList in format like is: fileId	forecastRate copys
    ostringstream oslogStr;
    oslogStr << "the orderedList:" << endl;
    for (unsigned int i = 0; i < resourceNum; i++) {
        oslogStr << orderedList[i] << " " << DRFileList[orderedList[i]]->forecastRate << " " << DRFileList[orderedList[i]]->copys << endl;
    }
    string logStr = oslogStr.str();
    serverLog.writeResourceLog(logStr);
    return 0;
}

/*@desc:get the visit rate of each server in serverList,then put them into vector serProList in increase order
 */
int
DRManager::getSerProList(const vector<unsigned int> & serverList, vector<unsigned int> &serProList) {
    ostringstream oslogStr;
    serProList.clear();
    //sort in order
    vector<bool> bechosed(subServerNum);
    for (unsigned int i = 0; i < subServerNum; i++) {
        bechosed[i] = false;
    }
    oslogStr << "the subSerPlayRate:" << endl;
    for (unsigned int i = 0; i < subSerPlayRate.size(); i++) {
        oslogStr << subSerPlayRate[i] << " ";
    }
    string logStr = oslogStr.str();
    serverLog.writeResourceLog(logStr);
    double minPlayRate;
    unsigned int chosedId;
    for (unsigned int i = 0; i < subServerNum; i++) {
        minPlayRate = 1.0;
        for (unsigned int j = 0; j < subServerNum; j++) {
            if (bechosed[j] == true)
                continue;
            if (minPlayRate > subSerPlayRate[j]) {
                minPlayRate = subSerPlayRate[j];
                chosedId = j;
            }
        }
        serProList.push_back(chosedId);
        bechosed[chosedId] = true;
    }
    vector<unsigned int>::iterator it, tempIt;
    for (it = serProList.begin(); it != serProList.end(); it++) {
        bool isIn = false;
        for (size_t i = 0; i < serverList.size(); i++) {
            if (serverList[i] == *it) {
                isIn = true;
                break;
            }
        }
        if (isIn == false) {
            tempIt = it;
            it--;
            serProList.erase(tempIt);
        }
    }
    oslogStr.str("");
    oslogStr << "the serProList:" << endl;
    for (unsigned int i = 0; i < serProList.size(); i++) {
        oslogStr << serProList[i] << " ";
    }
    logStr = oslogStr.str();
    serverLog.writeResourceLog(logStr);
    return 0;
}

double
DRManager::getAverPro(const vector<unsigned int> &serProList) {
    double averRate = 0.0;
    for (size_t i = 0; i < serProList.size(); i++) {
        averRate += subSerPlayRate[serProList[i]];
    }
    averRate = averRate / serProList.size();
    return averRate;
}

double
DRManager::getSumPro(unsigned int serverId) {
    double aver;
    aver = subSerPlayRate[serverId];
    return aver;
}

bool
DRManager::subServerOverflow(unsigned int serverId) {
    if (subSerPlayRate[serverId] >= 1.0 / subServerNum)
        return true;
    else
        return false;
}

int
DRManager::processOverflow(unsigned int serverId, unsigned int startIndex, const vector<unsigned int> & orderedList) {
    for (unsigned int i = startIndex + 1; i < orderedList.size(); i++) {
        unsigned int fileId = orderedList.at(i);
        if (DRFileList[fileId]->copys >= 2) {
            proMatrix[serverId][fileId] = 0.0;
            //delete the fileId in serverId
        }
    }
    return 0;
}

int
DRManager::resetDRCount() {
    totalCounts = 0;
    for (unsigned int i = 0; i < DRFileList.size(); i++) {
        DRFileList[i]->counts = 0;
    }
    return 0;
}

int
DRManager::resetSubSerRate() {
    for (unsigned int i = 0; i < subServerNum; i++) {
        subSerPlayRate[i] = 0.0;
    }
    return 0;
}

int
DRManager::getForecastRate() {
    if (totalCounts == 0)//no new comers 
        return -1;
    ostringstream oslogStr;
    oslogStr << "the forecast play rate of each file: ";
    for (unsigned int i = 0; i < DRFileList.size(); i++) {
        DRFileList[i]->forecastRate = DRFileList[i]->counts / (double) totalCounts;
        oslogStr << DRFileList[i]->forecastRate << " ";
    }
    serverLog.writeResourceLog(oslogStr.str());
    return 0;
}

int
DRManager::checkBalance() {
    ostringstream oslogStr;
    oslogStr << "start check balance " << endl;
    serverLog.writeSystemLog(oslogStr.str());
    string logStr;
    while (true) {
        oslogStr << "sleep " << period << "secs" << endl;
        serverLog.writeResourceLog(oslogStr.str());
        sleep(period);
        logStr = "the period = " + numToString(period) + " get the forecast play rate of each file";
        serverLog.writeResourceLog(logStr);
        if (getForecastRate() < 0)
            continue;
        logStr = "distribute the play rate of each file to the servers that contain the file";
        serverLog.writeResourceLog(logStr);
        logStr = "after sleep " + numToString(period) + " seconds,reset the counts of all file";
        serverLog.writeResourceLog(logStr);
        resetDRCount();
        proAllocate();
        logStr = "check whether the play  rate matrix is balanced or not";
        serverLog.writeResourceLog(logStr);
        checkMatrix();
    }
    return 0;
}

int
DRManager::printProMatrix() {
    ostringstream oss;
    oss << "the proMatrix：" << endl;
    for (unsigned int j = 0; j < resourceNum; j++) {
        for (unsigned int i = 0; i < subServerNum; i++) {
            oss << proMatrix[i][j] << "\t";
        }
        oss << endl;
    }
    serverLog.writeResourceLog(oss.str());
    return 0;
}

int
DRManager::proAllocate() {
    resetSubSerRate();
    string logStr = "allocate the probability to each server ";
    serverLog.writeResourceLog(logStr);
    vector<unsigned int> serverList;
    //get the file list in order 
    vector<unsigned int> orderedList;
    getOrderedList(orderedList);
    unsigned int nums = orderedList.size();
    unsigned int copys;
    for (unsigned int i = 0; i < nums; i++) {
        copys = DRFileList[orderedList[i]]->copys;
        //get the machine that contain the file orderedList[i];
        unsigned int curFileId = orderedList.at(i);
        getServerList(curFileId, serverList);
        if (serverList.size() == 1) {//only one copy
            proMatrix[serverList[0]][curFileId] = DRFileList[curFileId]->forecastRate;
            subSerPlayRate[serverList[0]] += proMatrix[serverList[0]][curFileId];
            if (subServerOverflow(serverList[0]) == true) {
                //process the overflow
                processOverflow(serverList[0], i, orderedList);
            }
        } else {//more than one copy
            vector<unsigned int> serProList; //the subserver visit rate in order
            getSerProList(serverList, serProList);
            int serNum = serProList.size();
            double curPro, maxPro, averPro;
            double curFilePro = DRFileList[curFileId]->forecastRate;
            maxPro = getSumPro(serProList.at(serNum - 1));
            averPro = getAverPro(serProList);
            if (maxPro <= averPro + curFilePro / serNum) {
                for (int j = 0; j < serNum; j++) { // for all subserver that contain the file orderedList.at(i));
                    curPro = getSumPro(serProList.at(j));
                    proMatrix[serProList[j]][orderedList.at(i)] = curFilePro / serNum + averPro - curPro;
                    subSerPlayRate[serProList[j]] += proMatrix[serProList[j]][curFileId];
                }
            } else {
                unsigned int l = 0;
                double m = curFilePro;
                while (m > 0) {
                    curPro = getSumPro(serProList.at(l));
                    if (curPro >= m - curFilePro / serNum - averPro) {
                        proMatrix[serProList[l]][curFileId] = curFilePro / serNum + averPro - curPro;
                    } else {
                        proMatrix[serProList[l]][curFileId] = m;

                    }
                    subSerPlayRate[serProList[l]] += proMatrix[serProList[l]][curFileId];
                    m = m - proMatrix[serProList[l]][curFileId];
                    l++;
                }
                for (int j = l; j < serNum; j++) {
                    proMatrix[serProList[j]][curFileId] = 0;

                }
                for (int j = 0; j < serNum; j++) {
                    if (subServerOverflow(serProList.at(j)) == true) {
                        processOverflow(serProList.at(j), i, orderedList);
                    }
                }
            }

        }
    }
    printProMatrix();
    ostringstream oslogStr;
    oslogStr << "the subSerPlayRate:" << endl;
    for (unsigned int i = 0; i < subSerPlayRate.size(); i++) {
        oslogStr << subSerPlayRate[i] << " ";
    }
    logStr = oslogStr.str();
    serverLog.writeResourceLog(logStr);
    return 0;
}

int
DRManager::BestCopy(const vector<double> &foreRateList, vector<unsigned int> &foreCopyList) {
    unsigned int maxCopys, curCopys;
    foreCopyList.clear();
    maxCopys = subServerNum * capacity;
    curCopys = 0;
    for (unsigned int i = 0; i < resourceNum; i++) {
        foreCopyList.push_back(1);
        curCopys += 1;
    }
    double maxValue;
    unsigned int chosedFile;
    while (curCopys < maxCopys) {
        maxValue = 0.0;
        for (unsigned int i = 0; i < foreRateList.size(); i++) {
            double cur = foreRateList[i] / (foreCopyList[i] + 0.5);
            if (cur > maxValue) {
                maxValue = cur;
                chosedFile = i;
            }
        }
        if (chosedFile < resourceNum) {
            foreCopyList[chosedFile]++;
            curCopys++;
        }
    }
    return 0;
}

int
DRManager::checkMatrix() {
    double variance;
    ostringstream oslogStr;
    while (true) {
        variance = 0.0;
        double aver = 0.0;
        for (unsigned int i = 0; i < subServerNum; i++) {
            aver += subSerPlayRate[i];
        }
        aver = aver / subServerNum;
        for (unsigned int i = 0; i < subServerNum; i++) {
            variance += pow(subSerPlayRate[i] - aver, 2);
        }
        oslogStr.str("");
        oslogStr << "variance = " << variance << ",DRBeta = " << DRBeta << endl;
        serverLog.writeResourceLog(oslogStr.str());
        //check the variance 
        oslogStr.str("");
        if (variance <= DRBeta) {
            oslogStr << "the system is balance" << endl;
            serverLog.writeResourceLog(oslogStr.str());
            break;
        }
        if (variance > DRBeta) {
            oslogStr << "the system is not  balance" << endl;
            serverLog.writeResourceLog(oslogStr.str());
            vector<double> foreRateList;
            vector<unsigned int> curCopyList, foreCopyList;
            //get the forecast rate list and copy list
            for (unsigned int i = 0; i < DRFileList.size(); i++) {
                foreRateList.push_back(DRFileList[i]->forecastRate);
                curCopyList.push_back(DRFileList[i]->copys);
            }
            //get the forecast copy list according the forcast play rate list
            BestCopy(foreRateList, foreCopyList);
            //get the file that increase most
            int maxIncrease = 0 - subServerNum;
            int minIncrease = subServerNum;
            unsigned int needSpreadFile, needDeleteFile;
            for (unsigned int i = 0; i < DRFileList.size(); i++) {
                int curIncrease = foreCopyList[i] - curCopyList[i];
                if (maxIncrease < curIncrease) {
                    needSpreadFile = i;
                    maxIncrease = curIncrease;
                }
                if (minIncrease > curIncrease) {
                    minIncrease = curIncrease;
                    needDeleteFile = i;
                }
            }
            oslogStr << "maxIncrease = " << maxIncrease << ",minIncrease = " << minIncrease << endl;
            serverLog.writeResourceLog(oslogStr.str());
            if (maxIncrease > 0)
                duplicateMethod(needSpreadFile);
            if (minIncrease < 0)
                deleteMethod(needDeleteFile);
        }
    }
    return 0;
}

int
DRManager::increaseDRCount(unsigned int fileId) {
    DRFileList[fileId]->counts++;
    totalCounts++;
    return 0;
}

int
DRManager::getTargetServer(unsigned int fileId) {
    ostringstream oslogStr;
    int targetServer = -1;
    double temp = Randomf(0, 1);
    temp = temp * DRFileList[fileId]->forecastRate;
    oslogStr << "target pro = " << temp << endl;
    vector<unsigned int > serverList;
    getServerList(fileId, serverList);
    double totalRate = 0.0;
    oslogStr << "the servers that contain file " << fileId << " : " << endl;
    for (unsigned int i = 0; i < serverList.size(); i++) {
        oslogStr << serverList.at(i) << "\t" << proMatrix[serverList.at(i)][fileId] << endl;
    }
    while (serverList.size() > 0) {
        vector<unsigned int> ::iterator it, chosedIt;
        double minRate = 1.0;
        for (it = serverList.begin(); it != serverList.end(); it++) {
            //		cout<<"the pro in server "<<*it<<" of file "<<fileId<<" is "<<proMatrix[*it][fileId]<<endl;
            if (proMatrix[*it][fileId] < minRate) {
                minRate = proMatrix[*it][fileId];
                chosedIt = it;
            }
        }
        totalRate += minRate;
        //		std::cout<<"totalRate = "<<totalRate<<",temp ="<<temp<<endl;
        if (totalRate >= temp) {
            targetServer = *chosedIt;
            break;

        } else {
            serverList.erase(chosedIt);
        }
    }
    oslogStr.str("");
    oslogStr << "targetServer = " << targetServer << endl;
    serverLog.writeResourceLog(oslogStr.str());
    if (lb->isOverLoad(targetServer) >= 2) {
        targetServer = -1;
    }
    oslogStr << "targetServer = " << targetServer << endl;
    serverLog.writeResourceLog(oslogStr.str());
    return targetServer;
}

int
DRManager::
resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen) {
    string logStr = "client [" + numToString(clientId) + "] request file " + numToString(fileId);
    serverLog.writeResourceLog(logStr);
    int targetServer;
    //get the play length of file 
    filePlayLen = fileList[fileId]->filePlayLen;
    logStr = "file " + numToString(fileId) + " can play " + numToString(filePlayLen) + " secs";
    serverLog.writeResourceLog(logStr);
    //get the server that will serve the client
    targetServer = getTargetServer(fileId);
    cout << "targetServer = " << targetServer << endl;
    if (targetServer < 0)
        return targetServer;
    logStr = "the DRManager choose server[" + numToString(targetServer) + "] to server it";
    serverLog.writeResourceLog(logStr);
    //change the count of the file
    increaseDRCount(fileId);
    lb->increaseLoad(targetServer);
    return 0;
}

int
DRManager::getMinPlayRateServer(const vector<unsigned int> &noList) {
    int chosedId = -1;
    vector<unsigned int>::const_iterator it;
    double minPlayRate = 1.0;
    ostringstream oslogStr;
    oslogStr << "the total play rate of some server:" << endl;
    for (it = noList.begin(); it != noList.end(); it++) {
        oslogStr << (*it) << " " << subSerPlayRate[*it] << endl;
        if (subSerPlayRate[*it] < minPlayRate) {
            minPlayRate = subSerPlayRate[*it];
            chosedId = *it;
        }
    }
    serverLog.writeResourceLog(oslogStr.str());
    return chosedId;
}

int
DRManager::duplicateMethod(unsigned int fileId) {
    //choose the machine that have the minLoad and don't have the file
    vector<unsigned int > containList;
    getServerList(fileId, containList);
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
    int targetServer = getMinPlayRateServer(noList);
    if (targetServer < 0) {
        cout << "no sub server to spread" << endl;
        return -1;
    }
    string logStr = "spread file " + numToString(fileId) + " to server[" + numToString(targetServer) + "]";
    serverLog.writeResourceLog(logStr);
    //should have some transport time
    lb->addFileToSubServer(fileId, targetServer);
    addNewServer(fileId, targetServer);
    DRFileList[fileId]->copys++;
    logStr = "re-computer the proMatrix;";
    serverLog.writeResourceLog(logStr);
    proAllocate();
    return 0;
}

/*@desc:the trigger condition:there is some server i that proMatrix[i][fileId]=0;
 */
int
DRManager::deleteMethod(unsigned int fileId) {
    for (unsigned int i = 0; i < subServerNum; i++) {
        if (proMatrix[i][fileId] == 0.0) {
            deleteServer(fileId, i);
            lb->deleteFileFromSubServer(fileId, i);
            DRFileList[fileId]->copys--;
        }
    }
    string logStr = "re-computer the proMatrix;";
    serverLog.writeResourceLog(logStr);
    proAllocate();
    return 0;
}

