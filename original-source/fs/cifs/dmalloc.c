
// This has been altered to make smalloc and sfree calls.



/*
 * This file contains two routines that are wrappers (Yo! Bum rush the show)
 * for malloc and free.
 *
 * They protect against messing up memory, accessing bad memory, etc.
 *
 *   Dominic Giampaolo
 *   (nick@cs.wpi.edu)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#define _DMALLOC_C
#include "dmalloc.h"

/*
 * How much to pad at the beginning and end of each allocation.
 * It is the number of integers (usually 4 bytes) to use as
 * padding.
 *
 */
#define PADSIZE 2

typedef struct MemChunk
{
  struct MemChunk *prev;         /* points back towards the head of the list */
  struct MemChunk *next;         /* points forward into the list */
  int              size;
	int line;
	char file[64];
  int              pad[PADSIZE];
}MemChunk;

/* MemChunk dummy = { NULL, NULL, -1, { 0, 0 } }, */
static MemChunk  dummy = { NULL, NULL, -1, 0, 0 },
                *allocated_mem = &dummy;


int  max_allocated = 0;     /* access this as an extern if you want to know */


static int initialized = 0,
           cur_allocated = 0;

static int sem = -1;


static void
lck()
{
  if (sem <= 0)
	sem = create_sem(1, "");
  acquire_sem(sem);
}

static void
unlck()
{
  release_sem(sem);
}

void *dmalloc(unsigned int size, int line, const char *file)
{
  void *ptr;
  int  *tmp,i;
  int real_size;
  unsigned char *junk;
  MemChunk *mc;

  lck();

#ifdef FOO_BAR
  if (initialized == 0)   /* never been called before */
   {
     initialized = 1;     
     atexit(check_mem);
   }
#endif

  if (size > 1024*1024*1024)
   {
     dprintf("Bogus malloc for %d bytes.\n", size);
     return NULL;
   }

  real_size = size
              + (4 - (size % sizeof(int)))    /* roundup to a multiple of 4 */
              + PADSIZE*sizeof(int)           /* padding at the end */
              + sizeof(MemChunk);             /* chunk at the beginning */

  /*
   * The code below assumes that malloc returns memory that is of proper
   * alignment for integers.
   */
  //ptr = (void *)malloc(real_size);
	ptr = (void *)smalloc(real_size);
  if (ptr == NULL) {
	unlck();
    return NULL;
  }

  mc  = (MemChunk *)ptr;
  mc->size = size;
	mc->line = line;
	strcpy(mc->file, file);
  mc->next = allocated_mem;
  mc->prev = NULL;           /* because we're at the head of the list */   
  allocated_mem->prev = mc;
  allocated_mem = mc;
  for(i=0; i < PADSIZE; i++)
    mc->pad[i] = 0xDEADC0DE;

  
  /*
   * Now, if size is an odd amount, then we'll fill in those spare bytes
   * with junk to insure that they don't get munged either.
   */
  junk = (unsigned char *)((char *)mc + sizeof(MemChunk) + size);
  for(; ((int)junk % sizeof(int)) != 0; junk++)
    *junk = 0xec;

  tmp = (int *)junk;
  for(i=0; i < PADSIZE; i++, tmp++)
    *tmp = 0xDEADC0DE;


  cur_allocated += size;
  if (cur_allocated > max_allocated)
    max_allocated = cur_allocated;

  /*panic("malloc %d @ 0x%x\n", size, (void *)((int)ptr+sizeof(MemChunk)));*/

  unlck();
  return (void *)((int)ptr + sizeof(MemChunk));
}


void *dcalloc(unsigned int nelem, unsigned int size, int line, const char *file)
{
  void *mem;

  mem = dmalloc(nelem*size, line, file);
  if (mem)
    memset(mem, '\0', nelem*size);

  return mem;
}



void *drealloc(void *ptr, unsigned int size, int line, const char *file)
{
  void *new;
  MemChunk *mc;

  new = dmalloc(size, line, file);
  if (new == NULL)
    return NULL;

	if (ptr != NULL) {
	  mc = (MemChunk *)((char *)ptr - sizeof(MemChunk));
	
	  size = (size < mc->size) ? size : mc->size;
	  memcpy(new, ptr, size);

	  dfree(ptr, 0, NULL);
  	}

  return new;
}



void dcfree(void *mem)
{
  dfree(mem, 0, NULL);
}


#define FREE_PTR1  ((MemChunk *)0xf00dface)
#define FREE_PTR2  ((MemChunk *)0xbeefbabe)

static void	*purg[1024];
static int pind = 0;

