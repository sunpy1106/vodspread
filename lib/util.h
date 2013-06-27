#ifndef UTIL_H
#define UTIL_H
#include<sys/select.h>
#include<stdlib.h>
#include<time.h>
#include<sys/time.h>
#include<string>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
using namespace std;


double Randomf(int a,int b);
int Randomi(int a,int b);
string numToString(int num);
int stringToInt(const string &value);
int mysleep(unsigned int usec);

int getCurrentTime(struct timeval *tv);
long int getTimeSlips(struct timeval *a,struct timeval *b);

#endif
