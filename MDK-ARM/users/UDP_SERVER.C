/***********************************************************************
文件名称：UDP_SERVER.C
功    能：完成UDP的数据收发
编写时间：2013.4.25
编 写 人：
注    意：
***********************************************************************/
#include "UDP_SERVER.h"
#include "lwip/igmp.h"

ip_addr_t ipgroup_send;  //组播使用
ip_addr_t ipgroup_rev;  //组播使用
struct udp_pcb *udppcb_multicast;

extern double TX_Num;

//unsigned char tmp_buf[25];
extern unsigned char rx_buf[25];
extern uint8_t RX_frequency;	 //24L01接收频率
extern uint8_t TX_frequency;	 //24L01发射频率
extern uint8_t bandwidth;  //24L01带宽
uint8_t package_success = 0;
uint16_t check;
struct udp_pcb *pcb_current;		//pcb指针定义为全局变量，方便后续回传数据到上位机
struct udp_pcb *udp_multicast_pcb;  //组播使用
const uint16_t Send_port = 1030;
ip_addr_t destAddr;

int package_receive_rx = 0;

/***********************************************************************
函数名称：udp_server_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,struct ip_addr *addr, u16_t port)
功    能：udp数据接收和发送
输入参数：
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：这是一个回调函数，当一个udp段到达这个连接时会被调用
***********************************************************************/
void udp_server_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
//	struct ip_addr destAddr = *addr; /* 获取远程主机 IP地址 */

	int i=0;
	struct pbuf *p_temp = p;
	unsigned char* p2401;
	while(p_temp != NULL)
	{
		/****************将数据原样返回*******************/
		//udp_sendto(pcb, p_temp, &destAddr, port); /* 将收到的数据再发送回来 */  //////////////////////////////////测试用///
		p2401 = (unsigned char*) p_temp -> payload;

		if ((p_temp->len == 6) || (p_temp->len == 25))
		{
			if((p2401[0] == 0xF0)	&& (p_temp->len == 6))//配置包解包
			{
				check = 0;
				for(i = 0; i < 5; i ++)
					check =  check + p2401[i];
				check = check & 0xFF;
				if(check == p2401[5])
				{
          pcb_current = pcb;
					destAddr = *addr;
					TX_frequency = p2401[1];
					RX_frequency = p2401[2];
					if(p2401[4] == 0x01)
						bandwidth = 0x26;
					else if(p2401[4] == 0x02)
						bandwidth = 0x06;
					else if(p2401[4] == 0x03)
						bandwidth = 0x0F;
				  BSP_LED_Toggle(LED1);
			  	BSP_LED_Toggle(LED2);
			  	BSP_LED_Toggle(LED3);
				}
				TX_Mode();
				RX_Mode();
			}
			else if (p_temp->len == 25)   //数据包
			{
				if(NRF24L01_TxPacket(p2401) == TX_OK)
				{
					package_success++;
					//TX_Num ++;                                     //////////////////////////////////////////////////////////////for test
					if (package_success==32)
					{
						BSP_LED_Toggle(LED3);
						HAL_GPIO_TogglePin(LED_TX_GPIO_Port, LED_TX_Pin);
						package_success=0;
					}
				}
			}
		}
		
		else
		{
		}
		p_temp = p_temp -> next;
	}
	pbuf_free(p); 						/* 释放该UDP段 */
	
	
//	if(NRF24L01_RxPacket(rx_buf) == 0)   //收到数据
//		{
//			package_receive_rx ++;
//			if (package_receive_rx == 32)
//			{
//				BSP_LED_Toggle(LED1);
//				package_receive_rx = 0;
//			}
//			udp_demo_senddata();
//		}
}


/***********************************************************************
函数名称：UDP_server_init(void)
功    能：完成UDP服务器的初始化，主要是使得UDP通讯快进入监听状态
输入参数：
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：
***********************************************************************/

void UDP_server_init(void)
{
	struct udp_pcb *pcb;
	pcb = udp_new();							           	        //申请udp控制块
	udp_bind(pcb, IP_ADDR_ANY, UDP_LOCAL_PORT); 	    /* 绑定本地IP地址和端口号（作为udp服务器） */
	udp_recv(pcb, udp_server_recv, NULL); 			     	/* 设置UDP段到时的回调函数 */
}

//自定义UDP服务器发送数据包函数(此函数以测试能用)
void udp_demo_senddata()
{
	struct pbuf *ptr;
	ptr=pbuf_alloc(PBUF_TRANSPORT,RX_PLOAD_WIDTH,PBUF_RAM); //申请内存
	if(ptr != NULL)
	{
		memcpy(ptr->payload, rx_buf, RX_PLOAD_WIDTH);    //24L01收到的数据
		udp_sendto(pcb_current, ptr, &destAddr, Send_port);	  //udp发送数据 ,测试可用
		pbuf_free(ptr);//释放内存+
	}
}



/***********************************************************************
函数名称：multicast_send_data(unsigned char * data,unsigned short len)  
功    能：完成UDP服务器组播发送功能
输入参数：
输出参数：
编写时间：2019.05.23
编 写 人：方献泽
注    意：
***********************************************************************/
//void multicast_send_data(unsigned char * data_multicast,unsigned short len)
//{
//    struct pbuf *p;
//    p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
//    memcpy(p->payload, data_multicast, len);
//    udp_sendto(udp_multicast_pcb, p, (ip_addr_t *)&ipgroup_send, 2060);
//    pbuf_free(p);
//}

/************************************************************************
函数名称：udp_Multicast(void)
功    能：完成UDP服务器组播初始化功能
输入参数：
输出参数：
编写时间：2019.05.23
编 写 人：方献泽
注    意：
*************************************************************************/
//void udp_Multicast(void)
//{
//	IP4_ADDR(&ipgroup_send, 235,0,1,104);
//	IP4_ADDR(&ipgroup_rev, 230,0,1,104);
//	
//	igmp_joingroup(IP_ADDR_ANY, &ipgroup_rev);
//	udppcb_multicast = udp_new();
//	if(udppcb_multicast != NULL)
//	{
//		udp_bind(udppcb_multicast, IP_ADDR_ANY, 2060);
//		//udp_recv(udppcb_multicast, udp_server_recv, NULL); 			     	/* 设置UDP段到时的回调函数 */
//	}
//}

