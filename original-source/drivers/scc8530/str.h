

#define	EOF	(-1)

struct str {
	uchar	*buffer;
	uint	bufsize,
		head,
		tail;
};

bool	salloc( struct str *, uint),
	sfree( struct str *),
	sputb( struct str *, uint);
uint	scount( struct str *),
	sgetb( struct str *),
	sunputb( struct str *);
uchar	*sgetm( struct str *, uint);
void	sungetm( struct str *, uint),
	sclear( struct str *);
