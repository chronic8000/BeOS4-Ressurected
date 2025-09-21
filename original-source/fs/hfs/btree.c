/*
 * hfsutils - tools for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996, 1997 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include <stdlib.h>
# include <string.h>
# include <errno.h>

# include "internal.h"
# include "data.h"
# include "block.h"
# include "file.h"
# include "btree.h"
# include "node.h"

/*
 * NAME:	btree->getnode()
 * DESCRIPTION:	retrieve a numbered node from a B*-tree file
 */
int bt_getnode(node *np)
{
  btree *bt = np->bt;
  block *bp = &np->data;
  unsigned char *ptr;
  int i;

  /* verify the node exists and is marked as in-use */

  if (np->nnum < 0 || (np->nnum > 0 && np->nnum >= bt->hdr.bthNNodes))
    {
      ERROR(EIO, "read nonexistent b*-tree node");
      return -1;
	}

  if (bt->map && (!BMTST(bt->map, np->nnum)))
    {
      ERROR(EIO, "read unallocated b*-tree node; disk is corrupt (not my fault)\n");
	  /* return -1; */
    }

  if (f_getblock(&bt->f, np->nnum, bp) < 0)
    return -1;

  ptr = *bp;

  d_fetchl(&ptr, (long *) &np->nd.ndFLink);
  d_fetchl(&ptr, (long *) &np->nd.ndBLink);
  d_fetchb(&ptr, (char *) &np->nd.ndType);
  d_fetchb(&ptr, (char *) &np->nd.ndNHeight);
  d_fetchw(&ptr, (short *) &np->nd.ndNRecs);
  d_fetchw(&ptr, &np->nd.ndResv2);

  if (np->nd.ndNRecs > HFS_MAXRECS)
    {
      ERROR(EIO, "too many b*-tree node records");
      return -1;
    }

  i = np->nd.ndNRecs + 1;

  ptr = *bp + bt->f.vol->blockSize - (2 * i);

  while (i--)
    d_fetchw(&ptr, (short *) &np->roff[i]);

  return 0;
}

/*
 * NAME:	btree->putnode()
 * DESCRIPTION:	store a numbered node into a B*-tree file
 */
int bt_putnode(node *np)
{
  btree *bt = np->bt;
  block *bp = &np->data;
  unsigned char *ptr;
  int i;

  /* verify the node exists and is marked as in-use */

  if (np->nnum && np->nnum >= bt->hdr.bthNNodes)
    {
      ERROR(EIO, "write nonexistent b*-tree node");
      return -1;
    }
  else if (bt->map && ! BMTST(bt->map, np->nnum))
    {
      ERROR(EIO, "write unallocated b*-tree node");
      return -1;
    }

  ptr = *bp;

  d_storel(&ptr, np->nd.ndFLink);
  d_storel(&ptr, np->nd.ndBLink);
  d_storeb(&ptr, np->nd.ndType);
  d_storeb(&ptr, np->nd.ndNHeight);
  d_storew(&ptr, np->nd.ndNRecs);
  d_storew(&ptr, np->nd.ndResv2);

  if (np->nd.ndNRecs > HFS_MAXRECS)
    {
      ERROR(EIO, "too many b*-tree node records");
      return -1;
    }

  i = np->nd.ndNRecs + 1;

  ptr = *bp + bt->f.vol->blockSize - (2 * i);

  while (i--)
    d_storew(&ptr, np->roff[i]);

  if (f_putblock(&bt->f, np->nnum, bp) < 0)
    return -1;

  return 0;
}

/*
 * NAME:	btree->readhdr()
 * DESCRIPTION:	read the header node of a B*-tree
 */
