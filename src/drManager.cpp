
#include"drManager.h"
#include "../lib/vodlog.h"
#include "server.h"
#include<sstream>

/**********************************************************************
//this part for DRManager
 **********************************************************************/

DRManager::DRManager(LBServer *_lb, ConfigType *serverConfig) : ResourceManager(_lb, serverConfig) {
    ostringstream oslogStr;
    DRBeta = serverConfig->DRBeta;
    totalCounts = 0;
    oslogStr << "period = " << period << endl;
    oslogStr << "beta = " << DRBeta << endl;
    serverLog.writeResourceLog(oslogStr.str());
    string logStr = "initialize  the probability matrix";
    serverLog.writeResourceLog(logStr);
    proMatrix = new double *[subServerNum];
    for (unsigned int i = 0; i < subServerNum; i++) {
        proMatrix[i] = new double[resourceNum];
    }
    for (unsigned int i = 0; i < subServerNum; i++) {
        for (unsigned int j = 0; j < resourceNum; j++) {
            proMatrix[i][j] = 0.0;
        }
    }
    initResource();
}

int
DRManager::initResource() {
    DRFileList.clear();
    unsigned int targetServer;
    string logStr;
    cout << "resourceNum = " << resourceNum << endl;
    for (unsigned int i = 0; i < resourceNum; i++) {
        DRFileInfo *fileInfo = new DRFileInfo(i);
        fileInfo->forecastRate = 1.0 / resourceNum;
        targetServer = i % subServerNum;
        if (fileInfo != NULL) {
            DRFileList.push_back(fileInfo);
        }
    }
    logStr = "init the DRManager::DRFileList";
    serverLog.writeResourceLog(logStr);
    for (unsigned int i = 0; i < subServerNum; i++) {
        subSerPlayRate.push_back(0.0);
    }
    logStr = "init the subSerPlayRate";
    serverLog.writeResourceLog(logStr);
    logStr = "init the probability matrix";
    serverLog.writeResourceLog(logStr);
    proAllocate();
    return 0;
}

/*@desc:sort the file according to such rules:1.the copys of i is fewer than the copys of file j,then i <j;
 * 2.if c(i) == c(j) and pro(i)> pro(j),then i<j;the sort algorithm is select sort.
 * @arg:save the ordered list in orderedList
 */
int
DRManager::getOrderedList(vector<unsigned int > &orderedList) {
    orderedList.clear();
    vector<bool> beChosed(resourceNum + 1); //show one files in already sort or not.
    for (unsigned int i = 0; i < resourceNum; i++) {
        beChosed[i] = false;
    }
    for (unsigned int i = 0; i < resourceNum; i++) {
        unsigned int minCopys = subServerNum + 1;
        unsigned int chosedId;
        double forecastPro = 1.0;
        for (unsigned int j = 0; j < resourceNum; j++) {
            if (beChosed[j] == true)
                continue;
            if (DRFileList[j]->copys < minCopys) {
                minCopys = DRFileList[j]->copys;
                chosedId = j;
                forecastPro = DRFileList[j]->forecastRate;
            } else if (DRFileList[j]->copys == minCopys) {
                if (forecastPro < DRFileList[j]->forecastRate) {
                    chosedId = j;
                    forecastPro = DRFileList[j]->forecastRate;
                }
            }
        }
        orderedList.push_back(chosedId);
        beChosed[chosedId] = true;
    }
    //output the orderedList in format like is: fileId	forecastRate copys
    ostringstream oslogStr;
    oslogStr << "the orderedList:" << endl;
    for (unsigned int i = 0; i < resourceNum; i++) {
        oslogStr << orderedList[i] << " " << DRFileList[orderedList[i]]->forecastRate << " " << DRFileList[orderedList[i]]->copys << endl;
    }
    string logStr = oslogStr.str();
    serverLog.writeResourceLog(logStr);
    return 0;
}

/*@desc:get the visit rate of each server in serverList,then put them into vector serProList in increase order
 */
