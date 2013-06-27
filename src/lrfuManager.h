
#ifndef LRFUMANAGER_H
#define LRFUMANAGER_H

#include "../include/global.h"
#include "config.h"
#include "resourceManager.h"
class LBServer;
class LRFUManager : public ResourceManager {
public:
    LRFUManager(LBServer* _lb, ConfigType *serverConfig);
public:
    
    int getSubServerList(unsigned int fileId, vector<unsigned int > & containList);
    int readFile(unsigned int fileId, unsigned int serverId);
public:
    int initResource();
    int duplicateMethod(unsigned int serverId);
    int resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen);
    int deleteMethod(unsigned int serverId);
private:

    class LRFUFileInfo {
    public:

        LRFUFileInfo(unsigned int _fileId, unsigned int _serverId) {
            fileId = _fileId;
            serverId = _serverId;
            this->vtime = 0;
            crfValue = 1.0;
        }
    public:
        unsigned int fileId, serverId;
        int vtime;
        double crfValue;
    };
    vector<LRFUFileInfo*> LRFUFileList;
    double lambda;
};
#endif
