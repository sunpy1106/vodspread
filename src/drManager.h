
#ifndef DRMANAGER_H
#define DRMANAGER_H

#include "../include/global.h"
#include "config.h"
#include "resourceManager.h"
class LBServer;
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
