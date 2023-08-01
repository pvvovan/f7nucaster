/*
 * ClientQueue.c
 *
 *  Created on: Oct 29, 2017
 *      Author: vovan
 */


#include "ClientQueue.h"
#include "Helper.h"
#include "stdio.h"
#include "ringbuffer_dma.h"
#include "string.h"

//struct tcp_server_struct* ntripClientState = NULL;
uint32_t s_numberOfElements = 0;
struct tcp_server_struct* s_firstElement = NULL;
struct tcp_server_struct* s_lastElement = NULL;
char msg[256];

void NotifyUserWithLiveConnections() {
	if (s_numberOfElements > 0)
		HAL_GPIO_WritePin(LD2_Blue_GPIO_Port, LD2_Blue_Pin, GPIO_PIN_SET);

	if (s_numberOfElements == 0)
		HAL_GPIO_WritePin(LD2_Blue_GPIO_Port, LD2_Blue_Pin, GPIO_PIN_RESET);
}

void AddClientToQueue(struct tcp_server_struct* newElement)
{
	s_numberOfElements++;
//	ntripClientState = newElement;

	newElement->m_IsInQueue = 1;
	newElement->m_ringBufferPos = 0;
	newElement->m_nextElement = NULL;
	newElement->m_previousElement = NULL;
	newElement->m_lastSendEpoch = HAL_GetTick();

	if (s_firstElement == NULL)
	{
		s_firstElement = newElement;
		s_lastElement = s_firstElement;
	}
	else
	{
		s_lastElement->m_nextElement = newElement;
		newElement->m_previousElement = s_lastElement;
		s_lastElement = newElement;
	}

	sprintf(msg, "Clients: %d", (int)s_numberOfElements);
	Console_WriteLn(msg);
	NotifyUserWithLiveConnections();
}

void RemoveClientFromQueue(struct tcp_server_struct* element)
{
//	Console_WriteLn("ntrip removed");

	// check for NULL
	if (element !=  NULL && element->m_IsInQueue)
	{
		element->m_IsInQueue = 0;
		if (s_numberOfElements > 0)
		{
			if (element == s_firstElement && s_numberOfElements == 1)
			{
				s_firstElement = NULL;
				s_lastElement = NULL;
			}
			else if (element == s_firstElement)
			{
				s_firstElement = s_firstElement->m_nextElement;
			}
			else if (element == s_lastElement)
			{
				s_lastElement = s_lastElement->m_previousElement;
			}
			else
			{
				element->m_previousElement->m_nextElement = element->m_nextElement;
			}
			s_numberOfElements--;
		}
		mem_free(element);
//		free(element);

//		newEsCounter--;
//		char msg[100];
//		sprintf(msg, "es: %d", newEsCounter);
//		Console_WriteLn(msg);
	}
	sprintf(msg, "Clients: %d", (int)s_numberOfElements);
	Console_WriteLn(msg);
	NotifyUserWithLiveConnections();
}

int GetElements(struct tcp_server_struct** elements, int size)
{
	if (size > s_numberOfElements)
	{
		size = s_numberOfElements;
	}
	struct tcp_server_struct* currentElement = s_firstElement;
	for(int i = 0; i < size; i++)
	{
		elements[i] = currentElement;
		currentElement = currentElement->m_nextElement;
	}
	return size;
}

void addToRingBuffer(unsigned char* correction, int size, struct tcp_server_struct* state_es) {
	for (int i = 0; i < size; i++) {
		state_es->m_ringBuffer[state_es->m_ringBufferPos] = correction[i];
		state_es->m_ringBufferPos++;
		if (state_es->m_ringBufferPos == CLIENT_BUFFER_SIZE) {
			state_es->m_ringBufferPos = 0;
		}
	}
}

//#define TOTAL_ELEMENTS 256
//struct tcp_server_struct* elements[TOTAL_ELEMENTS];
uint8_t corrBuffer[RTK_BUFFER_SIZE];
uint32_t lastCorrectionReadTick = 0;

void DoSendingCorrection(RingBuffer_DMA* uart_DMA)
{
	if (HAL_GetTick() - lastCorrectionReadTick > 500)
	{
		lastCorrectionReadTick = HAL_GetTick();

		uint32_t avail = RingBuffer_DMA_Count(uart_DMA);
		if (avail > 0)
		{
			for(int i = 0; i < avail; i++)
			{
				corrBuffer[i] = RingBuffer_DMA_GetByte(uart_DMA);
			}

			uint32_t totalElements = s_numberOfElements;
			if (totalElements > 0)
			{
				struct tcp_server_struct* elements[totalElements];
				int nEls = GetElements(elements, totalElements);
				for(int i = 0; i < nEls; i++)
				{
					struct tcp_server_struct* ntState = elements[i];
					if (ntState->state == ES_READY) // send data
					{
						addToRingBuffer(corrBuffer, avail, ntState);
						memcpy(ntState->m_buffer, ntState->m_ringBuffer, ntState->m_ringBufferPos);

						/* allocate pbuf */
						ntState->p_tx = pbuf_alloc(PBUF_TRANSPORT, ntState->m_ringBufferPos, PBUF_POOL);
						if (ntState->p_tx)
						{
							newPxCounter++;
//							char msg[100];
//							sprintf(msg, "corr px: %d", newPxCounter);
//							Console_WriteLn(msg);

							/* copy data to pbuf */
							pbuf_take(ntState->p_tx, (char*) ntState->m_buffer, ntState->m_ringBufferPos);
							/* send data */
							Tcp_NtripCaster_Send(ntState->pcb, ntState);
							ntState->m_lastSendEpoch = HAL_GetTick();
							ntState->m_ringBufferPos = 0;
						}
					}
					else if (ntState->state == ES_SENDING) // store data in temporary buffer
					{
						addToRingBuffer(corrBuffer, avail, ntState);
					}
				}

				// Check and remove dead connections
				for(int i = 0; i < nEls; i++)
				{
					struct tcp_server_struct* ntState = elements[i];
					if (HAL_GetTick() - ntState->m_lastSendEpoch > 30000)
					{
						Console_WriteLn("Send timeout");
						tcp_server_connection_close(ntState->pcb, ntState);
					}
				}
			}
		}
	}

}
