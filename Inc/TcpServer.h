/*
 * TcpServer.h
 *
 *  Created on: Oct 29, 2017
 *      Author: vovan
 */

#ifndef NTRIP_TCPSERVER_H_
#define NTRIP_TCPSERVER_H_

#include "lwip/tcp.h"



/* ECHO protocol states */
enum tcp_server_states
{
	ES_NONE = 0,
	ES_SENDING,
	ES_READY,
	ES_CLOSING
};

#define CLIENT_BUFFER_SIZE 10240
#define RTK_BUFFER_SIZE 4096
/* structure for maintaing connection infos to be passed as argument
   to LwIP callbacks*/
struct tcp_server_struct
{
	enum tcp_server_states state;             /* current connection state */
	u8_t retries;
	struct tcp_pcb *pcb;    /* pointer on the current tcp_pcb */
	struct pbuf *p_tx;         /* pointer on the received/to be transmitted pbuf */

	struct tcp_server_struct* m_nextElement;
	struct tcp_server_struct* m_previousElement;
	uint8_t m_IsInQueue;

	unsigned char m_buffer[CLIENT_BUFFER_SIZE];
	unsigned char m_ringBuffer[CLIENT_BUFFER_SIZE];
	int m_ringBufferPos;
	uint32_t m_lastSendEpoch;
};



void Tcp_NtripCaster_Init(void);
void Tcp_NtripCaster_Send(struct tcp_pcb *tpcb, struct tcp_server_struct * es);
void tcp_server_connection_close(struct tcp_pcb *tpcb, struct tcp_server_struct *es);

// extern int newEsCounter;
extern int newPxCounter;

#endif /* NTRIP_TCPSERVER_H_ */
