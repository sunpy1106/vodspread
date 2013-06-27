#ifndef MESSAGE_H
#define MESSAGE_H

const int LBSERVER=1000000;

enum MessageType{
    RESOURCEREQUEST,
    REQUESTACK,
    EXIT,
    EXITACK
};


typedef struct Message{
    unsigned int type;
    unsigned int clientId;
    unsigned int serverId;
    union{
        unsigned int fileId;
        unsigned int playLen;
    }info;
}Message;
#endif