int bt_readhdr(btree *bt)
{
  unsigned char *ptr;
  char *map;
  int i;
  unsigned long nnum;

  bt->hdrnd.bt   = bt;
  bt->hdrnd.nnum = 0;

  if (bt_getnode(&bt->hdrnd) < 0)
    return -1;

  if (bt->hdrnd.nd.ndType != ndHdrNode ||
      bt->hdrnd.nd.ndNRecs != 3 ||
      bt->hdrnd.roff[0] != 0x00e ||
      bt->hdrnd.roff[1] != 0x078 ||
      bt->hdrnd.roff[2] != 0x0f8 ||
      bt->hdrnd.roff[3] != 0x1f8)
    {
      ERROR(EIO, "malformed b*-tree header node");
      return -1;
    }

  /* read header record */

  ptr = HFS_NODEREC(bt->hdrnd, 0);

  d_fetchw(&ptr, (short *) &bt->hdr.bthDepth);
  d_fetchl(&ptr, (long *) &bt->hdr.bthRoot);
  d_fetchl(&ptr, (long *) &bt->hdr.bthNRecs);
  d_fetchl(&ptr, (long *) &bt->hdr.bthFNode);
  d_fetchl(&ptr, (long *) &bt->hdr.bthLNode);
  d_fetchw(&ptr, (short *) &bt->hdr.bthNodeSize);
  d_fetchw(&ptr, (short *) &bt->hdr.bthKeyLen);
  d_fetchl(&ptr, (long *) &bt->hdr.bthNNodes);
  d_fetchl(&ptr, (long *) &bt->hdr.bthFree);

  /*
  printf("hfs: btree(0x%08x) hdr read, has %d nodes, %d recs\n",
		 bt,bt->hdr.bthNNodes,bt->hdr.bthNRecs);
		 */
  
  for (i = 0; i < 76; ++i)
    d_fetchb(&ptr, (char *) &bt->hdr.bthResv[i]);

  if (bt->hdr.bthNodeSize != bt->f.vol->blockSize)
    {
      ERROR(EINVAL, "unsupported b*-tree node size");
      return -1;
    }

  /* read map record; construct btree bitmap */
  /* don't set bt->map until we're done, since getnode() checks it */

  map = ALLOC(char, HFS_MAP1SZ);
  if (map == 0)
    {
      ERROR(ENOMEM, 0);
      return -1;
    }

  memcpy(map, HFS_NODEREC(bt->hdrnd, 2), HFS_MAP1SZ);
  bt->mapsz = HFS_MAP1SZ;

  /* read continuation map records, if any */

  nnum = bt->hdrnd.nd.ndFLink;

  while (nnum)
    {
      node n;
      char *newmap;

      n.bt   = bt;
      n.nnum = nnum;

      if (bt_getnode(&n) < 0)
	{
	  FREE(map);
	  return -1;
	}

      if (n.nd.ndType != ndMapNode ||
	  n.nd.ndNRecs != 1 ||
	  n.roff[0] != 0x00e ||
	  n.roff[1] != 0x1fa)
	{
	  FREE(map);
	  ERROR(EIO, "malformed b*-tree map node");
	  return -1;
	}

      newmap = REALLOC(map, char, bt->mapsz + HFS_MAPXSZ);
      if (newmap == 0)
	{
	  FREE(map);
	  ERROR(ENOMEM, 0);
	  return -1;
	}
      map = newmap;

      memcpy(map + bt->mapsz, HFS_NODEREC(n, 0), HFS_MAPXSZ);
      bt->mapsz += HFS_MAPXSZ;

      nnum = n.nd.ndFLink;
    }

  bt->map = map;

  return 0;
}

/*
 * NAME:	btree->writehdr()
 * DESCRIPTION:	write the header node of a B*-tree
 */
