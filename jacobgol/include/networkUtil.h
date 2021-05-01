#ifndef NETWORKUTIL_H
#define NETWORKUTIL_H

#include <netinet/in.h>

struct in_addr ipToStruct(uint32_t ip){
	struct in_addr temp;
	temp.s_addr = ip;
	return temp;
}
