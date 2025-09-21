#include <stdio.h>
#include "common/kbq.h"
#include "common/kb.c"


int main()
{
	int i;
	kbuf_t *b;
	kbq_t q, p;
	
	
	printf("################ Testing kbuf_t\n\n");
	b = create_kb(NULL,500,NULL);
	kb_reserve(
	
	printf("################ Testing kbq_t\n\n");
	init_kbq(&q,"Test Q");
	printf_kbq(&q);
	
	printf("\npush_front"); for (i=0; i<3; ++i)
	{ b = create_kb(NULL,0,NULL); printf(" %p",b); kbq_push_front(&q,b); }
	putchar('\n'); printf_kbq(&q);
	
	printf("\npop_front"); for (i=0; i<4; ++i)
	{ b = kbq_pop_front(&q); printf(" %p",b); delete_kb(b); }
	putchar('\n');	printf_kbq(&q);
	
	printf("\npush_front"); for (i=0; i<3; ++i)
	{ b = create_kb(NULL,0,NULL); printf(" %p",b); kbq_push_front(&q,b); }
	putchar('\n');	printf_kbq(&q);
	
	printf("\npop_back");	for (i=0; i<4; ++i)
	{ b = kbq_pop_back(&q); printf(" %p",b); delete_kb(b); }
	putchar('\n');	printf_kbq(&q);
	
	printf("\npush_back"); for (i=0; i<3; ++i)
	{ b = create_kb(NULL,0,NULL); printf(" %p",b); kbq_push_back(&q,b); }
	putchar('\n'); printf_kbq(&q);
	
	printf("\npop_back"); for (i=0; i<4; ++i)
	{ b = kbq_pop_back(&q); printf(" %p",b); delete_kb(b); }
	putchar('\n'); printf_kbq(&q);
	
	printf("\npush_back"); for (i=0; i<3; ++i)
	{ b = create_kb(NULL,0,NULL); printf(" %p",b); kbq_push_back(&q,b); }
	putchar('\n'); printf_kbq(&q);
	
	printf("\npurge\n");
	purge_kbq(&q);
	printf_kbq(&q);
	
	init_kbq(&p,"Test P");
	
	printf("\nP->Q (empty->empty) front\n");
	kbq_pushall_front(&p,&q);
	printf_kbq(&q);
	
	printf("\npush_back (p)"); for (i=0; i<3; ++i)
	{ b = create_kb(NULL,0,NULL); printf(" %p",b); kbq_push_back(&p,b); }
	printf("\nP "); printf_kbq(&p);
	
	printf("\nP->Q (full->empty) front\n");
	kbq_pushall_front(&p,&q);
	printf_kbq(&q);
	
	printf("\nP->Q (empty->full) front\n");
	kbq_pushall_front(&p,&q);
	printf_kbq(&q);
	
	printf("\npush_back (p)"); for (i=0; i<3; ++i)
	{ b = create_kb(NULL,0,NULL); printf(" %p",b); kbq_push_back(&p,b); }
	printf("\nP "); printf_kbq(&p);
	
	printf("\nP->Q (full->full) front\n");
	kbq_pushall_front(&p,&q);
	printf_kbq(&q);


	printf("\npurge\n");
	purge_kbq(&q);
	printf_kbq(&q);
	
	printf("\nP->Q (empty->empty) back\n");
	kbq_pushall_back(&p,&q);
	printf_kbq(&q);
	
	printf("\npush_back (p)"); for (i=0; i<3; ++i)
	{ b = create_kb(NULL,0,NULL); printf(" %p",b); kbq_push_back(&p,b); }
	printf("\nP "); printf_kbq(&p);
	
	printf("\nP->Q (full->empty) back\n");
	kbq_pushall_back(&p,&q);
	printf_kbq(&q);
	
	printf("\nP->Q (empty->full) back\n");
	kbq_pushall_back(&p,&q);
	printf_kbq(&q);
	
	printf("\npush_back (p)"); for (i=0; i<3; ++i)
	{ b = create_kb(NULL,0,NULL); printf(" %p",b); kbq_push_back(&p,b); }
	printf("\nP "); printf_kbq(&p);
	
	printf("\nP->Q (full->full) back\n");
	kbq_pushall_back(&p,&q);
	printf_kbq(&q);
}