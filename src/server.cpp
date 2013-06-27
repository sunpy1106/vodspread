#include "server.h"
#define BUF_SIZE 256
#include "../lib/vodlog.h"
#include<sstream>
#include<string>
#include<iostream>
using namespace std;

void *countResetThreadForDW(void *arg) {
    DWManager *manager = (DWManager*) arg;
    manager->countReset();
    return NULL;
}
void *countResetThreadForLFRU(void *arg){
    LFRUManager *manager = (LFRUManager*) arg;
    manager->countReset();
    return NULL;
}

void *checkBalanceThread(void *arg) {
    DRManager *manager = (DRManager*) arg;
    manager->checkBalance();
    return NULL;
}
//this part for SubServer

SubServer::SubServer(unsigned int _serverId, unsigned int _maxLoad, unsigned int _capacity, double threshSkew) {
    serverId = _serverId;
    maxLoad = _maxLoad;
    currentLoad = 0;
    capacity = _capacity;
    pthread_mutex_init(&ser_mutex, NULL);
    threshold = maxLoad * threshSkew;
    //for DW
    //for DR
    //for lfru
    //for lrfu
    timeslot = 0;
}

int
SubServer::increaseLoad() {
    ostringstream logStr;
    int result;
    pthread_mutex_lock(&ser_mutex);
    result = ++currentLoad;
    logStr << "current load in server " << serverId << " is " << result;
    pthread_mutex_unlock(&ser_mutex);
    serverLog.writeServeLog(logStr.str());
    return result;
}

int
SubServer::decreaseLoad() {
    pthread_mutex_lock(&ser_mutex);
    currentLoad--;
    pthread_mutex_unlock(&ser_mutex);
    return 0;

}

unsigned int
SubServer::getCurrentLoad() {
    unsigned int curLoad;
    pthread_mutex_lock(&ser_mutex);
    curLoad = currentLoad;
    pthread_mutex_unlock(&ser_mutex);
    return curLoad;
}

int
SubServer::isOverLoad() {
    ostringstream logStr;
    int result;
    pthread_mutex_lock(&ser_mutex);
    logStr << "in server " << serverId << ",currentLoad = " << currentLoad;
    logStr << ",maxLoad = " << maxLoad << ",threshold = " << threshold;
    if (currentLoad >= maxLoad)
        result = 2;
    else if (currentLoad >= threshold)
        result = 1;
    else
        result = 0;
    pthread_mutex_unlock(&ser_mutex);
    serverLog.writeServeLog(logStr.str());
    return result;
}

bool
SubServer::isOverCapacity() {
    bool result = false;
    pthread_mutex_lock(&ser_mutex);
    if (fileList.size()> (int) capacity * 0.8) {
        result = true;
    }
    pthread_mutex_unlock(&ser_mutex);
    return result;
}

int
SubServer::addNewFile(unsigned int fileId) {
    pthread_mutex_lock(&ser_mutex);
    fileList.push_back(fileId);
    std::sort(fileList.begin(),fileList.end());
    pthread_mutex_unlock(&ser_mutex);
    return 0;
}