int
DRManager::getSerProList(const vector<unsigned int> & serverList, vector<unsigned int> &serProList) {
    ostringstream oslogStr;
    serProList.clear();
    //sort in order
    vector<bool> bechosed(subServerNum);
    for (unsigned int i = 0; i < subServerNum; i++) {
        bechosed[i] = false;
    }
    oslogStr << "the subSerPlayRate:" << endl;
    for (unsigned int i = 0; i < subSerPlayRate.size(); i++) {
        oslogStr << subSerPlayRate[i] << " ";
    }
    string logStr = oslogStr.str();
    serverLog.writeResourceLog(logStr);
    double minPlayRate;
    unsigned int chosedId;
    for (unsigned int i = 0; i < subServerNum; i++) {
        minPlayRate = 1.0;
        for (unsigned int j = 0; j < subServerNum; j++) {
            if (bechosed[j] == true)
                continue;
            if (minPlayRate > subSerPlayRate[j]) {
                minPlayRate = subSerPlayRate[j];
                chosedId = j;
            }
        }
        serProList.push_back(chosedId);
        bechosed[chosedId] = true;
    }
    vector<unsigned int>::iterator it, tempIt;
    for (it = serProList.begin(); it != serProList.end(); it++) {
        bool isIn = false;
        for (size_t i = 0; i < serverList.size(); i++) {
            if (serverList[i] == *it) {
                isIn = true;
                break;
            }
        }
        if (isIn == false) {
            tempIt = it;
            it--;
            serProList.erase(tempIt);
        }
    }
    oslogStr.str("");
    oslogStr << "the serProList:" << endl;
    for (unsigned int i = 0; i < serProList.size(); i++) {
        oslogStr << serProList[i] << " ";
    }
    logStr = oslogStr.str();
    serverLog.writeResourceLog(logStr);
    return 0;
}

double
DRManager::getAverPro(const vector<unsigned int> &serProList) {
    double averRate = 0.0;
    for (size_t i = 0; i < serProList.size(); i++) {
        averRate += subSerPlayRate[serProList[i]];
    }
    averRate = averRate / serProList.size();
    return averRate;
}

double
DRManager::getSumPro(unsigned int serverId) {
    double aver;
    aver = subSerPlayRate[serverId];
    return aver;
}

bool
DRManager::subServerOverflow(unsigned int serverId) {
    if (subSerPlayRate[serverId] >= 1.0 / subServerNum)
        return true;
    else
        return false;
}

int
DRManager::processOverflow(unsigned int serverId, unsigned int startIndex, const vector<unsigned int> & orderedList) {
    for (unsigned int i = startIndex + 1; i < orderedList.size(); i++) {
        unsigned int fileId = orderedList.at(i);
        if (DRFileList[fileId]->copys >= 2) {
            proMatrix[serverId][fileId] = 0.0;
            //delete the fileId in serverId
        }
    }
    return 0;
}

int
DRManager::resetDRCount() {
    totalCounts = 0;
    for (unsigned int i = 0; i < DRFileList.size(); i++) {
        DRFileList[i]->counts = 0;
    }
    return 0;
}

int
DRManager::resetSubSerRate() {
    for (unsigned int i = 0; i < subServerNum; i++) {
        subSerPlayRate[i] = 0.0;
    }
    return 0;
}

int
DRManager::getForecastRate() {
    if (totalCounts == 0)//no new comers 
        return -1;
    ostringstream oslogStr;
    oslogStr << "the forecast play rate of each file: ";
    for (unsigned int i = 0; i < DRFileList.size(); i++) {
        DRFileList[i]->forecastRate = DRFileList[i]->counts / (double) totalCounts;
        oslogStr << DRFileList[i]->forecastRate << " ";
    }
    serverLog.writeResourceLog(oslogStr.str());
    return 0;
}

int
DRManager::checkBalance() {
    ostringstream oslogStr;
    oslogStr << "start check balance " << endl;
    serverLog.writeSystemLog(oslogStr.str());
    string logStr;
    while (true) {
        oslogStr << "sleep " << period << "secs" << endl;
        serverLog.writeResourceLog(oslogStr.str());
        sleep(period);
        logStr = "the period = " + numToString(period) + " get the forecast play rate of each file";
        serverLog.writeResourceLog(logStr);
        if (getForecastRate() < 0)
            continue;
        logStr = "distribute the play rate of each file to the servers that contain the file";
        serverLog.writeResourceLog(logStr);
        logStr = "after sleep " + numToString(period) + " seconds,reset the counts of all file";
        serverLog.writeResourceLog(logStr);
        resetDRCount();
        proAllocate();
        logStr = "check whether the play  rate matrix is balanced or not";
        serverLog.writeResourceLog(logStr);
        checkMatrix();
    }
    return 0;
}

