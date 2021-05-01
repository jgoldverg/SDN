#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <netinet/in.h>
#include <stdlib.h> 
#include <structs.h>
#include <stdint.h>

#define AUTHORCTRLCODE 0
#define AUTHORCMD "I, jacobgol, have read and understood the course academic integrity policy."

fd_set headList, viewList;

void init();

void authorCmd(int sockIdx);

int buildDataSock();//this is tcp

int buildCtrlSock();//this is udp
int topFd = 0;


void outError(std::string message){
	perror(message.c_str());
	exit(EXIT_FAILURE);
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

struct in_addr ipToStruct(uint32_t ip){
	struct in_addr temp;
	temp.s_addr = ip;
	return temp;
}

struct sockaddr_in buildSockAddr(int ctrlSocket){
    struct sockaddr_in sockAddr;
    memset(&sockAddr, '\0',sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    sockAddr.sin_port = htons(ctrlSocket);
    return sockAddr;
}


#endif
