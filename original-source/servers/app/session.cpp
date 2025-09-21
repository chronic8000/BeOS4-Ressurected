//******************************************************************************
//
//	File:		session.cpp
//
//	Description:	TSession class.
//			connection session control.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	6/11/92		bgs	new today
//
//******************************************************************************
 
#include <OS.h>

#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif
#ifndef	SESSION_H
#include "session.h"
#endif
#ifndef	PROTO_H
#include "proto.h"
#endif
#ifndef _STRING_H
#include <string.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif

#include "atom.h"
#include "as_support.h"
#include "system.h"

#ifdef REMOTE_DISPLAY
#include "TCPIPSocket.h"
#endif

#if SYNC_CALL_LOG
CallTree synchronous("sync");
#endif

#if PROTOCOL_BANDWIDTH_LOG
CallTree rcvBandwidth("rcv");
CallTree sndBandwidth("snd");
#endif

/*---------------------------------------------------------------*/

inline void memcpy2(void *dst, const void *src, int32 count)
{
	uint32 *out = (uint32*)src;
	uint32 *in = (uint32*)dst;
	while (count >= 4) {
		*in++ = *out++;
		count-=4;
	};
	if (count > 0) {
		*((uint8*)in) = *((uint8*)out);
		if (count > 1) {
			*(((uint8*)in)+1) = *(((uint8*)out)+1);
			if (count > 2) {
				*(((uint8*)in)+2) = *(((uint8*)out)+2);
			};
		};
	};
};

#ifdef REMOTE_DISPLAY
TSession::TSession(TCPIPSocket *socket, long id)
{
	session_id = id;
	cur_pos_send = 4;
	cur_pos_receive = MAX_MSG_SIZE;
	receive_size = MAX_MSG_SIZE;
	send_port = -1;
	session_team = -1;
	receive_port = -1;
	message_buffer = 0;
	m_atom = NULL;
	m_client = NULL;
	m_sock = socket;
	m_sock->SetBlocking(false);
	m_sock->SetBlocking(true);
	#if SYNC_CALL_LOG
	state = 0;
	#endif
};
#endif

TSession::TSession(long s_port, long r_port, long id)
{
	session_id = id;
	cur_pos_send = 4;
	cur_pos_receive = MAX_MSG_SIZE;
	receive_size = MAX_MSG_SIZE;
	send_port = s_port;
	session_team = -1;
	receive_port = r_port;
	message_buffer = 0;
	m_atom = NULL;
	m_client = NULL;
	m_sock = NULL;
	#if SYNC_CALL_LOG
	state = 0;
	#endif
}

TSession::~TSession()
{
	#ifdef REMOTE_DISPLAY
	if (m_sock) {
		m_sock->Close();
		delete m_sock;
	};
	#endif
};

void TSession::SetAtom(SAtom *atom)
{
	m_atom = atom;
};

void TSession::SetClient(TSessionClient *client)
{
	m_client = client;
};
	
/*-------------------------------------------------------------*/

long	TSession::avail_cnt()
{
	return(receive_size - cur_pos_receive);
}

int32 
TSession::send_buffer(bool timeout, int64 timeoutVal)
{
	#ifndef REMOTE_DISPLAY
		return write_port(send_port, session_id, outMsg, *((int32*)outMsg));
	#else
		int32 r;
		if (!m_sock) return write_port(send_port, session_id, outMsg, *((int32*)outMsg));
		m_sock->SetBlocking(true);
		r = m_sock->Send(outMsg+4,*((int32*)outMsg)-4);
		return r;
	#endif
}

int32 
TSession::recv_buffer(bool timeout, int64 timeoutVal)
{
	#ifndef REMOTE_DISPLAY
		return read_port_etc(receive_port, &m_msgCode, inMsg, MAX_MSG_SIZE, timeout?B_TIMEOUT:0, timeoutVal);
	#else
		if (!m_sock) return read_port_etc(receive_port, &m_msgCode, inMsg, MAX_MSG_SIZE, timeout?B_TIMEOUT:0, timeoutVal);
	
		int32 r;
		int32 bufLen = MAX_MSG_SIZE-4;
		m_sock->SetBlocking(!timeout);
		r = m_sock->Receive(inMsg+4,&bufLen);
		if (r == /*B_WOULD_BLOCK*/-1) {
			if (timeoutVal != 0) {
				snooze(timeoutVal);
				r = m_sock->Receive(inMsg+4,&bufLen);
			};
			if (r == -1) r = B_WOULD_BLOCK;
		};
		*((int32*)inMsg) = bufLen+4;
		if ((bufLen == 0) && (r == 0)) r = B_BAD_VALUE;
		m_msgCode = session_id;
		return r;
	#endif
}

/*-------------------------------------------------------------*/
// this one is special since it will exit if the next message
// coming from the port is not for the current session but
// is a generic message.
// trust me.