int
DRManager::printProMatrix() {
    ostringstream oss;
    oss << "the proMatrix：" << endl;
    for (unsigned int j = 0; j < resourceNum; j++) {
        for (unsigned int i = 0; i < subServerNum; i++) {
            oss << proMatrix[i][j] << "\t";
        }
        oss << endl;
    }
    serverLog.writeResourceLog(oss.str());
    return 0;
}

int
DRManager::proAllocate() {
    resetSubSerRate();
    string logStr = "allocate the probability to each server ";
    serverLog.writeResourceLog(logStr);
    vector<unsigned int> serverList;
    //get the file list in order 
    vector<unsigned int> orderedList;
    getOrderedList(orderedList);
    unsigned int nums = orderedList.size();
    unsigned int copys;
    for (unsigned int i = 0; i < nums; i++) {
        copys = DRFileList[orderedList[i]]->copys;
        //get the machine that contain the file orderedList[i];
        unsigned int curFileId = orderedList.at(i);
        getServerList(curFileId, serverList);
        if (serverList.size() == 1) {//only one copy
            proMatrix[serverList[0]][curFileId] = DRFileList[curFileId]->forecastRate;
            subSerPlayRate[serverList[0]] += proMatrix[serverList[0]][curFileId];
            if (subServerOverflow(serverList[0]) == true) {
                //process the overflow
                processOverflow(serverList[0], i, orderedList);
            }
        } else {//more than one copy
            vector<unsigned int> serProList; //the subserver visit rate in order
            getSerProList(serverList, serProList);
            int serNum = serProList.size();
            double curPro, maxPro, averPro;
            double curFilePro = DRFileList[curFileId]->forecastRate;
            maxPro = getSumPro(serProList.at(serNum - 1));
            averPro = getAverPro(serProList);
            if (maxPro <= averPro + curFilePro / serNum) {
                for (int j = 0; j < serNum; j++) { // for all subserver that contain the file orderedList.at(i));
                    curPro = getSumPro(serProList.at(j));
                    proMatrix[serProList[j]][orderedList.at(i)] = curFilePro / serNum + averPro - curPro;
                    subSerPlayRate[serProList[j]] += proMatrix[serProList[j]][curFileId];
                }
            } else {
                unsigned int l = 0;
                double m = curFilePro;
                while (m > 0 && l < subServerNum) {
                    curPro = getSumPro(serProList.at(l));
                    if (curPro >= m - curFilePro / serNum - averPro) {
                        proMatrix[serProList[l]][curFileId] = curFilePro / serNum + averPro - curPro;
                    } else {
                        proMatrix[serProList[l]][curFileId] = m;

                    }
                    subSerPlayRate[serProList[l]] += proMatrix[serProList[l]][curFileId];
                    m = m - proMatrix[serProList[l]][curFileId];
                    l++;
                }
                for (int j = l; j < serNum; j++) {
                    proMatrix[serProList[j]][curFileId] = 0;

                }
                for (int j = 0; j < serNum; j++) {
                    if (subServerOverflow(serProList.at(j)) == true) {
                        processOverflow(serProList.at(j), i, orderedList);
                    }
                }
            }

        }
    }
    printProMatrix();
    ostringstream oslogStr;
    oslogStr << "the subSerPlayRate:" << endl;
    for (unsigned int i = 0; i < subSerPlayRate.size(); i++) {
        oslogStr << subSerPlayRate[i] << " ";
    }
    logStr = oslogStr.str();
    serverLog.writeResourceLog(logStr);
    return 0;
}

int
DRManager::BestCopy(const vector<double> &foreRateList, vector<unsigned int> &foreCopyList) {
    unsigned int maxCopys, curCopys;
    foreCopyList.clear();
    maxCopys = subServerNum * capacity;
    curCopys = 0;
    for (unsigned int i = 0; i < resourceNum; i++) {
        foreCopyList.push_back(1);
        curCopys += 1;
    }
    double maxValue;
    unsigned int chosedFile;
    while (curCopys < maxCopys) {
        maxValue = 0.0;
        for (unsigned int i = 0; i < foreRateList.size(); i++) {
            double cur = foreRateList[i] / (foreCopyList[i] + 0.5);
            if (cur > maxValue) {
                maxValue = cur;
                chosedFile = i;
            }
        }
        if (chosedFile < resourceNum) {
            foreCopyList[chosedFile]++;
            curCopys++;
        }
    }
    return 0;
}

