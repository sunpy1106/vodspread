#include"client.h"
#include "../lib/vodlog.h"
#define BUFSIZE 256

void * clientThread(void *arg){
	Client *curClient = (Client*)arg;
	curClient->startSession();
	return NULL;
}

void *monitorThread(void *arg){
    ClientManager* clientMaster = (ClientManager*)arg;
    clientMaster->startMonitor();
    return NULL;
}
//this part for clientManager class
ClientManager::ClientManager(ConfigType *_clientConfig){
    clientConfig=_clientConfig;
    poissonLambda = clientConfig->poissonLambda/1000.0;
    zipfParameter = clientConfig->zipfParameter/1000.0;
    clientNum = clientConfig->clientNumber;
    startedUsers = refusedUsers = 0;
    pthread_mutex_init(&count_mutex,NULL);
}

ClientManager::~ClientManager(){
    pthread_mutex_destroy(&count_mutex);
    for(unsigned int i = 0;i<clientNum;i++){
        delete clientList[i];
    }
}


int
ClientManager::startClientSide(){
    string logStr = "start a monitor thread to collect the client status";
    clientLog.writeSystemLog(logStr);
    pthread_t tid;
    pthread_create(&tid,NULL,&monitorThread,(void*)this);
    pthread_detach(tid);
    logStr = "start clients";
    clientLog.writeSystemLog(logStr);
    startClients();
    return 0;

}

int 
ClientManager::startMonitor(){
    /*
	ofstream client_ofs;
	client_ofs.open(clientConfig->resultFile.c_str());
	if(!client_ofs.is_open()){
		cout<<"open client result file error"<<endl;
		exit(1);
	}
	for(;;){
        sleep(10);
        client_ofs<<"the blockRate = "<<getBlockRate()<<endl;;

    }
	client_ofs.close();*/
    return 0;
}
int 
ClientManager::startClients(){
    vector<unsigned int >fileList(clientNum);//record the zipf list
    vector<unsigned int>timeInterval(clientNum);//record the start time interval
    int j = (clientConfig->requestListFile).find('.');
    string requestListFile = clientConfig->requestListFile.substr(0,j) + 
		'_' + numToString(clientConfig->zipfParameter)
		+'_'+numToString(clientConfig->poissonLambda )+"_" +numToString(clientConfig->clientNumber)
		+ clientConfig->requestListFile.substr(j);
//	string requestListFile = clientConfig->requestListFile;
    string logStr = "the files that each client request can be read from file "+ requestListFile;
    clientLog.writeSystemLog(logStr);
    ifstream ifs(requestListFile.c_str());
    
    //get the play file and time interval of each client

    if(!ifs){
        logStr = requestListFile +" is not exist,create the zipf list and poisson list";
        clientLog.writeSystemLog(logStr);
        createZipfList(fileList);
        getTimeInterval(timeInterval);
        ofstream ofs(requestListFile.c_str());
        for(unsigned int i = 0;i<clientNum;i++){
            ofs << fileList.at(i) <<" "<<timeInterval.at(i)<<endl;
        }
        ofs.close();
    }else{
        unsigned int a,b;
        fileList.clear();
        timeInterval.clear();
        while(!ifs.eof()){
            ifs>>a >>b;
            fileList.push_back(a);
            timeInterval.push_back(b);
        }
        ifs.close();
    }
    //create the structure of each client
    for(unsigned int i = 0;i<clientNum;i++){
        Client *client = new Client(i,fileList.at(i),clientConfig,this);
        if(client !=NULL){
            clientList.push_back(client);
        }
    }
    //start the client session according the timeInterval and filelist
    pthread_t tid;
    for(unsigned int i = 0;i<clientNum;i++){
        logStr = "after "+numToString(timeInterval.at(i))+" usecs,create client ["+numToString(i)+"]";
        clientLog.writeSystemLog(logStr);
        usleep(timeInterval.at(i));
        pthread_create(&tid,NULL,&clientThread,(void*)clientList[i]);
        pthread_detach(tid);
    }
	for(;;){
		pause();
	}
    return 0;

}


int 
ClientManager::createZipfList(vector<unsigned int> &fileList){
	double sita = clientConfig->zipfParameter;
	int n  = clientConfig->clientNumber;
    int m = clientConfig->resourceNumber;
	vector<double> probability;
	createZipfDistribution(sita,probability);
	fileList.clear();
	for( int i =0;i<n;i++){
		double temp = Randomf(0,1);
		double sum = 0.0;
		for(unsigned int i =m-1;i>=0 ;i--){
			sum +=probability.at(i);
			if(sum > temp ){
				fileList.push_back(i);
				break;
			}
		}
	}
	return 0;
}


int 
ClientManager::createZipfDistribution(double skew,vector<double> & probability){
	int n = clientConfig->resourceNumber;
	double denominator = 0.0;
	double u;
	for( int i=0;i<n;i++){				// get the sum of pow(i+1,-skew);
		denominator += pow(i+1,0-skew);
	}
	for( int i=0;i<n;i++){				// get the u1,u2,u3,...
		u = pow(i+1,0-skew);
		cout<<"u = "<<u<<endl;
		u = u / denominator;
		probability.push_back(u);
		cout<<"probability of file "<<i<<" is "<<u<<endl;
	}
	return 0;
}

