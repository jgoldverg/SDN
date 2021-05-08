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

std::vector<uint16_t> routerIds;
std::vector<uint16_t> costs;
std::vector<uint16_t> nextHops;
std::vector<uint16_t> routerPorts;
std::vector<uint16_t> dataPorts;
std::vector<uint32_t> routerIps;
std::vector<int> adjacentNodes;

int main(int argc, char **argv)
{
	ctrlPort = atoi(argv[1]);
    ctrlSock = buildCtrlSock();
    FD_ZERO(&headList);
    FD_ZERO(&viewList);
    FD_SET(ctrlSock, &headList);
    TOPFD = ctrlSock;
    timeVal.tv_sec = TIMEOUTVAL;
    timeVal.tv_usec = 0;
    mainMethod();
	return 0;
}
void mainMethod(){
    while (true){
        viewList = headList;
        auto selectRepeat = select(TOPFD+1, &viewList, NULL, NULL, &timeVal);
        if(selectRepeat < 0){
            std::string e = "Could not select for some reason";
            outError(e);
        }
        if(selectRepeat == 0){
            timeVal.tv_sec = TIMEOUTVAL;
            timeVal.tv_usec=0;
            sendRoutingUpdate();
        }
        handleFileDescriptors(TOPFD);
    }
}
void handleFileDescriptors(int TOPFD){
    int addedFd;
    for(int i = 0; i <= TOPFD; i++){ //i is the actual sockets number we can use.
        if(FD_ISSET(i, &viewList)){
            if(i == ctrlSock){
                addedFd = addConn(i, false);
                FD_SET(addedFd, &headList);
                if(TOPFD < addedFd) TOPFD = addedFd;
            }
        }else if(ctrlSock == routerSock){
            recvRouterUpdate(ctrlSock);
        }else if(ctrlSock == dataSock){
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
    memset(&info, '\0', sizeof(info));
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
    auto controlData = new char [CTRLHEADERSIZE];
    memset(controlData, '\0', CTRLHEADERSIZE);
    int bytesRead = recvAll(socket, controlData, CTRLHEADERSIZE);
    if(bytesRead < 0){
        removeConn(socket, false);
        free(controlData);
        return false;
    }
    struct CtrlMsgH* ctrlMsg = (struct CtrlMsgH*) controlData;
    uint8_t cc = ctrlMsg->controlCode;
    u_int16_t payLen = ntohs(ctrlMsg->payloadLen);
    char* payload;
    if(payLen != 0){
        payload = new char[payLen];
        memset(payload, '\0', payLen);
        auto read = recvAll(socket, payload, payLen);
        if(read < 0){
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
    if(payLen != 0) delete[]controlData;
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
    uint16_t payLen=0;
    initRouter(payload);
    auto respH = buildCtrlResponseH(socket, 1,0,payLen);
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
    uint16_t fullLength = routerCount * 8;
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
        if(routerIps[i] == cost){
            costs[i] = cost;
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
    auto totalLength = ROUTINGUPDATEHEADERSIZE + routerCount * sizeof(RoutingUpdateMsg);
    auto routerMessage = new char [totalLength];
    struct sockaddr routerInfo;
    socklen_t routerInfoLen = sizeof(routerInfo);
    memset(&routerInfo, '\0', routerInfoLen);
    recvfrom(ctrlSock, routerMessage, totalLength, 0, &routerInfo, &routerInfoLen);
    uint16_t fieldCount, rPort;
//    fieldCount = routerMessage[0] << 8 | routerMessage[1];
//    rPort = routerMessage[2] << 8 | routerMessage[3];
    memcpy(&fieldCount, &routerMessage[0], sizeof(uint16_t));//not sure if this will work
    memcpy(&rPort, &routerMessage[2], sizeof(uint16_t));//not sure how this will work
    auto offset = 18;
    for(int i = 0; i < fieldCount; i++){
        uint16_t iRouterCost = routerMessage[offset] | routerMessage[offset+1];
        offset+=12;
        auto tempCost = costs[adjacentRouter]+iRouterCost;
        if(iRouterCost == MAX && (costs[i] > tempCost)){
            costs[i] = tempCost;
            adjacentNodes[i] = routerIds[adjacentRouter];
        }
    }
    for(int i = 0; i < routerCount; i++){
        if(routerPorts[i] == rPort){
            adjacentRouter = i;
            return;
        }
    }

}

void sendRoutingUpdate(){
    uint16_t payLen=routerCount * 12, respLen;
    auto routerH = buildRouterH(routerPorts[currentRouter], routerIps[currentRouter],routerCount);
    respLen = sizeof(routerH) + payLen;
    auto routerUpdateH = new char[respLen];
    memcpy(routerUpdateH, routerH, 12);
    free(routerH);
    for(int i = 0; i < routerCount; i++){
        struct RoutingUpdateMsg updateMsg;
        updateMsg.routerId = htons(routerIds[i]);
        updateMsg.cost = htons(costs[i]);
        updateMsg.routerIp = htonl(routerIps[i]);
        updateMsg.routerPort = htons(routerPorts[i]);
        updateMsg.padding = htons(0);
        memcpy(routerUpdateH+ROUTINGUPDATEHEADERSIZE+(i*sizeof(RoutingUpdateMsg)), &updateMsg, sizeof(RoutingUpdateMsg));
    }
    for(int i = 0; i < routerCount; i++){
        if(i == currentRouter) continue;
        if(costs[i] == MAX) continue;
        auto nodetoHave = routerIds[i];
        if(adjacentNodes[i] > 0 && (std::find(nextHops.begin(), nextHops.end(), nodetoHave)) != nextHops.end()){
            struct sockaddr_in info;
            info.sin_port = htons(routerPorts[i]);
            info.sin_addr.s_addr = htonl(routerIps[i]);
            info.sin_family = AF_INET;
            auto result = sendto(routerSock, routerUpdateH, respLen, 0, (struct sockaddr*) &info, sizeof(info));
            if(result < 0){
                std::string e = "There was an error sending a router update response, the sento failed for some reason";
                outError(e);
            }
        }
    }
}

bool isAdjacentTo(int i){
    for(int idx = 0; idx < routerCount; idx++){
        if(nextHops[idx] == routerIds[i]) return true;
    }
    return false;
}