int bt_writehdr(btree *bt)
{
  unsigned char *ptr;
  char *map;
  unsigned long mapsz, nnum;
  int i;

  if (bt->hdrnd.bt != bt ||
      bt->hdrnd.nnum != 0 ||
      bt->hdrnd.nd.ndType != ndHdrNode ||
      bt->hdrnd.nd.ndNRecs != 3)
    abort();

  ptr = HFS_NODEREC(bt->hdrnd, 0);

  /*
  printf("hfs: btree(0x%08x) hdr written, has %d nodes, %d recs\n",
		 bt,bt->hdr.bthNNodes,bt->hdr.bthNRecs);
		 */
  
  d_storew(&ptr, bt->hdr.bthDepth);
  d_storel(&ptr, bt->hdr.bthRoot);
  d_storel(&ptr, bt->hdr.bthNRecs);
  d_storel(&ptr, bt->hdr.bthFNode);
  d_storel(&ptr, bt->hdr.bthLNode);
  d_storew(&ptr, bt->hdr.bthNodeSize);
  d_storew(&ptr, bt->hdr.bthKeyLen);
  d_storel(&ptr, bt->hdr.bthNNodes);
  d_storel(&ptr, bt->hdr.bthFree);
  
  for (i = 0; i < 76; ++i)
    d_storeb(&ptr, bt->hdr.bthResv[i]);

  memcpy(HFS_NODEREC(bt->hdrnd, 2), bt->map, HFS_MAP1SZ);

  if (bt_putnode(&bt->hdrnd) < 0)
    return -1;

  map   = bt->map   + HFS_MAP1SZ;
  mapsz = bt->mapsz - HFS_MAP1SZ;

  nnum  = bt->hdrnd.nd.ndFLink;

  while (mapsz)
    {
      node n;

      if (nnum == 0)
	{
	  ERROR(EIO, "truncated b*-tree map");
	  return -1;
	}

      n.bt   = bt;
      n.nnum = nnum;

      if (bt_getnode(&n) < 0)
	return -1;

      if (n.nd.ndType != ndMapNode ||
	  n.nd.ndNRecs != 1 ||
	  n.roff[0] != 0x00e ||
	  n.roff[1] != 0x1fa)
	{
	  ERROR(EIO, "malformed b*-tree map node");
	  return -1;
	}

      memcpy(HFS_NODEREC(n, 0), map, HFS_MAPXSZ);

      if (bt_putnode(&n) < 0)
	return -1;

      map   += HFS_MAPXSZ;
      mapsz -= HFS_MAPXSZ;

      nnum = n.nd.ndFLink;
    }

  bt->flags &= ~HFS_UPDATE_BTHDR;

  return 0;
}

/* High-Level B*-Tree Routines ============================================= */

/*
 * NAME:	btree->space()
 * DESCRIPTION:	assert space for new records, or extend the file
 */
int bt_space(btree *bt, unsigned int nrecs)
{
  unsigned int nnodes;
  int space;
  
  nnodes = nrecs * (bt->hdr.bthDepth + 1);
  
  if (nnodes <= bt->hdr.bthFree)
    return 0;
  
  /* make sure the extents tree has room too */
  
  if (bt != &bt->f.vol->ext) {
	if (bt_space(&bt->f.vol->ext, 1) < 0)
	  return -1;
  }
  
  space = f_alloc(&bt->f);
  if (space < 0)
    return -1;
  
  nnodes = space * (bt->f.vol->mdb.drAlBlkSiz / bt->hdr.bthNodeSize);
  
  //  printf("hfs: btree(0x%08x) numnodes %d->%d\n",bt,
  //		 bt->hdr.bthNNodes,bt->hdr.bthNNodes+nnodes);
  bt->hdr.bthNNodes += nnodes;
  bt->hdr.bthFree   += nnodes;
  
  bt->flags |= HFS_UPDATE_BTHDR;
  
  bt->f.vol->flags |= HFS_UPDATE_ALTMDB|HFS_UPDATE_MDB;
  
  while (bt->hdr.bthNNodes > bt->mapsz * 8) {
	char *newmap;
	
	/* extend tree map */
	
	newmap = REALLOC(bt->map, char, bt->mapsz + HFS_MAPXSZ);
	if (newmap == 0) {
	  ERROR(ENOMEM, 0);
	  return -1;
	}
	
	memset(newmap + bt->mapsz, 0, HFS_MAPXSZ);
	
	bt->map    = newmap;
	bt->mapsz += HFS_MAPXSZ;
	
	n_init(&bt->mapnd, bt, ndMapNode, 0);
	if (n_new(&bt->mapnd) < 0)
	  return -1;
	
	/* link the new map node */
	
	if (bt->hdrnd.nd.ndFLink == 0) {
	  bt->hdrnd.nd.ndFLink = bt->mapnd.nnum;
	  bt->mapnd.nd.ndBLink     = 0;
	} else {
	  bt->n.bt   = bt;
	  bt->n.nnum = bt->hdrnd.nd.ndFLink;
	  
	  while (1) {
		if (bt_getnode(&bt->n) < 0)
		  return -1;
		
		if (bt->n.nd.ndFLink == 0)
		  break;
		
		bt->n.nnum = bt->n.nd.ndFLink;
	  }
	  
	  bt->n.nd.ndFLink     = bt->mapnd.nnum;
	  bt->mapnd.nd.ndBLink = bt->n.nnum;
	  
	  if (bt_putnode(&bt->n) < 0)
	    return -1;
	}
	
	bt->mapnd.nd.ndNRecs = 1;
	bt->mapnd.roff[1]    = 0x1fa;
	
	if (bt_putnode(&bt->mapnd) < 0)
	  return -1;
  }
  
  return 0;
}