int
DRManager::checkMatrix() {
    double variance;
    ostringstream oslogStr;
    while (true) {
        variance = 0.0;
        double aver = 0.0;
        for (unsigned int i = 0; i < subServerNum; i++) {
            aver += subSerPlayRate[i];
        }
        aver = aver / subServerNum;
        for (unsigned int i = 0; i < subServerNum; i++) {
            variance += pow(subSerPlayRate[i] - aver, 2);
        }
        oslogStr.str("");
        oslogStr << "variance = " << variance << ",DRBeta = " << DRBeta << endl;
        serverLog.writeResourceLog(oslogStr.str());
        //check the variance 
        oslogStr.str("");
        if (variance <= DRBeta) {
            oslogStr << "the system is balance" << endl;
            serverLog.writeResourceLog(oslogStr.str());
            break;
        }
        if (variance > DRBeta) {
            oslogStr << "the system is not  balance" << endl;
            serverLog.writeResourceLog(oslogStr.str());
            vector<double> foreRateList;
            vector<unsigned int> curCopyList, foreCopyList;
            //get the forecast rate list and copy list
            for (unsigned int i = 0; i < DRFileList.size(); i++) {
                foreRateList.push_back(DRFileList[i]->forecastRate);
                curCopyList.push_back(DRFileList[i]->copys);
            }
            //get the forecast copy list according the forcast play rate list
            BestCopy(foreRateList, foreCopyList);
            //get the file that increase most
            int maxIncrease = 0 - subServerNum;
            int minIncrease = subServerNum;
            unsigned int needSpreadFile, needDeleteFile;
            for (unsigned int i = 0; i < DRFileList.size(); i++) {
                int curIncrease = foreCopyList[i] - curCopyList[i];
                if (maxIncrease < curIncrease) {
                    needSpreadFile = i;
                    maxIncrease = curIncrease;
                }
                if (minIncrease > curIncrease) {
                    minIncrease = curIncrease;
                    needDeleteFile = i;
                }
            }
            oslogStr.str("");
            oslogStr << "maxIncrease = " << maxIncrease << ",minIncrease = " << minIncrease << endl;
            serverLog.writeResourceLog(oslogStr.str());
            if (maxIncrease > 0)
                duplicateMethod(needSpreadFile);
            if (minIncrease < 0) {
                printFileInfo();
                printProMatrix();
                deleteMethod(needDeleteFile);
            }
        }
    }
    return 0;
}

int
DRManager::increaseDRCount(unsigned int fileId) {
    DRFileList[fileId]->counts++;
    totalCounts++;
    return 0;
}

int
DRManager::getTargetServer(unsigned int fileId) {
    ostringstream oslogStr;
    int targetServer = -1;
    double temp = Randomf(0, 1);
    temp = temp * DRFileList[fileId]->forecastRate;
    oslogStr << "target pro = " << temp << endl;
    vector<unsigned int > serverList;
    getServerList(fileId, serverList);
    double totalRate = 0.0;
    oslogStr << "the servers that contain file " << fileId << " : " << endl;
    for (unsigned int i = 0; i < serverList.size(); i++) {
        oslogStr << serverList.at(i) << "\t" << proMatrix[serverList.at(i)][fileId] << endl;
    }
    while (serverList.size() > 0) {
        vector<unsigned int> ::iterator it, chosedIt;
        double minRate = 1.0;
        for (it = serverList.begin(); it != serverList.end(); it++) {
            //		cout<<"the pro in server "<<*it<<" of file "<<fileId<<" is "<<proMatrix[*it][fileId]<<endl;
            if (proMatrix[*it][fileId] < minRate) {
                minRate = proMatrix[*it][fileId];
                chosedIt = it;
            }
        }
        totalRate += minRate;
        //		std::cout<<"totalRate = "<<totalRate<<",temp ="<<temp<<endl;
        if (totalRate >= temp) {
            targetServer = *chosedIt;
            break;

        } else {
            serverList.erase(chosedIt);
        }
    }
    oslogStr.str("");
    oslogStr << "targetServer = " << targetServer << endl;
    serverLog.writeResourceLog(oslogStr.str());
    if (lb->isOverLoad(targetServer) >= 2) {
        targetServer = -1;
    }
    oslogStr << "targetServer = " << targetServer << endl;
    serverLog.writeResourceLog(oslogStr.str());
    return targetServer;
}

