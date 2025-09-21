
/*      load_pcl.c      */

/*
*
*       $Header: //bliss/user/archive/rcs/4in1/load_pcl.c,v 14.0 97/03/17 17:22:04 shawn Release $
*
*       $Log:	load_pcl.c,v $
 * Revision 14.0  97/03/17  17:22:04  shawn
 * TDIS Version 6.00
 * 
 * Revision 13.5  96/08/20  12:53:35  shawn
 * Added RCS/Header and Log keywords.
 * 
*
*/

#include <stdio.h>
#ifdef MSDOS
#include <stdlib.h>
#include <string.h>
#else
#include <sys/types.h>
#endif
#include "spdo_prv.h"
#include "hp_readr.h"
#include "pcl5fnt.h"
#if PROC_PCL

#ifndef SEEK_SET
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0
#endif

/*** EXTERNAL FUNCTIONS ***/
#ifdef MSDOS
GLOBAL_PROTO btsbool fill_font_buffer(ufix8 **ptr, ufix32 size, FILE *fp);
#endif


/*** STATIC VARIABLES ***/
#define  ESC_FNDESC  1
#define  ESC_CHARID  2
#define  ESC_CHDESC  3
#define  NO_ESCAPE   0
#define  ESC_UNKN   -1
#define  ESC_EOF    -2


typedef  struct
    {
    ufix8  *font;
    ufix16  last_index;
    fix31  *char_offset;
    }   dir_t;

static dir_t    char_dir;
static btsbool  pcl_font_loaded = FALSE;
#endif /* PROC_PCL */
/* we need this global for most demo programs: */
PC5HEAD  pcl5head;
#if PROC_PCL

typedef fix31 fix;

/***  STATIC FUNCTIONS ***/

LOCAL_PROTO fix31 filesize(FILE *fd);
LOCAL_PROTO void mbyte(char *source, char *dest, fix31 count);
LOCAL_PROTO fix escape_seq(FILE *fd, real *arg);
LOCAL_PROTO btsbool load_header(char *buffer, PC5HEAD *pcl5head);
LOCAL_PROTO btsbool is_num(char c);

GLOBAL_PROTO fix pf_get_nr_font_chars(void);
GLOBAL_PROTO void pf_unload_font(void);


FUNCTION fix pf_load_pcl_font(char *pathname, eofont_t *eo_font, FILE *fd)
{
real     arg;
char     buffer[4096];
fix      char_index;
fix      chsize;
fix31    ll;
btsbool  ok;


if (!fd)
{
        /* calling unit did not open the file: */
	if ((fd = fopen(pathname, "rb")) == BTSNULL)
        {
    	printf("\nCan't open file %s\n\n", pathname);
    	return(0);
        }
}

ll = filesize(fd);

#ifdef MSDOS
ok = fill_font_buffer(&char_dir.font, ll, fd);
if (!ok)
{
    fprintf(stderr,"Error reading font file\n");
    fclose(fd);
    return(FALSE);
}
#else
char_dir.font = (ufix8 *)malloc(ll);
if (char_dir.font == BTSNULL)
    {
    printf("MALLOC fail %s line %d\n", __FILE__, __LINE__);
    goto abort;
    }
if (fread(char_dir.font, 1, ll, fd) != ll)
    {
    printf("\nRead Error (%s)!\n\n", pathname);
    goto abort;
    }
#endif

fseek(fd, (fix31)0, 0);
ok = TRUE;
while (ok)
    {
    switch (escape_seq(fd, &arg))
        {
        case ESC_FNDESC:
            if ((fix)arg < 4096)
                {
                fread((void *)buffer, (size_t)1, (size_t)arg, fd);
                load_header(buffer, &pcl5head);
                char_dir.last_index = (127 > pcl5head.last_code) ? 127 : pcl5head.last_code;
                ll = (char_dir.last_index + 1) * 4;
                char_dir.char_offset = (fix31 *)malloc((size_t)ll);
                if (char_dir.char_offset == BTSNULL)
                    {
                    printf("MALLOC fail %s line %d\n", __FILE__, __LINE__);
                    goto abort;
                    }
                for (ll=0; ll <= char_dir.last_index; ll++)
                    char_dir.char_offset[ll] = 0;
                }
            break;
        case ESC_CHARID:
            char_index = (fix15)arg;
            break;
        case ESC_CHDESC:
            chsize = (fix)arg;
            char_dir.char_offset[char_index] = ftell(fd);
            fseek(fd, (fix31)chsize, 1);
            break;
        case NO_ESCAPE:
            printf("Escape character not found\n");
            goto abort;
            break;
        case ESC_EOF:
            ok = FALSE;
            break;
        case ESC_UNKN:
        default:
            printf("Unrecognized escape sequence\n");
            goto abort;
            break;
        }
    }
eo_font->resolution = pcl5head.scale_factor;
eo_font->unique_id = 17;
eo_font->eofont = &char_dir;
pcl_font_loaded = TRUE;

return(1);

abort:
  fclose(fd);
  return(0);
}


