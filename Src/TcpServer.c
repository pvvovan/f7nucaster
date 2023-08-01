#include "TcpServer.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "string.h"
#include "ClientQueue.h"
#include "Helper.h"

char NtripHeader[256];

static struct tcp_pcb *tcp_server_pcb;

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static void tcp_server_error(void *arg, err_t err);
static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

int newPxCounter = 0;

/**
  * @brief  Initializes the tcp NTrip Caster server
  * @param  None
  * @retval None
  */
void Tcp_NtripCaster_Init(void)
{
//	newEsCounter = 0;
	newPxCounter = 0;
//#warning setup NtripHeader
	sprintf(NtripHeader, "ICY 200 OK\r\nNtrip-Version: Ntrip/1.0\r\nServer: NTRIP Caster 1.0\r\nDate: 12/5/2018 2:04:17\r\nContent-Type: gnss/data\r\n\r\n");

	/* create new tcp pcb */
	tcp_server_pcb = tcp_new();

	if (tcp_server_pcb != NULL) {
		err_t err;

		/* bind echo_pcb to port 7 (ECHO protocol) */
		err = tcp_bind(tcp_server_pcb, IP_ADDR_ANY, 2101);

		if (err == ERR_OK) {
			/* start tcp listening for echo_pcb */
			tcp_server_pcb = tcp_listen(tcp_server_pcb);

			/* initialize LwIP tcp_accept callback function */
			tcp_accept(tcp_server_pcb, tcp_server_accept);
		} else {
			/* deallocate the pcb */
			memp_free(MEMP_TCP_PCB, tcp_server_pcb);
		}
	}
}


/**
  * @brief  This function is the implementation of tcp_accept LwIP callback
  * @param  arg: not used
  * @param  newpcb: pointer on tcp_pcb struct for the newly created tcp connection
  * @param  err: not used
  * @retval err_t: error status
  */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	err_t ret_err;
	struct tcp_server_struct *es;

	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);

	/* set priority for the newly accepted tcp connection newpcb */
	tcp_setprio(newpcb, TCP_PRIO_MIN);

	/* allocate structure es to maintain tcp connection informations */
	es = (struct tcp_server_struct *)mem_malloc(sizeof(struct tcp_server_struct));
//	es = (struct tcp_server_struct *)malloc(sizeof(struct tcp_server_struct));

	if (es != NULL)
	{
//		newEsCounter++;
//		char msg[100];
//		sprintf(msg, "es: %d", newEsCounter);
//		Console_WriteLn(msg);

		es->state = ES_SENDING;
		es->pcb = newpcb;
		es->retries = 0;
		es->p_tx = NULL;

		/* pass newly allocated es structure as argument to newpcb */
		tcp_arg(newpcb, es);

//		#warning add client to queue and send header
		//    /* initialize lwip tcp_recv callback function for newpcb  */
		//    tcp_recv(newpcb, tcp_echoserver_recv);
		AddClientToQueue(es);

		/* allocate pbuf */
		es->p_tx = pbuf_alloc(PBUF_TRANSPORT, strlen(NtripHeader) , PBUF_POOL);

		if (es->p_tx)
		{
			newPxCounter++;
//			char msg[100];
//			sprintf(msg, "header px: %d", newPxCounter);
//			Console_WriteLn(msg);

			/* copy data to pbuf */
			pbuf_take(es->p_tx, NtripHeader, strlen(NtripHeader));

			/* pass newly allocated es structure as argument to tpcb */
			tcp_arg(newpcb, es);

			/* initialize lwip tcp_err callback function for newpcb  */
			tcp_err(newpcb, tcp_server_error);

			/* initialize LwIP tcp_recv callback function */
			tcp_recv(newpcb, tcp_server_recv);

			/* initialize LwIP tcp_sent callback function */
			tcp_sent(newpcb, tcp_server_sent);

			/* initialize LwIP tcp_poll callback function */
			tcp_poll(newpcb, tcp_server_poll, 1);

			/* send data */
			Tcp_NtripCaster_Send(newpcb, es);

			return ERR_OK;
		}

		//    /* initialize lwip tcp_err callback function for newpcb  */
		//    tcp_err(newpcb, tcp_server_error);
		//
		//    /* initialize lwip tcp_poll callback function for newpcb */
		//    tcp_poll(newpcb, tcp_server_poll, 0);

		//    ret_err = ERR_OK;

		Console_WriteLn("Low memory 1");
		/* return memory allocation error */
		return ERR_MEM;
	}
	else
	{
		/*  close tcp connection */
		Console_WriteLn("Low memory 2");
		tcp_server_connection_close(newpcb, es);
		/* return memory error */
		ret_err = ERR_MEM;
	}
	return ret_err;
}


