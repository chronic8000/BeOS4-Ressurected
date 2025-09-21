#ifndef _LINUX_SKBUFF_H
#define _LINUX_SKBUFF_H

#include <linux/mm.h>
#include <asm/types.h>

#define FREE_WRITE 0

struct sk_buff {
    struct sk_buff	*next;	    /* Next buffer in list 		*/
    struct device	*dev;	    /* Device we arrived on/are leaving by */
    unsigned long 	len;	    /* Length of actual data		*/
    unsigned short	protocol;   /* Packet protocol from driver. 	*/
    unsigned int	truesize;   /* Buffer size 			*/
    unsigned char	*head;	    /* Head of buffer 			*/
    unsigned char	*data;	    /* Data head pointer		*/
    unsigned char	*tail;	    /* Tail pointer			*/
    unsigned char 	*end;	    /* End pointer			*/
};
extern unsigned char *skb_put(struct sk_buff *skb, int len);
extern void skb_reserve(struct sk_buff *skb, int len);
extern struct sk_buff *dev_alloc_skb(unsigned size);

#endif
