/* ++++++++++++++
  3dfx_private.h

  Private data structures & definitions for 3dfx
  Voodoo Graphics cards
+++++++++++++++*/

typedef unsigned int FxU32;
typedef bool				 FxBool;

#define FXFALSE	false
#define FXTRUE  true

#define GET(s)       (*s)
#define SET(d, s)    ((*d) = s)
#define IGET(A)    sst1InitRead32((FxU32 *) &(A))
#define ISET(A,D)  sst1InitWrite32((FxU32 *) &(A), D)  
#define BIT(n)  (1UL<<(n))

#define SST_GRX_RESET           BIT(1)
#define SST_PCI_FIFO_RESET      BIT(2)
#define SST_VIDEO_RESET                 BIT(8)
#define SST_VIDEO_BLANK_EN              BIT(12)
#define SST_EN_DRAM_REFRESH             BIT(22)
#define SST_FBI_BUSY            BIT(7)
#define SST_EN_VGA_PASSTHRU     BIT(0)

#define P6FENCE __asm__ __volatile__ ("xchg %%eax, p6FenceVar": : :"%eax" );

