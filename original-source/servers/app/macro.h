#ifndef	MACRO_H
#define	MACRO_H

#define newstruct(x)				(struct x *)(gr_malloc((unsigned)sizeof(struct x)))
#define newtype(x)					(x *)(gr_malloc((unsigned)sizeof(x)))
#define vabs(a)						(((a)<0) ? -(a) : (a))
#define sgn(a)						(((a)<0) ? -1 : (a)>0 ? 1 : 0)
#define sign(a)						(((a)<0) ? -1 : 1)
#define print_rect(r)				printf("%ld, %ld, %ld, %ld\n",r.left,r.top,r.right,r.bottom);
#define	hs(r)						(r.right - r.left)
#define	vs(r)						(r.bottom - r.top)
#undef max
#undef min
#define min(a,b) 					(((a)>(b))?(b):(a))
#define max(a,b) 					(((a)<(b))?(b):(a))

#endif
