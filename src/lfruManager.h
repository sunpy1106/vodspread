#ifndef LFRUMANAGER_H
#define LFRUMANAGER_H

#include "../include/global.h"
#include "config.h"
#include "resourceManager.h"
class LBServer;
class LFRUManager : public ResourceManager {
public:
    LFRUManager(LBServer* _lb, ConfigType *serverConfig);
public:
    int countReset();
    int getSubServerList(unsigned int fileId, vector<unsigned int > & containList);
    int readFile(unsigned int fileId, unsigned int serverId);
    int getFileWeight(unsigned int fileId);
public:
    int initResource();
    int duplicateMethod(unsigned int serverId);
    int resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen);
    int deleteMethod(unsigned int serverId);
private:

    class LFRUFileInfo {
    public:
        LFRUFileInfo(unsigned int _fileId, unsigned int _serverId) {
            fileId = _fileId;
            serverId = _serverId;
            this->count=1;
            gettimeofday(&(this->vtime),NULL);
        }
    public:
        unsigned int fileId, serverId, count;
        struct timeval vtime;
    };
    vector<LFRUFileInfo*> LFRUFileList;
    struct timeval t0;

};

#endif