/**
  * @brief  This functions closes the tcp connection
  * @param  tcp_pcb: pointer on the tcp connection
  * @param  es: pointer on echo_state structure
  * @retval None
  */
void tcp_server_connection_close(struct tcp_pcb *tpcb, struct tcp_server_struct *es)
{
//#warning remove client from queue and check for NULL

	/* remove all callbacks */
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);

	/* delete es structure */
//	if (es != NULL)
//	{
//		mem_free(es);
//	}

	/* close tcp connection */
	tcp_close(tpcb);
	tcp_abort(tpcb);
	RemoveClientFromQueue(es);
}


/**
  * @brief  This function implements the tcp_err callback function (called
  *         when a fatal tcp_connection error occurs.
  * @param  arg: pointer on argument parameter
  * @param  err: not used
  * @retval None
  */
static void tcp_server_error(void *arg, err_t err)
{
	struct tcp_server_struct *es;

//#warning remove client from queue
	LWIP_UNUSED_ARG(err);

	es = (struct tcp_server_struct *) arg;
	Console_WriteLn("Tcp error");
//	RemoveClientFromQueue(es);
	tcp_server_connection_close(es->pcb, es);
//	if (es != NULL) {
//		/*  free es structure */
//		mem_free(es);
//	}
}


/**
  * @brief  This function implements the tcp_poll LwIP callback function
  * @param  arg: pointer on argument passed to callback
  * @param  tpcb: pointer on the tcp_pcb for the current tcp connection
  * @retval err_t: error code
  */
static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb)
{
	err_t ret_err;
	struct tcp_server_struct *es;

	es = (struct tcp_server_struct *) arg;
	if (es != NULL)
	{
		if (es->p_tx != NULL)
		{
//#warning is it needed here?
			tcp_sent(tpcb, tcp_server_sent);
			/* there is a remaining pbuf (chain) , try to send data */
			Tcp_NtripCaster_Send(tpcb, es);
		}
		else
		{
			/* no remaining pbuf (chain)  */
			if (es->state == ES_CLOSING)
			{
//#warning remove client from queue
//				RemoveClientFromQueue(es);
				/*  close tcp connection */
				Console_WriteLn("Closing in poll");
				tcp_server_connection_close(tpcb, es);
				return ERR_ABRT;
			}
		}
		ret_err = ERR_OK;
	}
	else
	{
		Console_WriteLn("es is NULL");
//#warning remove client from queue

		/* nothing to be done */
		tcp_abort(tpcb);
		ret_err = ERR_ABRT;
//#warning close tcp?
		RemoveClientFromQueue(es);
	}
	return ret_err;
}


/**
  * @brief  This function implements the tcp_sent LwIP callback (called when ACK
  *         is received from remote host for sent data)
  * @param  None
  * @retval None
  */
static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	struct tcp_server_struct *es;

	LWIP_UNUSED_ARG(len);

	es = (struct tcp_server_struct *)arg;
	es->retries = 0;

	if(es->p_tx != NULL)
	{
		/* still got pbufs to send */
//#warning is it needed here?
		tcp_sent(tpcb, tcp_server_sent);
		Tcp_NtripCaster_Send(tpcb, es);
	}
	else
	{
//#warning verify this code is necessary
		if(es->state == ES_SENDING)
			es->state = ES_READY;
		/* if no more data to send and client closed connection*/
		if(es->state == ES_CLOSING)
		{
			Console_WriteLn("Tcp closing in sent");
			tcp_server_connection_close(tpcb, es);
			return ERR_ABRT;
		}
	}
	return ERR_OK;
}


