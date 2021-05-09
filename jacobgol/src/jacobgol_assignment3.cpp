/**
 * @jacobgol_assignment3
 * @author  Jacob Goldverg <jacobgol@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

#include "structs.h"
#include "global.h"
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include "networkUtil.h"
#include <sys/select.h>
#include <arpa/inet.h>
#include <vector>
#include <algorithm>
#include <iostream>



uint16_t ctrlPort=0, routerPort=0, dataPort=0;

struct timeval timeVal;
int TIMEOUTVAL=2;
int TOPFD;
int ctrlSock=0, routerSock, dataSock;
fd_set headList, viewList;

int currentRouter, adjacentRouter;
uint16_t routerCount;
uint16_t routerIdx = MAX;// the max val
uint16_t interval;

std::vector<uint16_t> routerIds(5);
std::vector<uint16_t> costs(5);
std::vector<uint16_t> nextHops(5);
std::vector<uint16_t> routerPorts(5);
std::vector<uint16_t> dataPorts(5);
std::vector<uint32_t> routerIps(5);
std::vector<int> adjacentNodes(5);

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


int main(int argc, char **argv)
{
	ctrlPort = atoi(argv[1]);
    ctrlSock = buildCtrlSock();
    std::cout << "The control port is " <<ctrlPort << std::endl;
    std::cout << "The control socket is " <<ctrlSock << std::endl;
    FD_ZERO(&headList);
    FD_ZERO(&viewList);
    FD_SET(ctrlSock, &headList);
    TOPFD = ctrlSock;
    timeVal.tv_sec = TIMEOUTVAL;
    timeVal.tv_usec = 0;
    std::cout << "entering the main method" << std::endl;
    mainMethod();
	return 0;
}
void mainMethod(){
    std::cout << "entered the mainMethod()" << std::endl;
    while (true){
        viewList = headList;
        if(routerSock > 0){
            FD_SET(routerSock, &headList);
        }
        if(routerSock > TOPFD){
            TOPFD = routerSock;
        }
        viewList = headList;
        auto selectRepeat = select(TOPFD+1, &viewList, NULL, NULL, &timeVal);
        if(selectRepeat == 0){
            timeVal.tv_sec = TIMEOUTVAL;
            timeVal.tv_usec=0;
            std::cout << "before the sendRoutingUpdate()" << std::endl;
            if(routerSock > 0){
                sendRoutingUpdate();
            }
        }
        std::cout << "before handleFileDescriptors()" << std::endl;
        handleFileDescriptors();
    }
}
void handleFileDescriptors(){
    int addedFd;
    std::cout << "The top fd is " << TOPFD << std::endl;
    std::cout << "The ctrlSock is " << ctrlSock << std::endl;
    std::cout << "The routerSock is " << routerSock << std::endl;
    for(int i = 0; i <= TOPFD; i++){ //i is the actual sockets number we can use.
        std::cout << i << std::endl;
        if(FD_ISSET(i, &viewList)){
            if(i == ctrlSock){
                addedFd = addConn(i, false);
                FD_SET(addedFd, &headList);
                if(TOPFD < addedFd) TOPFD = addedFd;
            }
        }else if(i == routerSock){
            recvRouterUpdate(ctrlSock);
        }else if(i == dataSock){ //this prob needs to go as i wont get it done
            int dataFd = addConn(i, true);
            FD_SET(dataFd, &headList);
            if(TOPFD < dataFd) TOPFD = dataFd;
        }else{
            if(isCtrlFd(ctrlSock)){
                if(!handleControlData(i)){
                    FD_CLR(i, &headList);
                }
            }else if(isDataFd(ctrlSock)){
                if(!handleDataPacketData(i)){
                    FD_CLR(i, &headList);
                }
            }
        }
    }
}
int buildCtrlSock(){
    auto so = socket(AF_INET, SOCK_STREAM,0);
    if(so < 0){
        std::string e = "issue creating the socket";
        outError(e);
    }
    int num=1;
    if(setsockopt(so, SOL_SOCKET,SO_REUSEADDR, &num, sizeof(int))<0){
        std::string e ="error making control socket reuseable";
        outError(e);
    }
    struct sockaddr_in sockAddr = buildSockAddr(ctrlSock);
    if(bind(so, (struct sockaddr*)&sockAddr, sizeof(sockAddr))<0){
        std::string e = "error binding to the control socket";
        outError(e);
    }
    if(listen(so, 5) < 0){
        std::string e = "failed in listening to the ctrlSocket";
        outError(e);
    }
    LIST_INIT(&ctrlConnList);
    return so;
}
int buildRouterSock(){
    routerSock = socket(AF_INET, SOCK_DGRAM, 0);
    if(routerSock < 0 ){
        std::string e = "There was an issue with creating the router socket";
        outError(e);
    }
    struct sockaddr_in routerInfo;
    routerInfo.sin_addr.s_addr = htonl(INADDR_ANY);
    routerInfo.sin_port = htons(routerPorts[currentRouter]);
    routerInfo.sin_family = htonl(INADDR_ANY);
    if(bind(routerSock, (struct sockaddr*) &routerInfo, sizeof(routerInfo)) < 0){
        std::string e = "failed to bind to router socket";
        outError(e);
    }
    return routerSock;
}
int buildDataSock(){
    struct sockaddr_in info;
    dataSock = socket(AF_INET, SOCK_STREAM, 0);
    auto num=1;
    setsockopt(dataSock, SOL_SOCKET,SO_REUSEADDR, &num, sizeof(int));
    memset(&info, 0, sizeof(info));
    info.sin_addr.s_addr = htonl(INADDR_ANY);
    info.sin_port = htons(dataPorts[currentRouter]);
    bind(dataSock, (struct sockaddr*)&info, sizeof(socklen_t));
    listen(dataSock, 5);
    LIST_INIT(&dataConnList);
    return dataSock;
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
        dataConn = new DataConn();
        dataConn->sockfd = fd;
        LIST_INSERT_HEAD(&dataConnList, dataConn, next);
        return fd;
    } else {
        dataConn = new DataConn();
        dataConn->sockfd = fd;
        LIST_INSERT_HEAD(&dataConnList, dataConn, next);
        return fd;
    }
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
            }
        }
    }
    close(socket);
}
bool handleControlData(int socket){
    auto controlHeader = new char [sizeof(char)*CTRLHEADERSIZE];
    memset(controlHeader, 0, CTRLHEADERSIZE);
    if(recvAll(socket, controlHeader, CTRLHEADERSIZE) < 0){
        removeConn(socket, false);
        delete[](controlHeader);
        return false;
    }
    struct CtrlMsgH* ctrlMsg = (struct CtrlMsgH*) controlHeader;
    auto cc = ctrlMsg->controlCode;
    auto payLen = ntohs(ctrlMsg->payloadLen);
    char* payload;
    delete[](controlHeader);
    if(payLen != 0){
        payload = new char[sizeof(char) * payLen];
        memset(payload, 0, payLen);
        if(recvAll(socket, payload, payLen) < 0){
            removeConn(socket, false);
            delete[](payload);
            return false;
        }
    }
    if(cc == 0){
        authorCmd(socket);
    }else if(cc == 1){
        //initialize the response
        initResponse(payload, socket);
    }else if(cc == 2){
        //table response
        routeTableResponse(socket);
    }else if(cc == 3){
        //update response
        updateResponse(payload,socket);
    }else if(cc == 4){
        //takeDown response
        takeDown(socket);
    }else if(cc == 5){
        //send file response
        startFileResponse(payload, socket, payLen);
    }
    if(payLen != 0) delete[](ctrlMsg);
    return true;
}
void initRouter(char* payLoad){
    int offset = 0;
    memcpy(&routerCount, payLoad, sizeof(routerCount));
    routerCount = ntohs(routerCount);
    offset+=2;
    memcpy(&interval, payLoad+offset, sizeof(interval));
    timeVal.tv_sec = ntohs(interval);
    timeVal.tv_usec = 0;
    zeroOutVectors();
    for(int i = 0; i < routerCount; i++){
        auto currentOff = i* OFFSETTWELVE;
        memcpy(&routerIds[i], payLoad+OFFSETFOUR+currentOff, sizeof(routerIds[i]));
        memcpy(&routerPorts[i], payLoad+OFFSETSIX+currentOff, sizeof(routerPorts[i]));
        memcpy(&dataPorts[i], payLoad+OFFSETEIGHT+currentOff, sizeof(dataPorts[i]));
        memcpy(&costs[i], payLoad+OFFSETTEN+currentOff, sizeof(costs[i]));
        memcpy(&routerIps[i], payLoad+OFFSETTWELVE+currentOff, sizeof(routerIps[i]));
        if(costs[i] == MAX){
            adjacentNodes[i] = 1;
            costs[i] = MAX;
        }else{
            nextHops[i] = MAX;
            adjacentNodes[i] = 0;
        }
        if(costs[i] == 0){
            currentRouter = i;
        }
    }
}
void initResponse(char* payload, int socket){
    initRouter(payload);
    auto respH = buildCtrlResponseH(socket, 1,0,0);
    sendAll(socket, respH, CTRLRESPHSIZE);
    free(respH);
    routerSock = buildRouterSock();
    FD_SET(routerSock, &headList);
    if(routerSock > TOPFD){
        TOPFD = routerSock;
    }
    dataSock = buildDataSock();
    if(dataSock > TOPFD){
        TOPFD = dataSock;
    }
}
void routeTableResponse(int socket){
    uint16_t payLen=routerCount*8;
    auto respRH = buildCtrlResponseH(socket, 2,0,payLen);
    uint16_t fullLength = payLen + 8;
    auto respR = new char [fullLength];
    memcpy(&respR, respRH, CTRLRESPHSIZE);//copy in the header
    free(respRH);
    for(int i = 0; i < routerCount; i++){
        struct CtrlResponseRoutingTablePayload routerLoad;
        routerLoad.routerId = htons(routerIds[i]);
        routerLoad.cost = htons(costs[i]);
        routerLoad.nextHopId = htons(nextHops[i]);
        routerLoad.padding = htons(0);
        memcpy(respR + (CTRLRESPHSIZE+(i*8)), &routerLoad, sizeof(routerLoad));
    }
    sendAll(socket, respR, fullLength);
    free(respR);
}
void zeroOutVectors(){
    for(int i = 0; i < routerCount; i++){
        costs[i] = MAX;
        nextHops[i] = MAX;
        routerIds[i] = MAX;
        adjacentNodes[i] = INT8_MIN;
    }
}
void updateResponse(char* payload, int socket){
    uint16_t routerId, cost;
//    routerId = (uint16_t) (payload[0] << 8 | payload[1]);
//    cost = (uint16_t)(payload[2] << 8 | payload[3]);
    memcpy(&routerId, &payload[0], sizeof(uint16_t));//not sure if this works
    memcpy(&cost, &payload[2], sizeof(uint16_t));//not sure if this works
    auto respH = buildCtrlResponseH(socket, 3,0,0);
    for(int i = 0; i < routerCount; i++){
        if(routerIds[i] == routerId){
            costs[i] = cost;
            if(cost == MAX){
                nextHops[i] = MAX;
            }else{
                nextHops[i] = routerId;
            }
        }
    }
    sendAll(socket, respH, 8);
    free(respH);
}
void startFileResponse(char* payload, int socket, uint16_t payloadSize){

}

bool handleDataPacketData(int socket){
    return false;
}

void recvRouterUpdate(int ctrlSock){
    auto totalLength = OFFSETEIGHT + (OFFSETTWELVE*routerCount);
    auto routerMessage = new char [totalLength];
    struct sockaddr routerInfo;
    socklen_t routerInfoLen = sizeof(routerInfo);
    memset(&routerInfo, 0, routerInfoLen);
    recvfrom(ctrlSock, routerMessage, totalLength, 0, &routerInfo, &routerInfoLen);
    auto fieldCount = (uint8_t)routerMessage[0] << 8 | (uint8_t)routerMessage[1];
    auto rPort = (uint8_t)routerMessage[2] << 8 | (uint8_t)routerMessage[3];
    auto offset = 18;
    auto temp = -1;
    for(int i = 0; i < routerCount; i++){
        if(routerPorts[i] == rPort){
            temp = i;
            break;
        }
    }
    for(int i = 0; i < fieldCount; i++){
        auto iRouterCost = routerMessage[offset] << 8 | routerMessage[offset+1] << 8;
        if(iRouterCost != MAX && (costs[i] > costs[temp]+iRouterCost)){
            costs[i] = costs[temp]+iRouterCost;
            nextHops[i] = routerIds[temp];
        }
        offset+=ROUTINGUPDATESTRUCTSIZE;
    }
}

void sendRoutingUpdate(){
    uint16_t payLen = routerCount * ROUTINGUPDATESTRUCTSIZE, respLen=ROUTINGUPDATEHEADERSIZE + payLen;
    int bufferSize = OFFSETEIGHT+(OFFSETTWELVE*routerCount);
    char* routerH = buildRouterH(routerPorts[currentRouter], routerIps[currentRouter],routerCount);
    char* routerUpdateResp = new char[bufferSize];
    memset(&routerUpdateResp, 0, bufferSize);
    memcpy(routerUpdateResp, routerH, ROUTINGUPDATEHEADERSIZE);
    free(routerH);
    for(int i = 0; i < routerCount; i++){
        struct RoutingUpdateMsg updateMsg;
        updateMsg.routerId = htons(routerIds[i]);
        updateMsg.cost = htons(costs[i]);
        updateMsg.routerIp = htonl(routerIps[i]);
        updateMsg.routerPort = htons(routerPorts[i]);
        updateMsg.padding = htons(0);
        memcpy(routerUpdateResp+ROUTINGUPDATEHEADERSIZE+(i*sizeof(RoutingUpdateMsg)), &updateMsg, ROUTINGUPDATESTRUCTSIZE);
    }
    for(int i = 0; i < routerCount; i++){
        if(i == currentRouter) continue;
        if(costs[i] == MAX) continue;
        if(adjacentNodes[i] > 0 && costs[i] != MAX && (std::find(nextHops.begin(), nextHops.end(), routerIds[i])) != nextHops.end()){
            struct sockaddr_in info;
            memset(&info, 0, sizeof(info));
            info.sin_port = htons(routerPorts[i]);
            info.sin_addr.s_addr = htonl(routerIps[i]);
            info.sin_family = AF_INET;
            sendto(routerSock, routerUpdateResp, respLen, 0, (struct sockaddr*) &info, sizeof(info));
        }
    }
    if(routerUpdateResp){
        free(routerUpdateResp);
    }
}

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

void takeDown(int socket){
    char* header = buildCtrlResponseH(socket, 4,0,0);
    sendAll(socket, header, 8);
    free(header);
    exit(EXIT_SUCCESS);
}