long	TSession::sread2(long size, void *buffer)
{
	short	size0;
	short	avail;
	char	started = 0;
	int32	r;

	#if SYNC_CALL_LOG
	state = SESSION_READING;
	#endif

	#if PROTOCOL_BANDWIDTH_LOG
	CallStack stack;
	stack.Update(1);
	rcvBandwidth.AddToTree(&stack,size);
	#endif

	while (size > 0) {
		if (cur_pos_receive == receive_size) {
			do {
				r = recv_buffer(true,0);

				if (r == B_WOULD_BLOCK) {
					if (m_client) m_client->SessionWillBlock();
					if (m_atom && (m_atom->ServerUnlock() == 2)) {
						/*	This means that the window is no longer referenced by
							anyone except me.  This is perfectly acceptable.  We'll
							just finish the window destruction here and kill ourselves. */
						m_atom->ServerUnlock();
						suicide(0);
					};
					r = recv_buffer();
					if (m_atom && (m_atom->ServerLock() == 1)) {
						/*	The only time this will happen is if the window has been closed
							between our returning from read_port and doing the ServerLock(),
							because if the window is closed while we're blocked the port will be
							deleted and we'll get an error and return in the line above.  Anyway,
							we're sure now that we have the only references to the window,
							so delete it and die. */
						m_atom->ServerUnlock();
						m_atom->ServerUnlock();
						suicide(0);
					};
				};

				if (r < 0) return -2;

				if (m_msgCode != session_id) {
					add_to_buffer(inMsg);
					if (!started) return(-1);
				}

			} while(m_msgCode != session_id);

			cur_pos_receive = 4;
			receive_size = *((uint32*)inMsg);
		}
		avail = receive_size - cur_pos_receive;
		
		if (size < avail)
			size0 = size;
		else
			size0 = avail;

		started = 1;

		memcpy2(buffer, inMsg + cur_pos_receive, size0);
		
		size -= size0;
		buffer = (void *)((long)buffer + size0);
		cur_pos_receive += size0;
	}
	return(0);
}			

/*-------------------------------------------------------------*/

long	TSession::read_region(region *a_region)
{
	rect	tmp_rect;
	long	count;
	long	error;

	error = sread(4, &count);
	if (error < 0) {
		a_region->MakeEmpty();
		return(error);
	}

	rect* data = a_region->CreateRects(count, count);
	a_region->SetRectCount(count);
	if (!data)
		return B_NO_MEMORY;
	
	sread(16, &(a_region->Bounds()));
	while(count > 0) {
		count--;
		error = sread(16, data++);
		if (error < 0) {
			a_region->MakeEmpty();
			return(error);
		}
		
	}
	return(0);
}

/*-------------------------------------------------------------*/

char	*TSession::sread()
{
	char	*buffer;
	short	len;
	
	sread(2, &len);
	buffer = (char *)grMalloc(len,"sread str",MALLOC_CANNOT_FAIL);
	sread(len, buffer);
	return(buffer);
}

/*-------------------------------------------------------------*/

long	TSession::avail()
{
	return(receive_size - cur_pos_receive);
}

/*-------------------------------------------------------------*/

int32 TSession::drain(int32 size)
{
	int32 r=0,s;
	char buf[256];
	while (size) {
		s = 256;
		if (size < s) s = size;
		r = sread(s,buf);
		if (r < 0) return r;
		size -= s;
	};
	return r;
}

/*-------------------------------------------------------------*/

void	TSession::sread(long size, session_buffer* buffer)
{
	void* buf = buffer->reserve(size);
	if (buf) sread(size, buf);
	else {
		buffer->reserve(0);
		drain(size);
	}
}

/*-------------------------------------------------------------*/

long	TSession::sread(long size, void *buffer, bool canKill)
{
	long	r;
	char	*tmp;
	short	size0;
	short	avail;

	#if SYNC_CALL_LOG
	state = SESSION_READING;
	#endif

	#if PROTOCOL_BANDWIDTH_LOG
	CallStack stack;
	stack.Update(1);
	rcvBandwidth.AddToTree(&stack,size);
	#endif

	avail = receive_size - cur_pos_receive;

	if (size <= avail) {
		memcpy2(buffer, inMsg + cur_pos_receive, size);
		cur_pos_receive += size;
		return(0);
	}

	while (size > 0) {
		if (cur_pos_receive == receive_size) {
			do {
				r = recv_buffer(true,0);

				if (r == B_WOULD_BLOCK) {
					if (m_client) m_client->SessionWillBlock();
					if (canKill && m_atom && (m_atom->ServerUnlock() == 2)) {
						/*	This means that the window is no longer referenced by
							anyone except me and I've been given permission to die if
							such is the case.  So, die, after deleting the window. */
						m_atom->ServerUnlock();
						suicide(0);
					};
					r = recv_buffer();
					if (canKill && m_atom && (m_atom->ServerLock() == 1)) {
						/*	The only time this will happen is if the window has been closed
							between our returning from read_port and doing the ServerLock(),
							because if the window is closed while we're blocked the port will be
							deleted and we'll get an error and return in the line above.  Anyway,
							we're sure now that we have the only references to the window,
							so delete it and die. */
						m_atom->ServerUnlock();
						m_atom->ServerUnlock();
						suicide(0);
					};
				};

				if (r < 0) {
					if (!canKill) return r;
					if (m_client) m_client->SessionWillBlock();
					if (m_atom) {
						m_atom->ServerUnlock();
						m_atom->ServerUnlock();
						suicide(0);
					};
					return r;
				};
				
				if (m_msgCode != session_id)
					add_to_buffer(inMsg);

			} while(m_msgCode != session_id);

			cur_pos_receive = 4;
			receive_size = *((uint32*)inMsg);
		}
		avail = receive_size - cur_pos_receive;
		
		if (size < avail)
			size0 = size;
		else
			size0 = avail;

		memcpy2(buffer, inMsg + cur_pos_receive, size0);
		
		size -= size0;
		buffer = (void *)((long)buffer + size0);
		cur_pos_receive += size0;
	}
	return B_OK;
}			

