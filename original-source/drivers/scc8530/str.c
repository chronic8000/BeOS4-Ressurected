

#include	"rico.h"
#include	"str.h"


void	*malloc( );


bool
salloc( struct str *s, uint n)
{

	s->bufsize = n + 1;
	s->buffer = malloc( s->bufsize);
	s->head = 0;
	s->tail = 0;
	return (s->buffer != 0);
}


bool
sfree( struct str *s)
{

	if (s->buffer) {
		free( s->buffer);
		s->buffer = 0;
	}
}


bool
sputb( struct str *s, uint c)
{

	uint h = (s->head+1) % s->bufsize;
	if ((not s->buffer)
	or (h == s->tail))
		return (FALSE);
	s->buffer[s->head] = c;
	s->head = h;
	return (TRUE);
}


uint
scount( struct str *s)
{

	unless (s->head < s->tail)
		return (s->head - s->tail);
	return (s->bufsize-s->tail + s->head);
}


uint
sgetb( struct str *s)
{
	uint	c;

	if ((not s->buffer)
	or (s->tail == s->head))
		return (EOF);
	c = s->buffer[s->tail];
	s->tail = (s->tail+1) % s->bufsize;
	return (c);
}


uchar	*
sgetm( struct str *s, uint n)
{
	uchar	*p,
		temp[1024];

	unless ((s->buffer)
	and (s->bufsize < sizeof( temp)))
		return (0);
	if (s->head < s->tail) {
		uint i = s->bufsize - s->tail;
		memcpy( temp, s->buffer, s->head);
		memmove( s->buffer, s->buffer+s->tail, i);
		memcpy( s->buffer+i, temp, s->head);
		s->head += i;
		s->tail = 0;
	}
	p = s->buffer + s->tail;
	s->tail = (s->tail+n) % s->bufsize;
	return (p);
}


void
sungetm( struct str *s, uint n)
{

	s->tail = (s->tail-n) % s->bufsize;
}


uint
sunputb( struct str *s)
{

	if ((not s->buffer)
	or (s->tail == s->head))
		return (EOF);
	s->head = (s->head-1) % s->bufsize;
	return (s->buffer[s->head]);
}


void
sclear( struct str *s)
{

	s->head = 0;
	s->tail = 0;
}
