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



#endif