/*-------------------------------------------------------------*/

void	TSession::swrite(const session_buffer& buffer)
{
	swrite(buffer.size(), buffer.retrieve());
}

/*-------------------------------------------------------------*/

void	TSession::swrite(long size, const void *buffer)
{
	short	free_space,size0,result;

	#if SYNC_CALL_LOG
	if (state == SESSION_READING) {
		CallStack stack;
		stack.Update(1);
		synchronous.AddToTree(&stack);
	};
	state = SESSION_WRITING;
	#endif

	#if PROTOCOL_BANDWIDTH_LOG
	CallStack stack;
	stack.Update(1);
	sndBandwidth.AddToTree(&stack,size);
	#endif

	while(size > 0) {
		free_space = MAX_MSG_SIZE - cur_pos_send;

		if (size < free_space)
			size0 = size;	
		else
			size0 = free_space;

		memcpy2(outMsg + cur_pos_send, buffer, size0);
		cur_pos_send += size0;
		buffer = (const void *)((long)buffer + size0);
		size -= size0;

		if (cur_pos_send == MAX_MSG_SIZE) {
			*((uint32*)outMsg) = cur_pos_send;
			if (m_client) m_client->SessionWillBlock();
			result = send_buffer();
			cur_pos_send = 4;
		}
	}
}

/*-------------------------------------------------------------*/

void	TSession::swrite(const char *a_string)
{
	short	len;

	len = strlen(a_string) + 1;
	swrite(2, &len);
	swrite(len, (void*)a_string);
}

/*-------------------------------------------------------------*/

void*	TSession::inplace_write(long size)
{
	if (size > MAX_MSG_SIZE)
		return NULL;
	
	#if SYNC_CALL_LOG
	if (state == SESSION_READING) {
		CallStack stack;
		stack.Update(1);
		synchronous.AddToTree(&stack);
	};
	state = SESSION_WRITING;
	#endif

	#if PROTOCOL_BANDWIDTH_LOG
	CallStack stack;
	stack.Update(1);
	sndBandwidth.AddToTree(&stack,size);
	#endif

	if (size > (MAX_MSG_SIZE - cur_pos_send))
		sync();
	
	void* pos = outMsg + cur_pos_send;
	cur_pos_send += size;
	return pos;
}

/*-------------------------------------------------------------*/

void	TSession::sync()
{
	short	result;
	short	i;

	if (cur_pos_send > 4) {
		*((uint32*)outMsg) = cur_pos_send;
		if (m_client) m_client->SessionWillBlock();
		result = send_buffer();
		cur_pos_send = 4;
	}
}

/*-------------------------------------------------------------*/

void	TSession::sclose()
{
	sync();
	delete_port(receive_port);
}

/*-------------------------------------------------------------*/

void	TSession::add_to_buffer(void *a_message)
{
	char	*new_one;
	char	*tmp;

	new_one = (char *)grMalloc(sizeof(message)+4,"session msg",MALLOC_MEDIUM);
	if (!new_one) return;
	memcpy2(new_one + 4, (char *)a_message, sizeof(message));
	*((long *)new_one) = 0;

	tmp = message_buffer;

	if (tmp == 0) {
		message_buffer = new_one;
		return;
	}
	else {
		while (*((long *)tmp)) {
			tmp = (char *) *((long *)tmp);
		}
		*((long *)tmp) = (long)new_one;
	}
}

/*-------------------------------------------------------------*/

char	TSession::has_other()
{
	if (message_buffer != 0)
		return(1);
	else
		return(0);
}

/*-------------------------------------------------------------*/

long	TSession::get_other(void *a_message)
{
	char	*tmp;

	if (message_buffer == 0) return(-1);
	
	memcpy2(a_message, message_buffer + 4, sizeof(message));
	
	tmp = message_buffer;
	message_buffer =  (char *) *((long *)tmp);
	grFree(tmp);
	return(0);
}

/*---------------------------------------------------------------*/

int	bpcreate(int count)
{
	int	ref;
	ref = create_port(count,"Bpcreate");
	return(ref);
}

/*---------------------------------------------------------------*/

void bpdelete(int ref)
{
	delete_port(ref);
}

/*---------------------------------------------------------------*/