FUNCTION LOCAL_PROTO fix escape_seq(FILE *fd, real *arg)

/* Moves file pointer beyond the escape sequence, if there is one.
   If there is no escape code in the first character, or if the escape
   sequence is unrecognized, the file pointer is left unchanged. */

{
char    buf[4];
fix     code;
fix31   curpos;
fix     dpt;
real    ftemp;
fix     ii;
btsbool    minus;


curpos = fseek(fd, 0L, 1);
if (fread (buf, 1, 1, fd) != 1)
    return (ESC_EOF);
if (*buf != 0x1B)
    {
    fseek(fd, curpos, 0);
    return (NO_ESCAPE);
    }

fread (buf, 1, 2, fd);                      /* look for "*c", "(s", or ")s" */
if (strncmp( "*c", buf, 2 ) == 0)
    code = ESC_CHARID;
else if (strncmp( ")s", buf, 2 ) == 0)
    code = ESC_FNDESC;
else if (strncmp( "(s", buf, 2 ) == 0)
    code = ESC_CHDESC;
else
    {
    fseek(fd, curpos, 0);
    return (ESC_UNKN);
    }

fread (buf, 1, 1, fd);
for (*arg=0.0, minus=FALSE, dpt=0; is_num(*buf); fread (buf, 1, 1, fd))
    {
    if (*buf == '-')
        minus = TRUE;
    else if (*buf == '.')
        dpt = 1;
    else
        {
        if (!dpt)
            *arg = *arg * 10 + (*buf - '0');
        else
            {
            for (ftemp = *buf, ii=0; ii<dpt; ii++)
                ftemp /= 10.0;
            *arg += ftemp;
            dpt++;
            }
        }
    }
if (code == ESC_CHARID  &&  *buf == 'E')
    return (code);
else if ((code == ESC_FNDESC  ||  code == ESC_CHDESC)  &&  *buf == 'W')
    return (code);
else
    {
    fseek(fd, curpos, 0);
    return (ESC_UNKN);
    }
}