int
ClientManager::getTimeInterval(vector<unsigned int> &timeInterval){
	double temp;
	timeInterval[0] =0;
	for(unsigned int i = 1;i<clientNum;i++){
		temp = Randomf(0,1);
		temp  = 1 - temp;
		temp = 0 - log(temp)/poissonLambda;
		timeInterval[i]  = (int)(temp * 1000000);
	}
	return 0;
}


double
ClientManager::getBlockRate(){
    double blockRate;
    pthread_mutex_lock(&count_mutex);
    blockRate = refusedUsers/(double)startedUsers;
    pthread_mutex_unlock(&count_mutex);
    return blockRate;
}


double 
ClientManager::getAverageWaitTime(){
    double averWaitTime;
    long result;
    unsigned int totalWaitTime  = 0;
    for(unsigned int i = 0;i<clientNum;i++){
        result = clientList[i]->getWaitTime();
        if(result > 0)
            totalWaitTime +=result;
    }
    averWaitTime = totalWaitTime/(1000000.0 * (startedUsers- refusedUsers));
    return averWaitTime;
}

int
ClientManager::increaseStartedUsers(){
    pthread_mutex_lock(&count_mutex);
    startedUsers++;
	string logStr = "startUsers = "+numToString(startedUsers);
	pthread_mutex_unlock(&count_mutex);
	clientLog.writeSystemStateLog(logStr);
    return 0;
}

int
ClientManager::increaseRefusedUsers(){
    pthread_mutex_lock(&count_mutex);
    refusedUsers++;
	string logStr = "refusedUsers = "+ numToString(refusedUsers);
    pthread_mutex_unlock(&count_mutex);
	clientLog.writeSystemStateLog(logStr);
    return 0;
}

//this part for client class

Client::Client(unsigned int _clientId,unsigned int _fileId,ConfigType *clientConfig,ClientManager* clientManager){
    clientId = _clientId;
    fileId = _fileId;
    clientMaster = clientManager;
    isServed = false;
    serverIp = clientConfig->serverIpAddress;
    serverPort = clientConfig->serverPort;
    //circleFile = clientConfig->circleFile;
}

//comunicate with server
int 
Client::sendExitMessage(int sockfd,unsigned int serverId){
    char buf[256];
    size_t len;
    Message exitMessage;
	exitMessage.type = htonl(EXIT);
	exitMessage.clientId = htonl(clientId);
    exitMessage.serverId = htonl(serverId);
	writen(sockfd,(void*)&exitMessage,sizeof(Message));
	if((len = readn(sockfd,(void*)buf,sizeof(Message )))!= sizeof( Message)){
	    cout<<"read EXIT ACK message error"<<endl;
        exit(1);
    }
    Message *message = (Message*)buf;
    if(ntohl(message->type) == EXITACK){
        close(sockfd);
        pthread_exit(NULL);
    }
	return 0;
}

int
Client::startSession(){
	int sockfd;
	int len;
    string logStr;
	char buf[BUFSIZE];
	Message rePacket;
	rePacket.type = htonl(RESOURCEREQUEST);
	rePacket.clientId = htonl(clientId);
	rePacket.serverId = htonl(LBSERVER);
	rePacket.info.fileId = htonl(fileId);
	clientMaster->increaseStartedUsers();
	while(true){
		sockfd = tcp_connect(serverIp.c_str(),serverPort.c_str());
		if(sockfd < 0){
			perror("tcp connect error");
		    exit(1);
        }
		if(writen(sockfd,(void*)&rePacket,sizeof(rePacket))!= sizeof(rePacket)){
		    cout<<"write RESOURCEREQUST message error"<<endl;
            exit(1);
		}
        logStr ="client["+numToString(clientId)+"] request file "+ numToString(fileId)+" from server";
        clientLog.writeUserStateLog(clientId,logStr);
		if((len = readn(sockfd,(void*)buf,sizeof(Message )))!= sizeof( Message)){
	        cout<<"read RESOURCEREQUEST ACK message error"<<endl;
            exit(1);
		}
		break;
	}
	Message *message = (Message*)buf;
    playLen = ntohl(message->info.playLen);
    unsigned int serverId = ntohl(message->serverId);
	if((int)serverId ==LBSERVER){
		string logStr = "refused by the server";
	    clientLog.writeUserStateLog(clientId,logStr);
		clientMaster->increaseRefusedUsers();
	}else{
	   lifeCircle(sockfd,serverId);
	}
    return 0;
}


int 
Client::lifeCircle(int sockfd,unsigned int serverId){
    string logStr = "get the file from server["+ numToString(serverId)+"],will sleep "+numToString(playLen)+" secs";
    clientLog.writeUserStateLog(clientId,logStr);
    sleep(playLen);
    //send exit message to serverid
    logStr = "wake up,it will send exit message to the server["+numToString(serverId)+"]";
    clientLog.writeUserStateLog(clientId,logStr);
    sendExitMessage(sockfd,serverId);
    return 0;
}
long 
Client::getWaitTime(){
    if(isServed == true){
        return getTimeSlips(&feedbackTime,&requestTime);
    }else
        return -1;
}
