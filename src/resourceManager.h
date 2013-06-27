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

class DRManager : public ResourceManager {
public:
    DRManager(LBServer* _lb, ConfigType *serverConfig);

public:
    double getAverPro(const vector<unsigned int> &serProList);
    double getSumPro(unsigned int serverId);
    bool subServerOverflow(unsigned int serverId);
    int processOverflow(unsigned int serverId, unsigned int startIndex, const vector<unsigned int> & orderedList);
    int proAllocate();
    int checkMatrix();
    int getOrderedList(vector<unsigned int > &orderedList);
    int getSerProList(const vector<unsigned int> & serverList, vector<unsigned int> &serProList);
    int increaseDRCount(unsigned int fileId);
    int getTargetServer(unsigned int fileId);
    int BestCopy(const vector<double> &foreRateList, vector<unsigned int> &foreCopyList);
    int resetDRCount();
    int getMinPlayRateServer(const vector<unsigned int> &noList);
    int checkBalance();
    int getForecastRate();
    int printProMatrix();
    int resetSubSerRate();
public:
    int initResource();
    int resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen);
    int duplicateMethod(unsigned int fileId);
    int deleteMethod(unsigned int fileId);

private:

    class DRFileInfo {
    public:

        DRFileInfo(unsigned int _fileId) {
            fileId = _fileId;
            copys = 1;
            counts = 0;
        }
    public:
        unsigned int fileId;
        unsigned int copys;
        unsigned int counts;
        double forecastRate;
    };
    vector<DRFileInfo*> DRFileList;
    double **proMatrix;
    vector<double> subSerPlayRate;
    unsigned int totalCounts;
    double DRBeta;
};

#endif
