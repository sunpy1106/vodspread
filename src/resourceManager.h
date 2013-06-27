#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "../include/global.h"
#include "config.h"
class LBServer;

class FileInfo {
public:
    FileInfo(unsigned int _fileId, unsigned int _filePlayLen);
    ~FileInfo();
public:
    int addNewServer(unsigned int serverId);
    int deleteServer(unsigned int serverId);
    int getServerList(vector<unsigned int> &containList);
    void printServerList();
public:
    unsigned int fileId;
    unsigned int filePlayLen;
    pthread_mutex_t file_mutex;
    vector<unsigned int> serverList;
};

class ResourceManager {
public:
    ResourceManager(LBServer *_lb, ConfigType *serverConfig);
    ~ResourceManager();
public:
    int getServerList(unsigned int fileId, vector<unsigned int> & serverList);
    int addNewServer(unsigned int fileId, unsigned int serverId);
    int deleteServer(unsigned int fileId, unsigned int serverId);
    int initResource();
    int printFileInfo();
    int printServerInfo();
public:
    virtual int resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen);

    virtual int duplicateMethod(unsigned int) {
        return 0;
    }

    virtual int deleteMethod(unsigned int) {
        return 0;
    }

protected:
    LBServer *lb;
    unsigned int resourceNum;
    unsigned int subServerNum;
    unsigned int capacity;
    unsigned int period; //in secs;
    vector<FileInfo*> fileList;
};





#endif
