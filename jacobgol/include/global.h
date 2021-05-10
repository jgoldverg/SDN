#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <cstring>
#include <netinet/in.h>
#include <stdlib.h>
#include <structs.h>
#include <stdint.h>
#include <sys/socket.h>

#define AUTHORCTRLCODE 0
#define AUTHORCMD "I, jacobgol, have read and understood the course academic integrity policy."
#define MAX 65535


void authorCmd(int socket);
void initResponse(char* payload, int socket);
void routeTableResponse(int socket);
void updateResponse(char* payload, int socket);
void takeDown(int socket);
void startFileResponse(char* payload, int socket, uint16_t payloadSize);

void outError(std::string message);

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
//bool handleDataPacketData(int socket);

char* buildRouterH(uint16_t routerPort, uint32_t routerIp, uint16_t rC);
char* buildCtrlResponseH(int sockIdx, uint8_t ctrlCode, uint8_t respCode, uint16_t payLen);
char* buildDataH(uint8_t tId, uint8_t ttl, uint16_t seqNum, bool lastChunk, uint32_t destIp);
void recvRouterUpdate(int sock);
void sendRoutingUpdate();
void initRouter(char* payLoad);

bool isAdjacent(int index);

bool isCtrlFd(int socket);
bool isDataFd(int socket);

ssize_t recvAll(int sockIdx, char* buf, ssize_t totalBytes);
ssize_t sendAll(int sockIdx, char* buf, ssize_t totalBytes);

struct DataConn{
    int sockfd;
    LIST_ENTRY(DataConn) next;
}*dataConn, *dataConnTemp;
LIST_HEAD(DataConnHead, DataConn) dataConnList;

struct ControlConn{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
}*conn, *ctrlListTemp;
LIST_HEAD(CtrlConnHead, ControlConn) ctrlConnList;

bool isCtrlFd(int socket){
    LIST_FOREACH(conn, &ctrlConnList, next){
        if(conn->sockfd == socket) return true;
    }
    return false;
}
bool isDataFd(int socket){
    LIST_FOREACH(dataConn, &dataConnList, next){
        if(dataConn->sockfd == socket) return true;
    }
    return false;
}

//add & remove connection either data or control connections. As routers use UDP so no connections
int addConn(int socket, bool ctrlOrData) {
    struct sockaddr_in remoteHost;
    socklen_t addrLength = sizeof(remoteHost);
    int fd = accept(socket, (struct sockaddr *) &remoteHost, &addrLength);
    if (fd < 0) {
        std::string e = "Error adding a new control file descriptor";
        outError(e);
    }
    if (!ctrlOrData) {
        conn = new ControlConn();
        conn->sockfd = fd;
        LIST_INSERT_HEAD(&ctrlConnList, conn, next);
    } else {
        dataConn = new DataConn();
        dataConn->sockfd = fd;
        LIST_INSERT_HEAD(&dataConnList, dataConn, next);
    }
    return fd;
}
void removeConn(int socket, bool ctrlOrData){
    if(!ctrlOrData){
        LIST_FOREACH(conn, &ctrlConnList, next){
            if(conn->sockfd == socket){
                LIST_REMOVE(conn, next);
                delete(conn);
            }
        }
    }else{
        LIST_FOREACH(dataConn, &dataConnList, next){
            if(dataConn->sockfd == socket){
                LIST_REMOVE(dataConn, next);
                delete(dataConn);
            }
        }
    }
    close(socket);
}

ssize_t recvAll(int sockIdx, char* buf, ssize_t totalBytes){
    ssize_t bytesRead = 0;
    bytesRead = recv(sockIdx, buf, totalBytes, 0);
    if(bytesRead == 0) return -1;
    while(bytesRead != totalBytes){
        bytesRead+= recv(sockIdx, buf+bytesRead, totalBytes-bytesRead,0);
    }
    return bytesRead;
}
ssize_t sendAll(int sockIdx, char* buf, ssize_t totalBytes){
    ssize_t bytesRead = 0;
    bytesRead = send(sockIdx, buf, totalBytes, 0);
    if(bytesRead == -1) return -1;
    while(bytesRead != totalBytes){
        bytesRead += send(sockIdx, buf+bytesRead, totalBytes-bytesRead, 0);
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
    uint16_t payLen=sizeof(AUTHORCMD)-1, respLen=CTRLRESPHSIZE+payLen;
    char* ctrlRespH = buildCtrlResponseH(sockIdx, 0,0,payLen);
    char* ctrlRespPay = new char [payLen];
    memcpy(ctrlRespPay, AUTHORCMD, payLen);
    char* ctrlResp = new char [respLen];
    memcpy(ctrlResp, ctrlRespH, CTRLRESPHSIZE);
    memcpy(ctrlResp+CTRLRESPHSIZE, ctrlRespPay, payLen);
    sendAll(sockIdx, ctrlResp, respLen);
    delete[](ctrlResp);
    delete[](ctrlRespPay);
    delete[](ctrlRespH);
}
void takeDown(int socket){
    char* header = buildCtrlResponseH(socket, 4,0,0);
    sendAll(socket, header, 8);
    free(header);
    exit(EXIT_SUCCESS);
}

void outError(std::string message){
    perror(message.c_str());
    exit(EXIT_FAILURE);
}



#endif