/*
 * NAME:	btree->insertx()
 * DESCRIPTION:	recursively locate a node and insert a record
 */
int bt_insertx(node *np, unsigned char *record, int *reclen)
{
  node *child;
  int err = 0;
  unsigned char *rec;
  
  if (n_search(np, record)) {
	ERROR(EIO, "b*-tree record already exists");
	printf("Spinning in bt_insertx\n");
	for (;1;);
	return -1;
  }

  child = (node *)malloc(sizeof(node));
  
  switch ((unsigned char) np->nd.ndType) {
  case ndIndxNode:
	if (np->rnum < 0)
	  rec = HFS_NODEREC(*np, 0);
	else
	  rec = HFS_NODEREC(*np, np->rnum);
	
	child->bt   = np->bt;
	child->nnum = d_getl(HFS_RECDATA(rec));
	
	if (bt_getnode(child) < 0 ||
		bt_insertx(child, record, reclen) < 0) {
	  err = -1;
	  goto bt_insertx_done;
	};
	
	if (np->rnum < 0) {
	  n_index(np->bt, HFS_NODEREC((*child), 0), child->nnum, rec, 0);
	  if (*reclen == 0) {
		err = bt_putnode(np);
		goto bt_insertx_done;
	  };
	}
	
	err = *reclen ? n_insert(np, record, reclen) : 0;
	goto bt_insertx_done;
	
  case ndLeafNode:
	err = n_insert(np, record, reclen);
	goto bt_insertx_done;
	
  default:
	ERROR(EIO, "unexpected b*-tree node");
	err = -1;
	goto bt_insertx_done;
  }

bt_insertx_done:

  free(child);
  
  return err;
}

/*
 * NAME:	btree->insert()
 * DESCRIPTION:	insert a new node record into a tree
 */
int bt_insert(btree *bt, unsigned char *record, int reclen)
{
  if (bt->hdr.bthRoot == 0) {
	/* create root node */
	
	n_init(&bt->root, bt, ndLeafNode, 1);
	if (n_new(&bt->root) < 0 ||
		bt_putnode(&bt->root) < 0)
	  return -1;
	
	bt->hdr.bthDepth = 1;
	bt->hdr.bthRoot  = bt->root.nnum;
	bt->hdr.bthFNode = bt->root.nnum;
	bt->hdr.bthLNode = bt->root.nnum;
	
	bt->flags |= HFS_UPDATE_BTHDR;
  } else {
	bt->root.bt   = bt;
	bt->root.nnum = bt->hdr.bthRoot;
	
	if (bt_getnode(&bt->root) < 0)
	  return -1;
  }

  if (bt_insertx(&bt->root, record, &reclen) < 0)
    return -1;
  
  if (reclen) {
	unsigned char oroot[HFS_MAXRECLEN];
	int orootlen;
	
	/* root node was split; create a new root */
	
	n_index(bt, HFS_NODEREC((bt->root), 0), bt->root.nnum, oroot, &orootlen);
	
	n_init(&bt->root, bt, ndIndxNode, bt->root.nd.ndNHeight + 1);
	if (n_new(&bt->root) < 0)
	  return -1;
	
	++bt->hdr.bthDepth;
	bt->hdr.bthRoot = bt->root.nnum;
	
	bt->flags |= HFS_UPDATE_BTHDR;
	
	/* insert index records for new root */
	
	n_search(&bt->root, oroot);
	n_insertx(&bt->root, oroot, orootlen);
	
	n_search(&bt->root, record);
	n_insertx(&bt->root, record, reclen);
	
	if (bt_putnode(&bt->root) < 0)
	  return -1;
  }
  
  ++bt->hdr.bthNRecs;
  bt->flags |= HFS_UPDATE_BTHDR;
  
  return 0;
}

/*
 * NAME:	btree->deletex()
 * DESCRIPTION:	recursively locate a node and delete a record
 */
