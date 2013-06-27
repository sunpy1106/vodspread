#ifndef CLIENT_H
#define CLIENT_H

#include"../include/global.h"
#include "config.h"

class Client;
class ClientManager{
    public:
        ClientManager(ConfigType *_clientConfig);
        ~ClientManager();

    //class function
    public:
        int startClients(); //use lambda and sita
        int startMonitor();
        int startClientSide();
    public:
        double getBlockRate();
        double  getAverageWaitTime();
    public:
        int increaseStartedUsers();
        int increaseRefusedUsers();
    private:
        int createZipfList(vector<unsigned int> &fileList);
        int getTimeInterval(vector<unsigned int> &timeInterval);
		int createZipfDistribution(double sita,vector<double> &probability);
        //int createRequestList(vector<unsigned int> &fileList);
    //local variables;
    private:
        double poissonLambda;
        double zipfParameter;
        unsigned int clientNum;
        ConfigType * clientConfig;
    private:
        vector<Client*> clientList;
    private:
        unsigned int startedUsers;
        unsigned int refusedUsers;
        pthread_mutex_t count_mutex;
};

class Client{
    public:
        Client(unsigned int _clientId,unsigned int fileId, ConfigType* clientConfig,ClientManager *clientManager);
    public:
        int startSession();
        int lifeCircle(int sockfd ,double ,double );
        int lifeCircle(int sockfd,unsigned int serverId);
        int lifeCircle(int sockfd,string circleFile);
    public:
        int sendExitMessage(int sockfd,unsigned int serverId);

    public:
        long getWaitTime();
    private:
        unsigned int clientId;
        unsigned int fileId;
        unsigned int playLen;
        bool isServed;
        struct timeval requestTime;
        struct timeval feedbackTime;
        struct timeval finishTime;
        string circleFile;
        string serverIp,serverPort;
    private:
        ClientManager *clientMaster;
};
#endif