int
SubServer::deleteFile(unsigned int fileId) {
    vector<unsigned int>::iterator it;
    pthread_mutex_lock(&ser_mutex);
    for (it = fileList.begin(); it != fileList.end(); it++) {
        if (*it == fileId) {
            fileList.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&ser_mutex);
    return 0;
}

int
SubServer::printFileList() {
    string logStr = "the file list of server[ " + numToString(serverId) + " ] : ";
    vector<unsigned int>::iterator it;
    pthread_mutex_lock(&ser_mutex);
    for (it = fileList.begin(); it != fileList.end(); it++) {
        if (it != fileList.begin())
            logStr = logStr + "," + numToString(*it);
        else
            logStr = logStr + numToString(*it);

    }
    pthread_mutex_unlock(&ser_mutex);
    serverLog.writeServeLog(logStr);
    return 0;
}

int
SubServer::increaseTimeSlot() {
    ostringstream logStr;
    pthread_mutex_lock(&ser_mutex);
    timeslot++;
    logStr << "current time slot in sub server " << serverId << " is  " << timeslot;
    pthread_mutex_unlock(&ser_mutex);
    serverLog.writeServeLog(logStr.str());
    return 0;
}

int
SubServer::getTimeSlot() {
    int curTime;
    pthread_mutex_lock(&ser_mutex);
    curTime = (int) timeslot;
    pthread_mutex_unlock(&ser_mutex);
    return curTime;
}

//this part for LBServer

LBServer::LBServer(ConfigType *_serverConfig) {
    serverConfig = _serverConfig;
    resourceNum = _serverConfig->resourceNumber;
    subServerNum = _serverConfig->subServerNum;
    totalRequestNum = 0;
    managerAlgorithm = serverConfig->spreadAlgorithm;
}

LBServer::~LBServer() {
    for (unsigned int i = 0; i < subServerNum; i++) {
        delete subServerList[i];
    }
}

/*
int
LBServer::initResource(){
    fileList.clear();
    unsigned int long filePlayLen;
    for(int i =0;i<resourceNum;i++){
        filePlayLen = Randomi(serverConfig->minPlayLen,serverConfig->maxPlayLen); 
        FileInfo * fileInfo = new FileInfo(i,filePlayLen,1.0/(double)resourceNum);
        if(fileInfo!=NULL){
            fileList.push_back(fileInfo);
        }
    }
    return 0;
}
 */
int
LBServer::initSubServer() {
    subServerList.clear();
    unsigned int load, capacity;
    for (unsigned int i = 0; i < subServerNum; i++) {
        load = Randomi(serverConfig->minLoad, serverConfig->maxLoad);
        capacity = Randomi(serverConfig->minCapacity, serverConfig->maxCapacity);
        SubServer *subServer = new SubServer(i, load, capacity, serverConfig->loadThresh);
        if (subServer != NULL) {
            subServerList.push_back(subServer);
        }
    }
    // <editor-fold defaultstate="collapsed" desc="comment">
    string logInfo = "initilize the sub servers";
    serverLog.writeSystemLog(logInfo);
    return 0;
}

int
LBServer::messageProcess(int connfd, void *buf, size_t bufLen) {
    int clientId, serverId;
    int messageType = ntohl(*((int *) buf));
    Message *message = (Message*) buf;
    Message returnMessage;
    clientId = ntohl(message->clientId);
    serverId = ntohl(message->serverId);
    unsigned int fileId;
    string logStr = "the LBServer received message from client[" + numToString(clientId) + "].";
    string logStr1;
    unsigned int fileLen;
    switch (messageType) {
        case RESOURCEREQUEST://should find the best subserver to serve it
            if ((int) serverId != LBSERVER) {
                //write to the log
                exit(1);
            }
            logStr += "the message type is REQUESTRESOURCE";
            serverLog.writeServeLog(logStr);
            fileId = ntohl(message->info.fileId);
            //call the resource manager
            serverId = manager->resourceRequest(clientId, ntohl(message->info.fileId), fileLen);
            if (serverId >= 0) {
                logStr1 = "the client[" + numToString(clientId) + "] request file " + numToString(fileId) + ",which can play " + numToString(fileLen) + " secs.";
                logStr1 += "the LBServer chose server[" + numToString(serverId) + "] to serve it";
                serverLog.writeServeLog(logStr1);
                returnMessage.serverId = htonl(serverId);

                //send the ack message to client
            } else {
                returnMessage.serverId = htonl(LBSERVER);
            }
            returnMessage.type = htonl(REQUESTACK);
            returnMessage.clientId = htonl(clientId);
            returnMessage.info.playLen = htonl(fileLen);
            writen(connfd, (void*) &returnMessage, sizeof (Message));
            break;
        case EXIT:
            decreaseLoad(serverId);
            returnMessage.type = htonl(EXITACK);
            returnMessage.clientId = htonl(clientId);
            returnMessage.serverId = htonl(LBSERVER);
            writen(connfd, (void*) &returnMessage, sizeof (Message));
            break;
        default:
            cout << "unknown message type" << endl;
            exit(1);
            break;
    }
    return 0;
}

int
LBServer::addNewClient(int epfd, int listenfd) {
    struct epoll_event ev;
    struct sockaddr_in cliaddr;
    int len = sizeof (cliaddr);
    int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, (socklen_t*) & len);
    if (connfd < 0) {
        perror("accept error!");
        return -1;
    }
    ev.data.fd = connfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
    return connfd;
}