int bt_deletex(node *np, unsigned char *key, unsigned char *record, int *flag)
{
  node *child=NULL;
  unsigned char *rec;
  int found;

  found = n_search(np, key);
  
  switch ((unsigned char) np->nd.ndType) {
  case ndIndxNode:
	if (np->rnum < 0) {
	  ERROR(EIO, "b*-tree record not found");
	  free(child);
	  return -1;
	}
	
	child = (node *)malloc(sizeof(node));
	rec = HFS_NODEREC(*np, np->rnum);
	
	child->bt   = np->bt;
	child->nnum = d_getl(HFS_RECDATA(rec));
	
	if (bt_getnode(child) < 0 ||
		bt_deletex(child, key, rec, flag) < 0) {
	  free(child);
	  return -1;
	};
	
	if (*flag) {
	  *flag = 0;
	  
	  if (HFS_RECKEYLEN(rec) == 0) {
		free(child);
		return n_delete(np, record, flag);
	  };
	  
	  if (np->rnum == 0) {
		n_index(np->bt, HFS_NODEREC(*np, 0), np->nnum, record, 0);
		*flag = 1;
	  }

	  free(child);
	  return bt_putnode(np);
	}

	free(child);
	return 0;
	
  case ndLeafNode:
	if (found == 0) {
	  ERROR(EIO, "b*-tree record not found");
	  return -1;
	}
	
	return n_delete(np, record, flag);
	
  default:
	ERROR(EIO, "unexpected b*-tree node");
	return -1;
  }
}

/*
 * NAME:	btree->delete()
 * DESCRIPTION:	remove a node record from a tree
 */
int bt_delete(btree *bt, unsigned char *key)
{
  unsigned char record[HFS_MAXRECLEN];
  int flag = 0;

  bt->root.bt   = bt;
  bt->root.nnum = bt->hdr.bthRoot;

  if (bt->root.nnum == 0) {
	ERROR(EIO, "empty b*-tree");
	return -1;
  }
  
  if (bt_getnode(&bt->root) < 0 ||
      bt_deletex(&bt->root, key, record, &flag) < 0)
    return -1;
  
  if (bt->hdr.bthDepth > 1 && bt->root.nd.ndNRecs == 1) {
	unsigned char *rec;
	
	/* chop the root */
	
	rec = HFS_NODEREC(bt->root, 0);
	
	--bt->hdr.bthDepth;
	bt->hdr.bthRoot = d_getl(HFS_RECDATA(rec));
	
	n_free(&bt->root);
  } else if (bt->hdr.bthDepth == 1 && bt->root.nd.ndNRecs == 0) {
	/* delete the root node */
	
	bt->hdr.bthDepth = 0;
	bt->hdr.bthRoot  = 0;
	bt->hdr.bthFNode = 0;
	bt->hdr.bthLNode = 0;
	
	n_free(&bt->root);
  }
  
  --bt->hdr.bthNRecs;
  bt->flags |= HFS_UPDATE_BTHDR;
  
  return 0;
}

/*
 * NAME:	btree->search()
 * DESCRIPTION:	locate a data record given a search key
 */
int bt_search(btree *bt, unsigned char *key, node *np)
{
  int oldnum;

  np->bt   = bt;
  np->nnum = bt->hdr.bthRoot;

  if (np->nnum == 0)
    {
      ERROR(ENOENT, 0);
      return 0;
    }

  while (1)
    {
      int found;
      unsigned char *rec;

      if (bt_getnode(np) < 0) {
		printf("Couldn't get node!\n");
		return -1;
	  };

      found = n_search(np, key);

      switch ((unsigned char) np->nd.ndType)
	{
	case ndIndxNode:
	  if (np->rnum < 0)
	    {
	      ERROR(ENOENT, 0);
	      return 0;
	    }

	  /*
	  printf("Walking %d:%d...\n",np->nnum,np->rnum);
	  */
	  rec = HFS_NODEREC(*np, np->rnum);
	  oldnum = np->nnum;
	  np->nnum = d_getl(HFS_RECDATA(rec));
	  /*
	  printf("Node number: %d->%d\n",oldnum,np->nnum);
	  */
	  break;

	case ndLeafNode:
	  if (! found) {
	    ERROR(ENOENT, 0);
	  };

	  return found;

	default:
	  ERROR(EIO, "unexpected b*-tree node");
	  return -1;
	}
    }
}


