#ifndef DWMANAGER_H
#define DWMANAGER_H

#include "../include/global.h"
#include "config.h"
#include "resourceManager.h"
class LBServer;

class DWManager : public ResourceManager {
public:
    DWManager(LBServer* _lb, ConfigType *serverConfig);
public:
    int countReset();
    int getSubServerList(unsigned int fileId, vector<unsigned int > & containList);
    int increaseDWCount(unsigned int fileId, unsigned int serverId);
    int getTotalCount(unsigned int fileId);
public:
    int initResource();
    int duplicateMethod(unsigned int serverId);
    int resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen);
    int deleteMethod(unsigned int serverId);
private:

    class DWFileInfo {
    public:

        DWFileInfo(unsigned int _fileId, unsigned int _serverId) {
            fileId = _fileId;
            serverId = _serverId;
            c1 = 0;
            c0 = 0;
        }
    public:
        unsigned int fileId, serverId, c1, c0;
    };
    vector<DWFileInfo*> DWFileList;

};

#endif
