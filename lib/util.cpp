#include "util.h"
#include <iostream>
using namespace std;


double Randomf(int a,int b){
	double temp = random()/(RAND_MAX*1.0);
	return a + (b-a) * temp;
}

int Randomi(int a,int b){
	double  temp = random()/(RAND_MAX * 1.0);
	return (int)(a + (b - a ) * temp) ;
}

string numToString(int n){
	char buf[256];
	sprintf(buf,"%d",n);
	string temp(buf);
	return temp;
}

int stringToInt(const string &value){
	int result;
	string value1;
	bool isNegative = false;
	if(value.size()>0 && value[0]=='-'){
		isNegative = true;
		value1 = value.substr(1);
	}
	if(isNegative == true){
		result = atoi(value1.c_str());
		result = 0 - result;
	}else
		result = atoi(value.c_str());
//	cout<<"value = "<<value<<endl;
//	cout<<"result = "<<result<<endl;
	return result;

}

int mysleep(unsigned int usec){
	usleep(usec);
	return 0;
}


int getCurrentTime(struct timeval *tv){
	struct timeval temp;
	gettimeofday(&temp,NULL);
	tv->tv_sec = temp.tv_sec;
	tv->tv_usec = temp.tv_usec;
	return 0;
}
long int  getTimeSlips(struct timeval *a,struct timeval *b){
	long int usec;
	usec = a->tv_sec - b->tv_sec;
	usec = usec * 1000000;
	usec += a->tv_usec - b->tv_usec;
	return usec;
}