int
LBServer::readPacket(int connfd) {
    char buf[BUF_SIZE];
    int len = readn(connfd, (void*) buf, sizeof (Message));
    if (len < 0) {
        perror("receive error");
        return -1;
    } else if (len == 0) {
        return 0;
    } else {
        messageProcess(connfd, buf, len);
        return 1;
    }
    return 0;
}

int
LBServer::checkLoad(unsigned int serverId) {
    if(managerAlgorithm.compare("RE")==0 || managerAlgorithm.compare("DR")==0)
        return -1;
    string logStr;
    logStr = "current load of server[" + numToString(serverId) + 
            "] is " + numToString(getCurrentLoad(serverId));
    serverLog.writeSystemLog(logStr);
    subServerList[serverId]->printFileList();
    if (subServerList[serverId]->isOverLoad() >= 1) {
        manager->duplicateMethod(serverId);
    }
    if (subServerList[serverId]->isOverCapacity() == true) {
        manager->deleteMethod(serverId);
    }

    return 0;
}

int
LBServer::getCurrentLoad(unsigned int serverId) {
    return subServerList[serverId]->getCurrentLoad();
}

int
LBServer::startServerSide() {
    //initialize the server side
    pthread_t tid;
    initSubServer();
    //initialize the manager Algorithm
    if (managerAlgorithm.compare("DW") == 0) {
        string logStr = "the resourceManager is 'DW'";
        serverLog.writeSystemLog(logStr);
        logStr = "create the DWManager instance";
        serverLog.writeSystemLog(logStr);
        manager = new DWManager(this, serverConfig);
        //must create a thread to  check whether some sub servers have overload
        logStr = "create the check thread which will reset the file count of sub servers";
        serverLog.writeSystemLog(logStr);
        pthread_create(&tid, NULL, &countResetThreadForDW, (void*) manager);
        pthread_detach(tid);
    } else if (managerAlgorithm.compare("LFRU") == 0) {
        string logStr = "the resourceManager is 'LRRU'";
        serverLog.writeSystemLog(logStr);
        logStr = "create the LFRUManager instance";
        serverLog.writeSystemLog(logStr);
        manager = new LFRUManager(this, serverConfig);
        logStr = "create the check thread which will reset the file count of sub servers";
        serverLog.writeSystemLog(logStr);
        pthread_create(&tid, NULL, &countResetThreadForLFRU, (void*) manager);
        pthread_detach(tid);
    } else if (managerAlgorithm.compare("LRFU") == 0) {
        string logStr = "the resourceManager is 'LRFU'";
        serverLog.writeSystemLog(logStr);
        logStr = "create the LRFUManager instance";
        serverLog.writeSystemLog(logStr);
        manager = new LRFUManager(this, serverConfig);
    } else if (managerAlgorithm.compare("DR") == 0) {
        string logStr = "the resourceManager is 'DR'";
        serverLog.writeSystemLog(logStr);
        logStr = "create the DRManager instance";
        manager = new DRManager(this, serverConfig);
        pthread_t tid;
        pthread_create(&tid, NULL, &checkBalanceThread, (void*) manager);
        pthread_detach(tid);
    } else if (managerAlgorithm.compare("RE") == 0) {
        string logStr = "the resourceManager is 'RE'";
        serverLog.writeSystemLog(logStr);
        logStr = "create the ResourceManager instance";
        manager = new ResourceManager(this, serverConfig);

    }
    //initialize the serve part
    startLBServer();
    return 0;
}

