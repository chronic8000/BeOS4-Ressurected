//******************************************************************************
//
//	File:		session.h
//
//	Description:	connection session.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	6/12/92		bgs	new today
//
//******************************************************************************


#ifndef	SESSION_H
#define	SESSION_H

#ifndef	_MESSAGES_H
#include "messages.h"
#endif
#ifndef	GR_TYPES_H
#include "gr_types.h"
#endif

#include "as_debug.h"
#include "BArray.h"

class SAtom;

//------------------------------------------------------------------------------

enum {
	SESSION_WRITING = 1,
	SESSION_READING
};

class TSessionClient {
	public:
	
		virtual void	SessionWillBlock()=0;
};

class TCPIPSocket;

// Stack-based memory buffer that will dynamically select between
// using the stack or heap.
class session_buffer {
public:
	inline void* reserve(size_t req_size)
	{
		used = req_size;
		if (req_size < sizeof(stack)) {
			if (buffer) {
				grFree(buffer);
				buffer = NULL;
			}
			return stack;
		}
		return (buffer = grRealloc(buffer, req_size, "session_buffer", MALLOC_MEDIUM));
	}
	
	inline size_t size() const				{ return used; }
	inline const void* retrieve() const		{ return buffer ? buffer : stack; }
	
	inline session_buffer()	: used(0), buffer(NULL)	{ }
	inline ~session_buffer()	{ if (buffer) grFree(buffer); }

private:
	session_buffer(const session_buffer&);
	session_buffer& operator=(const session_buffer&);
	
	uint32 stack[64];
	size_t used;
	void* buffer;
};

class TSession {
public:
		uint8	inMsg[MAX_MSG_SIZE];
		uint8	outMsg[MAX_MSG_SIZE];

		char	*message_buffer;
		long	cur_pos_send;
		long	cur_pos_receive;
		long	session_id;
		SAtom * m_atom;
		TSessionClient * m_client;

		long	send_port;
		long	receive_port;
		long	receive_size;
		long	session_team;
		TCPIPSocket *m_sock;
		int32	m_msgCode;
		#if SYNC_CALL_LOG
		int32	state;
		#endif
		

				TSession(long s_port, long r_port, long id);
				~TSession();

				#ifdef REMOTE_DISPLAY
				TSession(TCPIPSocket *socket, long id);
				#endif

		int32	send_buffer(bool timeout=false, int64 timeoutVal=0);
		int32	recv_buffer(bool timeout=false, int64 timeoutVal=0);

		int32	drain(int32 size);
		void	swrite(long size, const void *buffer);
		void	swrite(const char *);
		void	swrite(const session_buffer& buffer);
		void	swrite_l(long value);
		long	read_region(region *a_region);
		long	sread(long size, void *buffer, bool canKill=true);
		char	*sread();
		void	sread(long size, session_buffer* buffer);
		long	sread2(long size, void *buffer);
		void	add_to_buffer(void *);
		long	get_other(void *);
		long	avail_cnt();
		void	sync();
		void	sclose();
		long	avail();
		char	has_other();
		
		// a special write, to place data directly in the buffer.
		// note very well: be prepared to deal with a NULL return by
		// allocating your own buffer.
		void*	inplace_write(long size);
		
		void	SetAtom(SAtom *atom);
		void	SetClient(TSessionClient *client);
};

inline void	TSession::swrite_l(long value)
{
	swrite(4, &value);
};

int		bpcreate(int count);
void	bpdelete(int ref);

//------------------------------------------------------------------------------

#endif
