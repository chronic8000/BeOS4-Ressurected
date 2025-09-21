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

# include <stdio.h>
# include <sys/types.h>
# include <unistd.h>
# include <errno.h>
# include <time.h>

# include "internal.h"
# include "data.h"
# include "block.h"
# include "low.h"
# include "lock.h"
# include "cache.h"

/*
 * NAME:	block->readlb()
 * DESCRIPTION:	read a logical block from a volume
 */
int b_readlb(hfsvol *vol, unsigned long num, block *bp)
{
  int bytes;

/*
  if (lseek(vol->fd, (vol->vstart + num) * vol->blockSize, SEEK_SET) < 0)
    {
      ERROR(errno, "error seeking device");
      return -1;
    }

  bytes = read(vol->fd, bp, vol->blockSize);
*/
  bytes = cached_read(vol->fd, vol->vstart+num, bp, 1, vol->blockSize);
/*
  if (bytes < 0)
    {
      ERROR(errno, "error reading from device");
      return -1;
    }
  else if (bytes == 0)
    {
      ERROR(EIO, "read EOF on volume");
      return -1;
    }
  else if (bytes != vol->blockSize)
    {
      ERROR(EIO, "read incomplete block");
      return -1;
    }
*/
  return bytes;
}

/*
 * NAME:	block->writelb()
 * DESCRIPTION:	write a logical block to a volume
 */
int b_writelb(hfsvol *vol, unsigned long num, block *bp)
{
  int bytes;

/*
  if (lseek(vol->fd, (vol->vstart + num) * vol->blockSize, SEEK_SET) < 0)
    {
      ERROR(errno, "error seeking device");
      return -1;
    }

  bytes = write(vol->fd, bp, vol->blockSize);
*/
  bytes = cached_write(vol->fd, vol->vstart+num, bp, 1, vol->blockSize);
/*
  if (bytes < 0)
    {
      ERROR(errno, "error writing to device");
      return -1;
    }
  else if (bytes != vol->blockSize)
    {
      ERROR(EIO, "wrote incomplete block");
      return -1;
    }
*/
  return bytes;
}

/*
 * NAME:	block->readab()
 * DESCRIPTION:	read a block from an allocation block from a volume
 */
int b_readab(hfsvol *vol,
	     unsigned int anum, unsigned int index, block *bp)
{
  /* verify the allocation block exists and is marked as in-use */

  if (anum >= vol->mdb.drNmAlBlks)
    {
      ERROR(EIO, "read nonexistent block");
      return -1;
    }
  else if (vol->vbm && ! BMTST(vol->vbm, anum))
    {
      ERROR(EIO, "read unallocated block");
      return -1;
    }

  return b_readlb(vol, vol->mdb.drAlBlSt + anum * vol->lpa + index, bp);
}

/*
 * NAME:	b->writeab()
 * DESCRIPTION:	write a block to an allocation block to a volume
 */
int b_writeab(hfsvol *vol,
	      unsigned int anum, unsigned int index, block *bp)
{
  /* verify the allocation block exists and is marked as in-use */

  if (anum >= vol->mdb.drNmAlBlks)
    {
      ERROR(EIO, "write nonexistent block");
      return -1;
    }
  else if (vol->vbm && ! BMTST(vol->vbm, anum))
    {
      ERROR(EIO, "write unallocated block");
      return -1;
    }

  vol->mdb.drAtrb &= ~HFS_ATRB_UMOUNTED;
  vol->mdb.drLsMod = d_tomtime(time(0));
  ++vol->mdb.drWrCnt;

  vol->flags |= HFS_UPDATE_MDB;

  return b_writelb(vol, vol->mdb.drAlBlSt + anum * vol->lpa + index, bp);
}

