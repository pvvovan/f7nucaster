/*
 * ClientQueue.h
 *
 *  Created on: Oct 29, 2017
 *      Author: vovan
 */

#ifndef NTRIP_CLIENTQUEUE_H_
#define NTRIP_CLIENTQUEUE_H_

#include "TcpServer.h"
#include "ringbuffer_dma.h"

void AddClientToQueue(struct tcp_server_struct* cl_state);

void RemoveClientFromQueue(struct tcp_server_struct* cl_state);

void DoSendingCorrection(RingBuffer_DMA* uart_DMA);

#endif /* NTRIP_CLIENTQUEUE_H_ */