int
DRManager::
resourceRequest(unsigned int clientId, unsigned int fileId, unsigned int &filePlayLen) {
    string logStr = "client [" + numToString(clientId) + "] request file " + numToString(fileId);
    serverLog.writeResourceLog(logStr);
    int targetServer;
    //get the play length of file 
    filePlayLen = fileList[fileId]->filePlayLen;
    logStr = "file " + numToString(fileId) + " can play " + numToString(filePlayLen) + " secs";
    serverLog.writeResourceLog(logStr);
    //get the server that will serve the client
    targetServer = getTargetServer(fileId);
    cout << "targetServer = " << targetServer << endl;
    if (targetServer < 0)
        return targetServer;
    logStr = "the DRManager choose server[" + numToString(targetServer) + "] to server it";
    serverLog.writeResourceLog(logStr);
    //change the count of the file
    increaseDRCount(fileId);
    lb->increaseLoad(targetServer);
    return 0;
}

int
DRManager::getMinPlayRateServer(const vector<unsigned int> &noList) {
    int chosedId = -1;
    vector<unsigned int>::const_iterator it;
    double minPlayRate = 1.0;
    ostringstream oslogStr;
    if (noList.size() == 0) {
        oslogStr << "no sub server to spread" << endl;
        serverLog.writeResourceLog(oslogStr.str());
        lb->printFileList();
    } else {
        oslogStr << "the total play rate of some server:" << endl;
        for (it = noList.begin(); it != noList.end(); it++) {
            oslogStr << (*it) << " " << subSerPlayRate[*it] << endl;
            if (subSerPlayRate[*it] < minPlayRate) {
                minPlayRate = subSerPlayRate[*it];
                chosedId = *it;
            }
        }
        serverLog.writeResourceLog(oslogStr.str());

    }
    
    return chosedId;
}

int
DRManager::duplicateMethod(unsigned int fileId) {
    //choose the machine that have the minLoad and don't have the file
    vector<unsigned int > containList;
    getServerList(fileId, containList);
    vector<unsigned int> noList;
    noList.clear();
    for (unsigned int i = 0; i < subServerNum; i++) {
        bool isContain = false;
        for (unsigned int j = 0; j < containList.size(); j++) {
            if (containList[j] == i) {
                isContain = true;
                break;
            }
        }
        if (isContain == false) {
            noList.push_back(i);
        }
    }
    this->printFileInfo();
    int targetServer = getMinPlayRateServer(noList);
    if (targetServer < 0) {
        cout << "no sub server to spread" << endl;
        return -1;
    }
    string logStr = "spread file " + numToString(fileId) + " to server[" + numToString(targetServer) + "]";
    serverLog.writeResourceLog(logStr);
    //should have some transport time
    lb->addFileToSubServer(fileId, targetServer);
    addNewServer(fileId, targetServer);
    DRFileList[fileId]->copys++;
    logStr = "re-computer the proMatrix;";
    serverLog.writeResourceLog(logStr);
    printFileInfo();
    proAllocate();
    return 0;
}

/*@desc:the trigger condition:there is some server i that proMatrix[i][fileId]=0;
 */
int
DRManager::deleteMethod(unsigned int fileId) {
    if (DRFileList[fileId]->copys <= 1)
        return -1;
    this->printFileInfo();
    ostringstream oslogStr;
    for (unsigned int i = 0; i < subServerNum; i++) {
        oslogStr.str("");
        oslogStr << " pro of file " << fileId << " in server " << i << " is " << proMatrix[i][fileId] << endl;
        serverLog.writeResourceLog(oslogStr.str());
        if (proMatrix[i][fileId] == 0.0) {
            deleteServer(fileId, i);
            lb->deleteFileFromSubServer(fileId, i);
            DRFileList[fileId]->copys--;
            if(DRFileList[fileId]->copys == 1)
                break;
        }
    }
    /*string logStr = "re-computer the proMatrix;";
    serverLog.writeResourceLog(logStr);
    proAllocate();*/
    return 0;
}

