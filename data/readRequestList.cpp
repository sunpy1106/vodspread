#include<iostream>
#include<fstream>
#include<vector>
#include<stdlib.h>
using namespace std;

int main(int argc,char* argv[]){
	vector<unsigned int> loads;
	unsigned int subServerNum = atoi(argv[1]);
	ifstream ifs;
	ifs.open(argv[2]);
	for(unsigned int i=0;i<subServerNum;i++){
		loads.push_back(0);
	}
	if(ifs.is_open()){
		unsigned int fileId,timeInterval;
        unsigned int clientIndex = 0;
		while(!ifs.eof()){
			ifs >> fileId >> timeInterval;
            if(ifs.fail())
                break;
            cout<<"client "<<clientIndex <<" : "<<fileId<<"\t"<<timeInterval<<endl;
            clientIndex ++;
            loads[fileId%subServerNum]++;
		}
	}
	ifs.close();
	for(unsigned int i=0;i<subServerNum;i++){
		cout<<"in machine "<<i<<" : there will have "<< loads[i]<<endl;
	}
	return 0;
}
