
#include <sys/socket.h>
#include <stdint.h>
#include <sys/queue.h>
#include "structs.h"
#include "global.h"
struct DataConn{
	int sockfd;
	LIST_ENTRY(DataConn) next;
}*dataList, *dataConnTemp;
LIST_HEAD(DataConnHead, DataConn) dataConnList;

struct ControlConn{
	int sockfd;
	LIST_ENTRY(ControlConn) next;
}*ctrlList, *ctrlListTemp;
LIST_HEAD(CtrlConnHead, ControlConn);