void dfree(void *mem, int line, const char *file)
{
  int *tmp,i, len_to_dump = 128, dumped;
  unsigned char *junk;
  MemChunk *mc;
  
  lck();

  mc = (MemChunk *)((int)mem - sizeof(MemChunk));
  if (mem == NULL || mc->next == FREE_PTR1 || mc->prev == FREE_PTR2)
   {
     dprintf("Potential free twice error at 0x%.8x in file %s line %d\n",
	     (unsigned int)mem, file, line);
     return;
   }

  /* panic("free %d @ 0x%x \n", mc->size, mem); */

  if (mc->size <= 0 || mc->size > (256*1024*1024))
   {
     dprintf("Potentially Bogus free at 0x%.8x, size is %d\n",
	     (unsigned int)mem, mc->size);

     if (mc->size < len_to_dump)
       len_to_dump = mc->size;
#ifdef USER
     fflush(stdout);
#endif

     /*
      * What can we do at this point?  If we continue we'll probably
      * crash, if just return, we aren't freeing memory that should be.
      * For now, just punt.
      */
     return;
   }


  /*
   * Now we check the padding to make sure it wasn't messed up.
   */
  dumped = 0;
  for(i=0; i < PADSIZE; i++)
    if (mc->pad[i] != 0xDEADC0DE)
     {
       dprintf("MEMORY MUNGED1: loc. 0x%.8x size %d\n",
	       (unsigned int)mem, mc->size);
       dprintf("\t%d bytes BEFORE start of allocation modified.\n",
	       PADSIZE - i*sizeof(int));

       if (mc->size < len_to_dump)
	 len_to_dump = mc->size;
       if (dumped++ == 0) {
#ifdef USER
	   fflush(stdout);
#endif
       }

       mc->pad[i] = 0xDEADC0DE;   /* set it back to what it should be */
     }


  junk = (unsigned char *)((char *)mc + sizeof(MemChunk) + mc->size);
  for(; ((int)junk % sizeof(int)) != 0; junk++)
    if ((int)*junk != 0xec)
     {
       dprintf("MEMORY MUNGED2: loc. 0x%.8x size %d file %s line %d\n", (int)mem,
	       mc->size, mc->file, mc->line);
       dprintf("\tbyte %d AFTER end of allocation modified.\n",
	       i+1);

       len_to_dump = (PADSIZE * sizeof(int)) + ((int)junk % sizeof(int));
       if (dumped++ == 0) {
#ifdef USER
	   fflush(stdout);
#endif 
       }
       
       *junk = 0xec;        /* set it back to what it should be */
     }

  tmp = (int *)junk;
  for(i=0; i < PADSIZE; i++, tmp++)
    if (*tmp != 0xDEADC0DE)
     {
       dprintf("MEMORY MUNGED3: loc. 0x%.8x size %d file %s line %d\n", (int)mem,
	       mc->size, mc->file, mc->line);
       dprintf("\tbyte %d AFTER end of allocation modified.\n",
	       i*sizeof(int));

       len_to_dump = (PADSIZE * sizeof(int)) + ((int)junk % sizeof(int));
       if (dumped++ == 0) {
#ifdef USER
	   fflush(stdout);
#endif
       }

       *tmp = 0xDEADC0DE;   /* set it back to what it should be */
     }


  /*
   * Now go and munge all the freed memory to insure that if it is
   * used again it will cause errors.
   */
  for(tmp=(int *)mem,i=0; i < mc->size/sizeof(int); i++, tmp++)
    *tmp = 0xC0DEBABE;


  /*
   * Now we have to unlink it from our list.
   */
  if (mc->prev)
   {
     mc->prev->next = mc->next;
   }
  if (mc->next)
   {
     mc->next->prev = mc->prev;
   }
  if (allocated_mem == mc)
    allocated_mem = mc->next;
  if (allocated_mem == NULL)
    dprintf("BUG: allocated_mem == NULL\n");

  cur_allocated -= mc->size;
  if (cur_allocated < 0)
   {
     dprintf("More memory freed than allocated (loc == 0x%.8x).\n",
	     (unsigned int)mem);
   }

  mc->next = FREE_PTR1;
  mc->prev = FREE_PTR2;
  mc->size = -1;

  if (purg[pind]) {
	//free(purg[pind]);
	sfree(purg[pind]);
	}
  purg[pind] = mc;
  pind = (pind + 1) % (sizeof(purg) / sizeof(void*));

  unlck();
}

void check_mem(void)
{
  int len;
  MemChunk *mc;
  
  if (allocated_mem == &dummy && cur_allocated == 0) { /* everything is ok */
	dprintf("allocated memory checked out\n");
	return;
  }  
  if (cur_allocated)
    dprintf("%d bytes not freed.\n", cur_allocated);

  for(mc = allocated_mem; mc != &dummy && mc != NULL; mc = mc->next)
   {
     dprintf("Unfreed memory at: 0x%.8x size == %d %s %d\n",
	     (int)((char *)mc + sizeof(MemChunk)), mc->size,
			mc->file, mc->line);

     if (mc->size < 128)
       len = mc->size;
     else
       len = 128;
#ifdef USER
     fflush(stdout);
#endif
   }
  
}



char *dstrdup(char *str, int line, const char *file)
{
  char *new;

  new = dmalloc(strlen(str)+1, line, file);
  if (new)
    strcpy(new, str);

  return new;
}
