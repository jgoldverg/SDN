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
#include <arpa/inet.h>


uint16_t ctrlPort=0, routerPort=0, dataPort=0;
int ctrlSock=0;


int main(int argc, char **argv)
{
	/*Start Here*/
	ctrlPort = atoi(argv[1]);
	init();
	return 0;
}

void init(){
	ctrlSock = buildCtrlSock();
    FD_ZERO(&headList);
    FD_ZERO(&viewList);
    FD_SET(ctrlSock, &headList);
}

int buildCtrlSock(){
	int so = socket(AF_INET, SOCK_STREAM,0);
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
	if(listen(so, 5)<0){
		std::string e = "failed in listening to the ctrlSocket";
		outError(e);
	}
	LIST_INIT(&ctrlConnList);
	return so;
}

void authorCmd(int sockIdx){
	uint16_t messageSize = sizeof(AUTHORCMD)-1;
	uint16_t respLen = CTRLRESPHSIZE + messageSize;
	char* ctrlResp = new char[respLen];
	char* ctrlRespH = buildCtrlResponseH(sockIdx, AUTHORCTRLCODE, AUTHORCTRLCODE, messageSize);
	char* authorMessage = new char[messageSize];
	strcpy(authorMessage, AUTHORCMD);
	memcpy(ctrlResp, ctrlRespH, CTRLRESPHSIZE);
	memcpy(ctrlResp+CTRLRESPHSIZE, authorMessage, messageSize);
	sendAll(sockIdx, ctrlResp, respLen);
	delete(ctrlRespH);
	delete[](authorMessage);
	delete[](ctrlResp);
}

char* buildCtrlResponseH(int sockIdx, uint8_t ctrlCode, uint8_t respCode, uint16_t payLen){
	char* buffer = new char[CTRLHEADERSIZE];
	struct sockaddr_in in;
	socklen_t addrSize = sizeof(struct sockaddr_in);
	getpeername(sockIdx, (struct sockaddr*) &in, &addrSize);
	memcpy(buffer, &(in.sin_addr), sizeof(struct in_addr)); //copy in the IP should be 32 bits b/c in_addr has a uint32_t as the only property
	memcpy(buffer+CTRLCCOFFSET, &ctrlCode, sizeof(ctrlCode));//copy in the ctrlCode
	memcpy(buffer+CTRLRESPRESPCODEOFFSET, &respCode, sizeof(respCode));//copy in the response code
	uint16_t networkByteOrderPayLen = htons(payLen);
	memcpy(buffer+CTRLPAYLOADOFF, &networkByteOrderPayLen, sizeof(networkByteOrderPayLen));
	return buffer;
}
