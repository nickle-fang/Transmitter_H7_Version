
/***********************************************************************
�ļ����ƣ�UDP_SERVER.h
��    �ܣ����UDP�������շ�
��дʱ�䣺2013.4.25
�� д �ˣ�������Ƕ���������Ӽ����Ŷ�
ע    �⣺
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

