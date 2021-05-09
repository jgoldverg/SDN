#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <cstring>
#include <netinet/in.h>
#include <stdlib.h>
#include <structs.h>
#include <stdint.h>

#define AUTHORCTRLCODE 0
#define AUTHORCMD "I, jacobgol, have read and understood the course academic integrity policy."
#define MAX 65535


void authorCmd(int socket);
void initResponse(char* payload, int socket);
void routeTableResponse(int socket);
void updateResponse(char* payload, int socket);
void takeDown(int socket);
void startFileResponse(char* payload, int socket, uint16_t payloadSize);


void zeroOutVectors();
void init();
void mainMethod();
void handleFileDescriptors();

int buildDataSock();//create data sockets
int buildCtrlSock();//create Control sockets
int buildRouterSock();//create router sockets

int addConn(int socket, bool ctrlOrData);//add to LinkedList
void removeConn(int socket, bool ctrlOrData);//remove from LinkedList

bool handleControlData(int socket);
bool handleDataPacketData(int socket);

char* buildRouterH(uint16_t routerPort, uint32_t routerIp, uint16_t rC);
char* buildDataH(uint8_t tId, uint8_t ttl, uint16_t seqNum, bool lastChunk, uint32_t destIp);
void recvRouterUpdate(int ctrlSock);
void sendRoutingUpdate();
void initRouter(char* payLoad);

void outError(std::string message){
    perror(message.c_str());
    exit(EXIT_FAILURE);
}
struct sockaddr_in buildSockAddr(int ctrlSocket){
    struct sockaddr_in sockAddr;
    memset(&sockAddr, 0,sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    sockAddr.sin_port = htons(ctrlSocket);
    return sockAddr;
}
ssize_t recvAll(int sockIdx, char* buf, ssize_t totalBytes){
    ssize_t bytesRead = 0;
    bytesRead = recv(sockIdx, buf, totalBytes, 0);
    if(bytesRead == -1) return -1;
    while(bytesRead != totalBytes){
        bytesRead = totalBytes - bytesRead;
        bytesRead += recv(sockIdx, buf+bytesRead,bytesRead, 0);
    }
    return bytesRead;
}

ssize_t sendAll(int sockIdx, char*  buf, ssize_t totalBytes){
    ssize_t bytesRead = 0;
    bytesRead = send(sockIdx, buf, totalBytes, 0);
    if(bytesRead == -1) return -1;
    while(bytesRead != totalBytes){
        bytesRead -= totalBytes;
        bytesRead += send(sockIdx, buf+bytesRead, bytesRead, 0);
    }
    return bytesRead;
}

//build the headers should be three: control header, router header, data header
char* buildRouterH(uint16_t routerPort, uint32_t routerIp, uint16_t rC){
    char* rUpdateHeader = new char [ROUTINGUPDATEHEADERSIZE];
    struct RoutingUpdateH routingUpdateH;
    routingUpdateH.routerCount = htons(rC);
    routingUpdateH.routerIp = htonl(routerIp);
    routingUpdateH.routerPort = htons(routerPort);
    memcpy(rUpdateHeader, &routingUpdateH, ROUTINGUPDATEHEADERSIZE);
    return rUpdateHeader;
}
char* buildDataH(uint8_t tId, uint8_t ttl, uint16_t seqNum, bool lastChunk, uint32_t destIp){
    char* rUpdateH = new char[DATAPACKETHEADERSIZE];
    struct DataPacketH dataHeader;
    dataHeader.destIp = destIp;
    dataHeader.ttl = ttl;
    dataHeader.transferId = tId;
    dataHeader.seqNum = seqNum;
    if(lastChunk){
        dataHeader.isLast = htons(0x8000);
    }else{
        dataHeader.isLast = htons(0);
    }
    memcpy(rUpdateH, &dataHeader, DATAPACKETHEADERSIZE);
    return rUpdateH;
}


char* buildCtrlResponseH(int socket, uint8_t ctrlCode, uint8_t respCode, uint16_t payLen) {
    struct sockaddr_in sockaddr;
    socklen_t size = sizeof(struct sockaddr_in);
    char* buffer = new char[sizeof(char)*CTRLRESPHSIZE];
    struct CtrlMsgRespH* header = (struct CtrlMsgRespH*) buffer;
    getpeername(socket, (struct sockaddr*)&sockaddr, &size);
    memcpy(&(header->ctrlIpAddress), &(sockaddr.sin_addr), sizeof(struct in_addr));
    header->controlCode = ctrlCode;
    header->respCode = respCode;
    header->payloadLen = payLen;
    return buffer;
}

void authorCmd(int sockIdx){
    uint16_t  payLen, respLen;
    char* ctrlRespH, *ctrlRespPay, *ctrlResp;
    payLen = sizeof(AUTHORCMD)-1;//remove the null char
    ctrlRespPay = new char [payLen];
    memcpy(ctrlRespPay, AUTHORCMD, payLen);
    ctrlRespH = buildCtrlResponseH(sockIdx, 0,0,payLen);
    respLen = CTRLRESPHSIZE+payLen;
    ctrlResp = new char [respLen];
    memcpy(ctrlResp, ctrlRespH, CTRLRESPHSIZE);
    memcpy(ctrlResp+CTRLRESPHSIZE, ctrlRespPay, payLen);
    sendAll(sockIdx, ctrlResp, respLen);
    delete[](ctrlResp);
    delete[](ctrlRespPay);
    delete[](ctrlRespH);
}

//this is the old version the only difference is how I wrote it out tbh.
//void authorCmd(int sockIdx){
//    uint16_t messageSize = sizeof(AUTHORCMD)-1;
//    uint16_t respLen = CTRLRESPHSIZE + messageSize;
//    char* ctrlResp = new char[respLen];
//    char* ctrlRespH = buildCtrlResponseH(sockIdx, AUTHORCTRLCODE, AUTHORCTRLCODE, messageSize);
//    char* authorMessage = new char[messageSize];
//    strcpy(authorMessage, AUTHORCMD);
//    memcpy(ctrlResp, ctrlRespH, CTRLRESPHSIZE);
//    memcpy(ctrlResp+CTRLRESPHSIZE, authorMessage, messageSize);
//    sendAll(sockIdx, ctrlResp, respLen);
//    delete[](ctrlRespH);
//    delete[](authorMessage);
//    delete[](ctrlResp);
//}


void takeDown(int socket){
    char* header = buildCtrlResponseH(socket, 4,0,0);
    sendAll(socket, header, 8);
    free(header);
    exit(EXIT_SUCCESS);
}


#endif
