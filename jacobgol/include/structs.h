#ifndef CONTROLSTRUCTS_H
#define CONTROLSTRUCTS_H

#include <stdint.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>


#define CTRLHEADERSIZE 8
#define CTRLRESPHSIZE 8
#define ROUTINGUPDATEHEADERSIZE 8
#define ROUTINGUPDATESTRUCTSIZE 12
#define CTRLCCOFFSET 0x04 //used for the response as well
#define CTRLPAYLOADOFF 0x06 //used for the response as well
#define CTRLRESPRESPCODEOFFSET 0x05 
#define DATAPACKETHEADERSIZE 12
//offset constants
#define OFFSETTWO 2
#define OFFSETFOUR 4
#define OFFSETSIX 6
#define OFFSETEIGHT 8
#define OFFSETTEN 10
#define OFFSETTWELVE 12
#define OFFSETSIXTEEN 16
#define OFFSETEIGHTEEN 18

char* buildCtrlResponseH(int sockIdx, uint8_t ctrlCode, uint8_t respCode, uint16_t payLen);

struct __attribute__((packed)) CtrlMsgH {
	uint32_t destIpAddress;
    uint8_t controlCode;
    uint8_t respTime;
    uint16_t payloadLen;
};

struct __attribute__((packed)) CtrlMsgRespH{
	uint32_t ctrlIpAddress;
	uint8_t controlCode;
    uint8_t respCode;
    uint16_t payloadLen;
};

struct __attribute__((packed)) RoutingUpdateH {
	uint16_t routerCount;
    uint16_t routerPort;
    uint32_t routerIp;
};

struct __attribute__((packed)) RoutingUpdateMsg{
	uint32_t routerIp;
	uint16_t routerPort;
	uint16_t padding;
	uint16_t routerId;
	uint16_t cost;
};

struct __attribute__((packed)) CtrlResponseRoutingTablePayload{
	uint16_t routerId;
	uint16_t nextHopId;
	uint16_t cost;
	uint16_t padding;
};

struct __attribute__((packed)) DataPacketH{
	uint32_t destIp;
	uint16_t seqNum;
	uint16_t padding;
    uint16_t isLast;
    uint8_t transferId;
    uint8_t ttl;
};








#endif
