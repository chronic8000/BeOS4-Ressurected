#include	<sys/types.h>
#include	<sys/file.h>
#include    <unistd.h>
#include	"btconf.h"
#include	"btree.h"

/*
         (C) Copyright, 1988, 1989 Marcus J. Ranum
                    All rights reserved


          This software, its documentation,  and  supporting
          files  are  copyrighted  material  and may only be
          distributed in accordance with the terms listed in
          the COPYRIGHT document.

*/

int
bt_wlabel(BT_INDEX *b, bt_chrp buf, int siz)
{
	size_t len = siz;
	
	if(siz > BT_LABSIZ(b)) {
		bt_errno(b) = BT_LTOOBIG;
		return(BT_ERR);
	}

	if(write_data_stream(bfs_info(b), sizeof(struct bt_super),
						 (char *)buf, &len) != 0)
		return(BT_ERR);
	bt_clearerr(b);
	return(BT_OK);
}

int
bt_rlabel(BT_INDEX *b, bt_chrp buf, int siz)
{
	size_t len = BT_LABSIZ(b); 
	
	if(siz < BT_LABSIZ(b)) {
		bt_errno(b) = BT_BTOOSMALL;
		return(BT_ERR);
	}

	if(read_data_stream(bfs_info(b), sizeof(struct bt_super),
						(char *)buf, &len) != 0)
			return(BT_ERR);
	bt_clearerr(b);
	return(BT_OK);
}