/**
  * @brief tcp_receiv callback
  * @param arg: argument to be passed to receive callback
  * @param tpcb: tcp connection control block
  * @param err: receive error code
  * @retval err_t: retuned error
  */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	struct tcp_server_struct *es;
	err_t ret_err;

	LWIP_ASSERT("arg != NULL", arg != NULL);

	es = (struct tcp_server_struct *)arg;

	/* if we receive an empty tcp frame from server => close connection */
	if (p == NULL)
	{
		/* remote host closed connection */
		es->state = ES_CLOSING;
		if(es->p_tx == NULL)
		{
//#warning remove client from queue here?
			/* we're done sending, close connection */
//			Console_WriteLn("Tcp closing 3");
			tcp_server_connection_close(tpcb, es);
			return ERR_ABRT;
		}
		else
		{
//#warning decided to acknowledge
			/* we're not done yet */
			/* acknowledge received packet */
			tcp_sent(tpcb, tcp_server_sent);

			/* send remaining data*/
			Tcp_NtripCaster_Send(tpcb, es);
		}
		ret_err = ERR_OK;
	}
  /* else : a non empty frame was received from echo server but for some reason err != ERR_OK */
	else if(err != ERR_OK)
	{
//#warning remove client from queue?
//		Console_WriteLn("not ERR_OK in recv");
//		RemoveClientFromQueue(es);
		/* free received pbuf*/
		if (p != NULL)
		{
			newPxCounter--;
			char msg[100];
			sprintf(msg, "recv 1 px: %d", newPxCounter);
			Console_WriteLn(msg);

			pbuf_free(p);
		}
		ret_err = err;
	}
	else if(es->state == ES_SENDING)
	{
		/* Acknowledge data reception */
		tcp_recved(tpcb, p->tot_len);

//		newPxCounter--;
//		char msg[100];
//		sprintf(msg, "recv 2 px: %d", newPxCounter);
//		Console_WriteLn(msg);
//		pbuf_free(p);
//		tcp_echoclient_connection_close(tpcb, es);
		ret_err = ERR_OK;
	}

	/* data received when connection already closed */
	else
	{
//#warning remove client from queue
//		RemoveClientFromQueue(es);
		/* Acknowledge data reception */
		tcp_recved(tpcb, p->tot_len);

//		newPxCounter--;
//		char msg[100];
//		sprintf(msg, "recv 3 px: %d", newPxCounter);
//		Console_WriteLn(msg);

		/* free pbuf and do nothing */
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	return ret_err;
}


/**
  * @brief function used to send data
  * @param  tpcb: tcp control block
  * @param  es: pointer on structure of type echoclient containing info on data
  *             to be sent
  * @retval None
  */
void Tcp_NtripCaster_Send(struct tcp_pcb *tpcb, struct tcp_server_struct * es)
{
	struct pbuf *ptr;
	err_t wr_err = ERR_OK;
	es->state = ES_SENDING;

	while ((wr_err == ERR_OK) &&
		 (es->p_tx != NULL) &&
		 (es->p_tx->len <= tcp_sndbuf(tpcb)))
	{
		/* get pointer on pbuf from es structure */
		ptr = es->p_tx;

		/* enqueue data for transmission */
		wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);

		if (wr_err == ERR_OK)
		{
//			u16_t plen = ptr->len;
			u8_t freed;

			/* continue with next pbuf in chain (if any) */
			es->p_tx = ptr->next;

			if(es->p_tx != NULL)
			{
				/* increment reference count for es->p */
				pbuf_ref(es->p_tx);
			}

//			/* free pbuf: will free pbufs up to es->p (because es->p has a reference count > 0) */
//			pbuf_free(ptr);

			/* chop first pbuf from chain */
			do
			{
				/* try hard to free pbuf */
				freed = pbuf_free(ptr);
			}
			while (freed == 0);

			newPxCounter--;
//			char msg[100];
//			sprintf(msg, "send px: %d", newPxCounter);
//			Console_WriteLn(msg);
//			/* we can read more data now */
//			tcp_recved(tpcb, plen);
		}
		else if(wr_err == ERR_MEM)
		{
			/* we are low on memory, try later, defer to poll */
			es->p_tx = ptr;
		}
		else
		{
			/* other problem ?? */
		}
	}
}
