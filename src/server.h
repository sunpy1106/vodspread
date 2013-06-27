#ifndef SERVER_H
#define SERVER_H
#include"../include/global.h"
#include "config.h"
#include "resourceManager.h"
#include "drManager.h"
#include "lfruManager.h"
#include "lrfuManager.h"
#include "dwManager.h"

class SubServer {
public:
    SubServer(unsigned int _serverId, unsigned int _maxLoad, unsigned int _capacity,double loadThresh);
public:
    int increaseLoad();
    int decreaseLoad();
    unsigned int getCurrentLoad();
    double getVisitRate()const;
    int isOverLoad();
    bool isOverCapacity();
    int addNewFile(unsigned int fileId);
    int deleteFile(unsigned int fileId);
    int printFileList();
    int increaseTimeSlot();
    int getTimeSlot();
private://common variables ,used by all algorithm
    vector<unsigned int > fileList;
    unsigned int serverId;
    unsigned int maxLoad;
    unsigned int currentLoad;
    pthread_mutex_t ser_mutex;
    unsigned int serveTimes;
    unsigned int threshold;
    unsigned int capacity;
    unsigned int timeslot;
};

class LBServer {
public:
    LBServer(ConfigType *serverConfig);
    ~LBServer();
public:
    int initResource();
    int initSubServer();
public:// start the server ,listening and processing the client 
    int startServerSide();
    int startLBServer();
    int addNewClient(int epfd, int listenfd);
    int readPacket(int connfd);
    int messageProcess(int connfd, void *buf, size_t len);
public:
    int isOverLoad(unsigned int serverId);
    int checkLoad(unsigned int serverId);
    int getCurrentLoad(unsigned int serverId);
    int increaseLoad(unsigned int serverId);
    int decreaseLoad(unsigned int serverId);
    int getTargetServer(unsigned int clientId, unsigned int fileId);
    int increaseDRCount(unsigned int fileId);
    int addNewFile(unsigned int fileId, unsigned int serverId);
    int getMinLoadServer(const vector<unsigned int > &containList);
    double getForecastPro(unsigned int fileId);
    int addFileToSubServer(unsigned int fileId, unsigned int serverId);
    int deleteFileFromSubServer(unsigned int fileId, unsigned int serverId);
    int getServerList(unsigned int fileId, vector<unsigned int> &serverList);
    int getTimeSlot(unsigned int serverId);
    int increaseTimeSlot(unsigned int serverId);
    void printFileList();
private:
    unsigned int resourceNum;
    unsigned int allFileCounts;
    unsigned int subServerNum;
    unsigned int totalRequestNum;
    ConfigType *serverConfig;
private:
private:
    //vector<FileInfo*> fileList;
    vector<SubServer*> subServerList;
    string managerAlgorithm;
    ResourceManager *manager;
};

#endif
