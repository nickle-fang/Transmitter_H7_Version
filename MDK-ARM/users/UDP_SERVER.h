
/***********************************************************************
文件名称：UDP_SERVER.h
功    能：完成UDP的数据收发
编写时间：2013.4.25
编 写 人：北京智嵌物联网电子技术团队
注    意：
***********************************************************************/
#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_
#include "main.h"
#include "NRF24L01.h"

#define UDP_LOCAL_PORT     		1030

void UDP_server_init(void);
void udp_demo_senddata(void);
void udp_server_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,const ip_addr_t *addr, u16_t port);

void multicast_send_data(unsigned char * data,unsigned short len);
void udp_Multicast(void);
#endif