FUNCTION LOCAL_PROTO btsbool load_header(char *buffer, PC5HEAD *pcl5head)
{
ufix16  temp16;

pcl5head->size = GET_WORD(buffer);
pcl5head->dformat = buffer[2];
pcl5head->font_type = buffer[3];
temp16 = (buffer[4] << 8) | (ufix8)buffer[23];
pcl5head->style.structure = (temp16 & 0x3E0) >> 5;
pcl5head->style.width = (temp16 & 0x1C) >> 2;
pcl5head->style.posture = (temp16 & 3);
pcl5head->baseline = GET_WORD((buffer+6));
pcl5head->cell_width = GET_WORD((buffer+8));
pcl5head->cell_height = GET_WORD((buffer+10));
pcl5head->orientation = buffer[12];
pcl5head->spacing = buffer[13];
pcl5head->symbol_set = GET_WORD((buffer+14));
pcl5head->pitch = GET_WORD((buffer+16));
pcl5head->height = GET_WORD((buffer+18));
pcl5head->x_height = GET_WORD((buffer+20));
pcl5head->width_type = buffer[22];
pcl5head->stroke_weight = buffer[24];
temp16 = (buffer[26] << 8) | (ufix8)buffer[25];
pcl5head->typeface.vendor = (temp16 & 0x7800) >> 11;
pcl5head->typeface.version = (temp16 & 0x0600) >> 9;
pcl5head->typeface.value = (temp16 & 0x01FF);
pcl5head->serif_style = buffer[27];
pcl5head->quality = buffer[28];
pcl5head->placement = buffer[29];
pcl5head->uline_dist = buffer[30];
pcl5head->uline_height = buffer[31];
pcl5head->text_height = GET_WORD((buffer+32));
pcl5head->text_width = GET_WORD((buffer+34));
pcl5head->first_code = GET_WORD((buffer+36));
pcl5head->last_code = GET_WORD((buffer+38));
pcl5head->pitch_extend = buffer[40];
pcl5head->height_extend = buffer[41];
pcl5head->cap_height = GET_WORD((buffer+42));
pcl5head->font_vendor_code = buffer[44];
pcl5head->font_number = GET_WORD((buffer+46)) + (((ufix32)(buffer[45])) << 16);
mbyte (buffer+48, pcl5head->fontname, (fix31)16);
pcl5head->scale_factor = GET_WORD((buffer+64));
pcl5head->x_resolution = GET_WORD((buffer+66));
pcl5head->y_resolution = GET_WORD((buffer+68));
pcl5head->mstr_uline_pos = (fix15)GET_WORD((buffer+70));
pcl5head->mstr_uline_hght = GET_WORD((buffer+72));
pcl5head->threshold = GET_WORD((buffer+74));
pcl5head->italic_angle = (fix15)GET_WORD((buffer+76));
pcl5head->scal_data_size = GET_WORD((buffer+78));
pcl5head->nobtf = GET_WORD((buffer + 80 + pcl5head->scal_data_size));
pcl5head->copyright = (char *)malloc(pcl5head->nobtf - 2);
pcl5head->checksum = GET_WORD((buffer + 80 + pcl5head->scal_data_size + pcl5head->nobtf));
return(TRUE);
}



FUNCTION char *eo_get_char_addr(SPGLOBPTR2 void *eofont, ufix16 index)
{

if (index <= ((dir_t *)eofont)->last_index)
    if (((dir_t *)eofont)->char_offset[index] > 0)
        return((char *)(((dir_t *)eofont)->font + ((dir_t *)eofont)->char_offset[index]));

return((char *)BTSNULL);
}



FUNCTION fix pf_get_nr_font_chars(void)
{
return((fix)pcl5head.last_code);
}



FUNCTION static void mbyte(char *source, char *dest, fix31 count)

/*  MBYTE (SOURCE, DEST, COUNT)
 *
 *  Arguments:
 *    source - address of source array
 *    dest - address of destination array
 *    count - number of bytes to be moved
 *  Returns:  nothing
 *
 *  Moves 'count' bytes from the source array to the destination array
 *  Error handling:
 *    If 'count' less than zero, does nothing
 *    Will crash if 'source', or 'dest' don't point to valid memory location
 */

{
fix31   i;
char   *sarr, *darr;

    
if (count <= 0)
    return;

if (source > dest)
    {
    for (i=0; i<count; i++)
        *dest++ = *source++;
    }
else
    {
    sarr = (char *) (source + count);
    darr = (char *) (dest + count);
    for (i=0; i<count; i++)
        *(--darr) = *(--sarr);
    }

return;
}


FUNCTION LOCAL_PROTO fix31 filesize(FILE *fd)
{
fix31  nn;

fseek(fd,(long)0,SEEK_END);
nn = ftell(fd);
fseek(fd,0L,SEEK_SET);
return(nn); 
}

FUNCTION LOCAL_PROTO btsbool is_num(char c)
{
if (c >= '0'  &&  c <= '9')
    return(TRUE);
else if (c == '-')
    return(TRUE);
else if (c == '.')
    return(TRUE);

return(FALSE);
}

FUNCTION void pf_unload_font(void)
{

if (char_dir.char_offset)
	{
	free(char_dir.char_offset);
	char_dir.char_offset = (fix31 *)0;
	}

if (pcl5head.copyright)
        {	
	free(pcl5head.copyright);
	pcl5head.copyright = (char *)0;
	}

if (char_dir.font)
	{
	free(char_dir.font);
	char_dir.font = (ufix8 *)0;
	}
}
#endif /* PROC_PCL */
/* EOF load_pcl.c */