int
LBServer::startLBServer() {
    int listen_fd = tcp_listen(serverConfig->serverPort.c_str());
    int epfd, i;
    int clientSize = 1000;
    struct epoll_event listen_ev, events[1000];
    epfd = epoll_create(10);
    if (epfd < 0) {
        perror("epoll_create error");
        exit(1);
    }
    listen_ev.events = EPOLLIN;
    listen_ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &listen_ev);
    int nready;
    for (;;) {
        nready = epoll_wait(epfd, events, clientSize, -1);
        if (nready < 0) {
            if (errno == EINTR)
                continue;
            perror("epoll_wait error");
            exit(1);
        }
        for (i = 0; i < nready; i++) {
            if (events[i].data.fd == listen_fd) {
                addNewClient(epfd, listen_fd);
            } else if (events[i].events && EPOLLIN) {
                int flag = readPacket(events[i].data.fd);
                if (flag == 0) {
                    close(events[i].data.fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, events + i);
                }
            } else if (events[i].events && EPOLLERR) {
                close(events[i].data.fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, events + i);
            }
        }
    }
    return 0;
}

int
LBServer::increaseLoad(unsigned int serverId) {
    if (serverId >= subServerList.size()) {
        cout << "unexpected server id" << endl;
        exit(1);
    }
    subServerList[serverId]->increaseLoad();
    checkLoad(serverId);
    return 0;
}

int
LBServer::decreaseLoad(unsigned int serverId) {
    if (serverId >= subServerList.size()) {
        cout << "unexpected server id" << endl;
        exit(1);
    }
    return subServerList[serverId]->decreaseLoad();

}

int
LBServer::addNewFile(unsigned int fileId, unsigned int serverId) {
    return subServerList[serverId]->addNewFile(fileId);
}

int
LBServer::getMinLoadServer(const vector<unsigned int > &containList) {
    string logStr;
    int targetServer = -1;
    unsigned int minLoad = serverConfig->maxLoad;
    for (unsigned int i = 0; i < containList.size(); i++) {
        unsigned int curServer = containList.at(i);
        if (subServerList[curServer]->isOverLoad() >= 2)
            continue;
        unsigned int curLoad = subServerList[curServer]->getCurrentLoad();
        logStr = "server[" + numToString(curServer) + "] have " + numToString(curLoad) + " loads,minLoad = " + numToString(minLoad);
        serverLog.writeSystemLog(logStr);
        if (curLoad < minLoad) {
            minLoad = curLoad;
            targetServer = curServer;
        }
    }
    return targetServer;
}

int
LBServer::addFileToSubServer(unsigned int fileId, unsigned int serverId) {
    return subServerList[serverId]->addNewFile(fileId);
}

int
LBServer::deleteFileFromSubServer(unsigned int fileId, unsigned int serverId) {
    if (serverId >= subServerNum) {
        cout << "unexpected server id" << endl;
        exit(1);
    }
    return subServerList[serverId]->deleteFile(fileId);
}

int
LBServer::getServerList(unsigned int fileId, vector<unsigned int> &serverList) {
    //fileList[fileId]->getServerList(serverList); 
    return 0;
}

int
LBServer::isOverLoad(unsigned int serverId) {
    return subServerList[serverId]->isOverLoad();
}

int
LBServer::getTimeSlot(unsigned int serverId) {
    return subServerList[serverId]->getTimeSlot();
}

int
LBServer::increaseTimeSlot(unsigned int serverId) {
    return subServerList[serverId]->increaseTimeSlot();
}

void 
LBServer::printFileList(){
    for(size_t i = 0;i<this->subServerList.size();i++){
        this->subServerList[i]->printFileList();
    }
}
