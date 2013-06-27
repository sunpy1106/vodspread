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
    ostringstream logStr;
    pthread_mutex_lock(&file_mutex);
    serverList.push_back(serverId);
    logStr<<"file "<<fileId<< " now in  server "<< serverId<<endl;
    std::sort(serverList.begin(),serverList.end());
    pthread_mutex_unlock(&file_mutex);
    serverLog.writeResourceLog(logStr.str());
    return 0;
}

int
FileInfo::deleteServer(unsigned int serverId) {
    vector<unsigned int>::iterator it;
    ostringstream logStr;
    pthread_mutex_lock(&file_mutex);
    logStr<<"delete file "<<fileId<<" from server "<<serverId<<endl;
    for (it = serverList.begin(); it != serverList.end(); it++) {
        if (*it == serverId) {
            serverList.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&file_mutex);
    serverLog.writeResourceLog(logStr.str());
    return 0;

}

int
FileInfo::getServerList(vector<unsigned int> &containList) {
    pthread_mutex_lock(&file_mutex);
    containList = serverList;
    assert(serverList.size()>=1);
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





