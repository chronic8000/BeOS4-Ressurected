/* ++++++++++
	FILE:	font_file.cpp
	REVS:	$Revision: 1.4 $
	NAME:	pierre
	DATE:	Thu Apr  3 10:31:54 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#ifndef _MATH_H
#include <math.h>
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _STRING_H
#include <string.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#ifndef _FCNTL_H
#include <fcntl.h>
#endif
#ifndef	MACRO_H
#include "macro.h"
#endif
#ifndef	PROTO_H
#include "proto.h"
#endif
#ifndef _FONT_RENDERER_H
#include "font_renderer.h"
#endif
#ifndef _FONT_DATA_H
#include "font_data.h"
#endif
#ifndef _FONT_CACHE_H
#include "font_cache.h"
#endif
#include <UTF8.h>

#include "as_support.h"
#include <Debug.h>

#define BAD_FONT_DEBUG	0

#if BAD_FONT_DEBUG
#define _bad_font_printf1(x)		_sPrintf(x)
#define _bad_font_printf2(x,y)		_sPrintf(x,y)
#define _bad_font_printf3(x,y,z)	_sPrintf(x,y,z)
#define _bad_font_printf4(x,y,z,t)	_sPrintf(x,y,z,t)
#else
#define _bad_font_printf1(x)		{;}
#define _bad_font_printf2(x,y)		{;}
#define _bad_font_printf3(x,y,z)	{;}
#define _bad_font_printf4(x,y,z,t)	{;}
#endif

#define SIZE_TO(type, var, field) \
	( offsetof(type,field) + sizeof(var.field) )

bool get_stroke_info(fc_file_entry *fe, FILE *fp);


/*****************************************************************/
/* Font file registration */

//static void cut_8bits_name(char *name, int max_bytes) {
//	int        length;
//
//	length = strlen(name);
//	if (length > max_bytes)
//		name[max_bytes] = 0;
//}

static void cut_utf8_name(char *name, int max_bytes) {
	int        cut, index;

	cut = 0;
	index = 0;
	while (name[index] != 0) {
		if (index > max_bytes) {
			name[cut] = 0;
			break;
		}
		if (((uchar)name[index] & 0xc0) != 0x80)
			cut = index;
		index++;
	}
}

char* fc_convert_font_name_to_utf8(const char *name, int32 nameLength, int32 maxLength, uint16 platformID, uint16 languageID)
{
	uint32	conversion = B_ISO1_CONVERSION;
	
	switch (platformID) {
		case 0: // plat_Unicode
			conversion = B_UNICODE_CONVERSION;
			break;

		case 1: // plat_Macintosh
			if (languageID == 11) // Japanese
				conversion = B_SJIS_CONVERSION;
			else
				conversion = B_MAC_ROMAN_CONVERSION;
			break;

		case 2: // plat_ISO
			conversion = B_ISO1_CONVERSION;
			break;

		case 3: // plat_MS
			if (languageID == 1041) // Japanese
				conversion = B_SJIS_CONVERSION;
			else
				conversion = B_MS_WINDOWS_CONVERSION;
			break;
		
		case 0xffff: // special code to convert from 16-bit unicode
			conversion = B_UNICODE_CONVERSION;
			break;
			
		default:
			conversion = B_ISO1_CONVERSION;
			break;
	}

	int32	state = 0;
	int32	utf8NameLength = nameLength * 4;
	char	*utf8Name = (char *)grMalloc(utf8NameLength+1,"fc:utf8Name",MALLOC_CANNOT_FAIL);

	convert_to_utf8(conversion, name, &nameLength, utf8Name, &utf8NameLength, &state, '?');
	utf8Name[utf8NameLength] = '\0';

	cut_utf8_name(utf8Name, maxLength);

	return (utf8Name);
}

#define PS_BUFFER_SIZE	128

static bool find_ps_header(char *buf, int32 size, int32 *index) {
	int32		i;
	
	for (i=0; i<(size-20); i++)
		if ((buf[i] == 0x25) && (buf[i+1] == 0x21))
			if ((strncmp(buf+(i+2), "PS-AdobeFont-1.0", 16) == 0) ||
				(strncmp(buf+(i+2), "PS-AdobeFont-1.1", 16) == 0) ||
				(strncmp(buf+(i+2), "FontType1-1.0", 13) == 0)) {
			 	*index = i;
			 	return true;
			}
	return false;
}

static char *extract_string(char *buffer, int32 *index) {
	char		*res;
	int32		i, j;
	
	i = *index;
	while ((buffer[i] != '(') && (i<(2*PS_BUFFER_SIZE))) i++;

	if (i == (2*PS_BUFFER_SIZE)) {
		return NULL;
	}
	j = i+1;
	while ((buffer[j] != ')') && (j<(2*PS_BUFFER_SIZE))) j++;
	if (j == (2*PS_BUFFER_SIZE)) {
		return NULL;
	}
	res = (char*)grMalloc(j-i,"fc:extract_string:res",MALLOC_CANNOT_FAIL);
	memcpy(res, buffer+i+1, j-i-1);
	res[j-i-1] = 0;
	*index = j+1;
	return res;
}

static bool extract_bbox(char *buffer, int32 *index, int32 *bbox) {
	int32		i, j;
	
	i = *index;
	while ((buffer[i] != '{') && (i<(2*PS_BUFFER_SIZE))) i++;
	sscanf(buffer+i+1, "%ld %ld %ld %ld", bbox+0, bbox+1, bbox+2, bbox+3);
	j = i+8;
	while ((buffer[j] != '}') && (j<(2*PS_BUFFER_SIZE))) j++;
	if (j == (2*PS_BUFFER_SIZE))
		return false;
	*index = j;
	return true;
}

static bool extract_matrix(char *buffer, int32 *index, float *matrix) {
	int32		i, j;
	
	i = *index;
	while ((buffer[i] != '[') && (i<(2*PS_BUFFER_SIZE))) i++;
	sscanf(buffer+i+1, "%f %f %f %f %f %f", matrix+0, matrix+1, matrix+2, matrix+3, matrix+4, matrix+5);
	j = i+12;
	while ((buffer[j] != ']') && (j<(2*PS_BUFFER_SIZE))) j++;
	if (j == (2*PS_BUFFER_SIZE))
		return false;
	*index = j;
	return true;
}

static bool get_ps_info(fc_file_entry *fe, FILE *fp, int32 index) {
	const char FamilyHeader[] = "FamilyName";
	const char WeightHeader[] = "Weight";
	const char ItalicHeader[] = "ItalicAngle";
	const char StyleHeader[] = "FullName";
	const char PitchHeader[] = "isFixedPitch";
	const char BBoxHeader[] = "FontBBox";
	const char MatrixHeader[] = "FontMatrix";
	const char EncodingDict[] = "Encoding";
	const char ReadOnlyDef[] = "readonly def";
	const char EndHeader[] = "eexec";
	const int32 EndHeaderLength = 5;

	char		buffer[2*PS_BUFFER_SIZE+1];
	char		*family, *weight, *fullname, *style;
	int32		italic, monospaced, length;
	int32		i, j, k;
	int32		bbox[4];
	float		matrix[6];
	bool		inEncodingDict = false;


	if (fseek(fp, index, SEEK_SET) != 0) {
		_bad_font_printf1("##### ERROR #####  Cannot seek to begining of file !\n");
		return false;
	}
	if (fread(buffer+PS_BUFFER_SIZE, 1, PS_BUFFER_SIZE, fp) != PS_BUFFER_SIZE) {
		_bad_font_printf2("##### ERROR #####  PS file smaller than %d bytes !\n", PS_BUFFER_SIZE);
		return false;
	}
	buffer[2*PS_BUFFER_SIZE] = 0;

	family = NULL;
	weight = NULL;
	fullname = NULL;
	monospaced = -1;
	italic = -1000000;
	bbox[0] = -1000000;
	matrix[0] = -1000000.0;

	while ((family == NULL) || (weight == NULL) || (fullname == NULL) || (italic == -1000000) ||
		   (monospaced == -1) || (bbox[0] == -1000000) || (matrix[0] == -1000000.0)) {
		memcpy(buffer, buffer+PS_BUFFER_SIZE, PS_BUFFER_SIZE);
		if (fread(buffer+PS_BUFFER_SIZE, 1, PS_BUFFER_SIZE, fp) != PS_BUFFER_SIZE) {
			_bad_font_printf1("##### ERROR #####  Unexpected EOF !\n");
			goto exit_error;
		}
		i = 0;
		while (i < PS_BUFFER_SIZE) {
			if (!inEncodingDict && (buffer[i] == '/') &&
				(((buffer[i+1] >= 'A') && (buffer[i+1] <= 'Z')) ||
				 ((buffer[i+1] >= 'a') && (buffer[i+1] <= 'z')))) {
				j = i+1;
				while ((((buffer[j] >= 'A') && (buffer[j] <= 'Z')) ||
						((buffer[j] >= 'a') && (buffer[j] <= 'z'))) &&
					   (j < (i+PS_BUFFER_SIZE))) j++;

				/* Family name */
				if (strncmp(buffer+i+1, FamilyHeader, j-i-1) == 0) {
					family = extract_string(buffer, &j);
					if (family == NULL) {
						_bad_font_printf1("##### ERROR #####  Corrupted family name !\n");
						goto exit_error;
					}
				}
				/* Weigth name */
				else if (strncmp(buffer+i+1, WeightHeader, j-i-1) == 0) {
					weight = extract_string(buffer, &j);
					if (weight == NULL) {
						_bad_font_printf1("##### ERROR #####  Corrupted Weight info !\n");
						goto exit_error;
					}
				}
				/* Italic value */
				else if (strncmp(buffer+i+1, ItalicHeader, j-i-1) == 0) {
					j++;
					sscanf(buffer+j, "%ld", &italic);
				}
				/* Fixed Pitch flag */
				else if (strncmp(buffer+i+1, PitchHeader, j-i-1) == 0) {
					j++;
					if (strncmp(buffer+j+1, "true", 4) == 0)
						monospaced = 1;
					else
						monospaced = 0;
				}
				/* Encoding Dictionary begins */
				else if (strncmp(buffer+i+1, EncodingDict, j-i-1) == 0) {
					j++;
					inEncodingDict = true;					
				}
				/* Full name, including style info */
				else if (strncmp(buffer+i+1, StyleHeader, j-i-1) == 0) {
					fullname = extract_string(buffer, &j);
					if (fullname == NULL) {
						_bad_font_printf1("##### ERROR #####  Corrupted Full name !\n");
						goto exit_error;
					}
				}
				/* Bounding box */
				else if (strncmp(buffer+i+1, BBoxHeader, j-i-1) == 0) {
					if (!extract_bbox(buffer, &j, bbox)) {
						_bad_font_printf1("##### ERROR #####  Corrupted Bounding Box datas !\n");
						goto exit_error;
					}
					if ((bbox[0] >= bbox[2]) || (bbox[1] >= bbox[3])) {
						_bad_font_printf1("##### ERROR #####  Invalid Bounding Box datas !\n");
						goto exit_error;
					}
				}				
				/* Font Matrix */
				else if (strncmp(buffer+i+1, MatrixHeader, j-i-1) == 0) {
					if (!extract_matrix(buffer, &j, matrix)) {
						_bad_font_printf1("##### ERROR #####  Corrupted Matrix !\n");
						goto exit_error;
					}
					if ((matrix[0] != matrix[3]) ||
						(matrix[1] != 0.0) ||
						(matrix[2] != 0.0) ||
						(matrix[4] != 0.0) ||
						(matrix[5] != 0.0))
						matrix[0] = -1000000.0;
				}
				/* next one... */			
				i = j; 
			}
			else if (inEncodingDict && buffer[i] == 'r') {
				if (strncmp(buffer+i, ReadOnlyDef, strlen(ReadOnlyDef)) == 0) {
					inEncodingDict = false; // encoding dictionary ends
					i += strlen(ReadOnlyDef);
				}
				else i++;
			}
			else if (buffer[i] == 'e') {
				if (strncmp(buffer+i, EndHeader, EndHeaderLength) == 0)
					goto end_of_scan;
				i++;
			}
			else i++;
		}
	}
	
end_of_scan:
	if ((family == NULL) || (fullname == NULL) || (bbox[0] == -1000000)) {
		_bad_font_printf1("##### ERROR #####  Didn't find the names and the BBox...\n");
		goto exit_error;
	}
	length = strlen(family);

//	There's nothing in the Type 1 spec which says that the family name
//	needs to match the full font name...perhaps this was for BeOS-specific
//	reasons?

#if 0
	if (strncmp(family, fullname, length) != 0) {
		_bad_font_printf1("##### ERROR #####  Both names are not compatible\n");
		goto exit_error;
	}
#endif 

	if ((fullname[length] != ' ') && (fullname[length] != '-')) {
		if (weight != NULL)
			style = weight;
		else if (fullname[length] != 0)
			style = fullname+length;
		else {
			_bad_font_printf1("##### ERROR #####  No style information at all...\n");
			goto exit_error;
		}
	}
	else
		style = fullname+(length+1);
	
	/* Hardcoded blocks definition corresponding to the standard cmap */
	fe->blocks[0] = 0x1000002fL;
	fe->blocks[1] = 0x05000409L;
	fe->blocks[2] = 0x00000000L;

	if ((bbox[0] != -1000000) && (matrix[0] != -1000000.0)) {
		const float ascent = matrix[0]*bbox[3];
		const float descent = -matrix[0]*bbox[1];
		if (ascent > 0.0 && descent > 0 &&
				(ascent+descent) >= 0.3 &&
				(ascent+descent) <= 5.0) {
			fe->ascent = ascent;
			fe->descent = descent;
			if (descent < 0.16)
				fe->leading = 0.08;
			else if (descent < 0.4)
				fe->leading = descent*0.5;
			else
				fe->leading = descent*0.25 + 0.1;
		}
	}
	for (i=0; i<4; i++)
		fe->bbox[i] = matrix[0]*bbox[i];
	fe->flags = 0;
	if (monospaced == 1)
		fe->flags |= FC_MONOSPACED_FONT;
	fe->faces = 0;
	if ((italic > -40) && (italic < 0))
		fe->faces |= B_ITALIC_FACE;
	if (weight != NULL) {
		if (strncmp(weight, "Bold", 4) == 0)
			fe->faces |= B_BOLD_FACE;
		if (((strcmp(weight, "Roman") == 0) ||
			 (strcmp(weight, "Medium") == 0) ||
			 (strcmp(weight, "Regular") == 0)) && (fe->faces == 0))
			fe->faces = B_REGULAR_FACE;
	}
	fe->size = 10.0;
	fe->family_id = fc_register_family(family, "PS");
	fe->style_id = fc_set_style(fe->family_id, style, fe-file_table,
								fe->flags, fe->faces, fe->gasp, TRUE);
	if (fe->style_id == 0xffff) {
		fc_remove_style(fe->family_id, -1);
		_bad_font_printf2("##### ERROR #####  Failed to create the new style [%s]\n", style);
		goto exit_error;
	}
	
	if (family)
		grFree(family);
	if (weight)
		grFree(weight);
	if (fullname)
		grFree(fullname);
	return true;

exit_error:
	if (family)
		grFree(family);
	if (weight)
		grFree(weight);
	if (fullname)
		grFree(fullname);
	return false;
}

bool load_cmap_table(FILE *fp, uint32 file_offset,
					 fc_cmap_segment **list, int32 *seg_count, uint16 **glyphArray)
{
	typedef struct {
		uint16			format;
		uint16			length;
		uint16			version;
		uint16			segCountX2;
		uint16			searchRange;
		uint16			entrySelector;
		uint16			rangeShift;
	} sfnt_CmapSubTable4;

	int						i, count, loop_count, offset, size;
	uint16					buf[256];
	uint16					*glyphs;
	fc_cmap_segment			*segs;
	sfnt_CmapSubTable4		h;

	/* access and check the validity of the fixed part of the table header */
	if (fseek(fp, file_offset, SEEK_SET) != 0)
		return false;
	if (fread((char*)&h, 1, sizeof(sfnt_CmapSubTable4), fp) != sizeof(sfnt_CmapSubTable4))
		return false;
	if (fc_read16(&h.format) != 4)
		return false;
	/* calculate the size of the dynamic array */
	count = fc_read16(&h.segCountX2)/2;
	if ((count >= 8192) || (count <= 0))
		return false;
	*seg_count = count;
	/* allocate the segment array */
	segs = (fc_cmap_segment*)grMalloc(count*sizeof(fc_cmap_segment),"fc:seg_array",MALLOC_MEDIUM);
	if (segs == NULL)
		return false;
	*list = segs;
	glyphs = NULL;
	/* read and convert the dynamic tables in the segment array */
	offset = 0;
	while ((loop_count = (((count-offset)>256)?256:(count-offset))) != 0) {
		if (fread((char*)buf, 1, loop_count*sizeof(uint16), fp) != loop_count*sizeof(uint16))
			goto false_exit;
		for (i=0; i<loop_count; i++)
			segs[i+offset].endCount = fc_read16(buf+i);
		offset += loop_count;
	}
	if (fread((char*)buf, 1, sizeof(uint16), fp) != sizeof(uint16))
		goto false_exit;
	offset = 0;
	while ((loop_count = (((count-offset)>256)?256:(count-offset))) != 0) {
		if (fread((char*)buf, 1, loop_count*sizeof(uint16), fp) != loop_count*sizeof(uint16))
			goto false_exit;
		for (i=0; i<loop_count; i++)
			segs[i+offset].startCount = fc_read16(buf+i);
		offset += loop_count;
	}
	offset = 0;
	while ((loop_count = (((count-offset)>256)?256:(count-offset))) != 0) {
		if (fread((char*)buf, 1, loop_count*sizeof(uint16), fp) != loop_count*sizeof(uint16))
			goto false_exit;
		for (i=0; i<loop_count; i++)
			segs[i+offset].idDelta = fc_read16(buf+i);
		offset += loop_count;
	}
	offset = 0;
	while ((loop_count = (((count-offset)>256)?256:(count-offset))) != 0) {
		if (fread((char*)buf, 1, loop_count*sizeof(uint16), fp) != loop_count*sizeof(uint16))
			goto false_exit;
		for (i=0; i<loop_count; i++)
			segs[i+offset].idRangeOffset = fc_read16(buf+i);
		offset += loop_count;
	}
	/* calculate the size of the glyphArray */
	size = 0;
	for (i=0; i<count; i++)
		if (segs[i].idRangeOffset != 0) {
			size += (segs[i].endCount - segs[i].startCount + 1);
			/* update the idRangeOffset to be relative to the start of the glyphArray-1 */
			segs[i].idRangeOffset -= (count-i-1)*sizeof(uint16);
			segs[i].idRangeOffset /= sizeof(uint16);
		}
	for (i=0; i<count; i++)
		if (segs[i].idRangeOffset != 0)
			if ((segs[i].idRangeOffset > size) ||
				(segs[i].idRangeOffset+segs[i].endCount-segs[i].startCount > size))
				goto false_exit;
	/* allocate the glyphArray buffer */
	if (size != 0) {
		glyphs = (uint16*)grMalloc((size+1)*sizeof(uint16),"fc:glyphs",MALLOC_MEDIUM);
		if (glyphs == NULL)
			goto false_exit;
		*glyphArray = glyphs;
		/* read and convert the glyphArray buffer. Read it offset by 1, so that the glyphs
		   buffer was properly aligned to be directly used with the idRangeOffset. */
		if (fread((char*)(glyphs+1), 1, size*sizeof(uint16), fp) != size*sizeof(uint16))
			goto false_exit;
		for (i=0; i<size; i++)
			glyphs[i] = fc_read16(glyphs+i);
	}
	else
		*glyphArray = NULL;
	return true;
		
false_exit:
	grFree(segs);
	if (glyphs != NULL)
		grFree(glyphs);
	return false;
}

/* lower than belongs to entry number # */
enum { BLOCK_TABLE_COUNT = 84 };
static uint32 block_dispatch_table[BLOCK_TABLE_COUNT] = {
	0x0000	+0x01000000*	127,
	0x0080	+0x01000000*	0,
	0x0100	+0x01000000*	1,
	0x0180	+0x01000000*	2,
	0x0250	+0x01000000*	3,
	0x02b0	+0x01000000*	4,
	0x0300	+0x01000000*	5,
	0x0370	+0x01000000*	6,
	0x03d0	+0x01000000*	7,
	0x0400	+0x01000000*	8,
	0x0500	+0x01000000*	9,
	0x0530	+0x01000000*	127,
	0x0590	+0x01000000*	10,
	0x05d0	+0x01000000*	11,
	0x0600	+0x01000000*	12,
	0x0671	+0x01000000*	13,
	0x0700	+0x01000000*	14,
	0x0900	+0x01000000*	127,
	0x0980	+0x01000000*	15,
	0x0a00	+0x01000000*	16,
	0x0a80	+0x01000000*	17,
	0x0b00	+0x01000000*	18,
	0x0b80	+0x01000000*	19,
	0x0c00	+0x01000000*	20,
	0x0c80	+0x01000000*	21,
	0x0d00	+0x01000000*	22,
	0x0d80	+0x01000000*	23,
	0x0e00	+0x01000000*	127,
	0x0e80	+0x01000000*	24,
	0x0f00	+0x01000000*	25,
	0x0fc0	+0x01000000*	70,
	0x10a0	+0x01000000*	127,
	0x10d0	+0x01000000*	26,
	0x1100	+0x01000000*	27,
	0x1200	+0x01000000*	28,
	0x1e00	+0x01000000*	127,
	0x1f00	+0x01000000*	29,
	0x2000	+0x01000000*	30,
	0x2070	+0x01000000*	31,
	0x20a0	+0x01000000*	32,
	0x20d0	+0x01000000*	33,
	0x2100	+0x01000000*	34,
	0x2150	+0x01000000*	35,
	0x2190	+0x01000000*	36,
	0x2200	+0x01000000*	37,
	0x2300	+0x01000000*	38,
	0x2400	+0x01000000*	39,
	0x2440	+0x01000000*	40,
	0x2460	+0x01000000*	41,
	0x2500	+0x01000000*	42,
	0x2580	+0x01000000*	43,
	0x25a0	+0x01000000*	44,
	0x2600	+0x01000000*	45,
	0x2700	+0x01000000*	46,
	0x27c0	+0x01000000*	47,
	0x3000	+0x01000000*	127,
	0x3040	+0x01000000*	48,
	0x30a0	+0x01000000*	49,
	0x3100	+0x01000000*	50,
	0x3130	+0x01000000*	51,
	0x3190	+0x01000000*	52,
	0x31a0	+0x01000000*	53,
	0x3200	+0x01000000*	127,
	0x3300	+0x01000000*	54,
	0x3400	+0x01000000*	55,
	0x4e00	+0x01000000*	127,
	0xa000	+0x01000000*	59,
	0xac00	+0x01000000*	127,
	0xd7b0	+0x01000000*	56,
	0xd800	+0x01000000*	127,
	0xdc00	+0x01000000*	57,
	0xe000	+0x01000000*	58,
	0xf900	+0x01000000*	60,
	0xfb00	+0x01000000*	61,
	0xfb50	+0x01000000*	62,
	0xfe00	+0x01000000*	63,
	0xfe20	+0x01000000*	127,
	0xfe30	+0x01000000*	64,
	0xfe50	+0x01000000*	65,
	0xfe70	+0x01000000*	66,
	0xfeff	+0x01000000*	67,
	0xff00	+0x01000000*	69,
	0xfff0	+0x01000000*	68,
	0x10000	+0x01000000*	69
};

static bool get_tt_info(fc_file_entry *fe, FILE *fp)
{					   
	enum {
		tag_NamingTable = 0x6e616d65L,        /* 'name' */
		tag_HoriHeader  = 0x68686561L,        /* 'hhea' */
		tag_FontHeader  = 0x68656164L,        /* 'head' */
		tag_CmapHeader  = 0x636d6170L,        /* 'cmap' */
		tag_HMetHeader  = 0x686d7478L,        /* 'hmtx' */
		tag_OS2Header   = 0x4f532f32L,        /* 'OS/2' */
		tag_GaspTable   = 0x67617370L         /* 'gasp' */
		};
	typedef long sfnt_TableTag;
	typedef short FWord;
	typedef ushort uFWord;
	typedef struct {
		uint32 bc;
		uint32 ad;
	} BigDate;

	typedef struct {
		sfnt_TableTag       tag;
		uint32              checkSum;
		uint32              offset;
		uint32              length;
	} sfnt_DirectoryEntry;

	typedef struct {
		int32               version;                  /* 0x10000 (1.0) */
		uint16              numOffsets;              /* number of tables */
		uint16              searchRange;             /* (max2 <= numOffsets)*16 */
		uint16              entrySelector;           /* log2 (max2 <= numOffsets) */
		uint16              rangeShift;              /* numOffsets*16-searchRange*/
		sfnt_DirectoryEntry table[1];   /* table[numOffsets] */
	} sfnt_OffsetTable;

	typedef struct {
		uint16              platformID;
		uint16              specificID;
		uint16              languageID;
		uint16              nameID;
		uint16              length;
		uint16              offset;
	} sfnt_NameRecord;

	typedef struct {
		uint16              format;
		uint16              count;
		uint16              stringOffset;
	} sfnt_NamingTable;

	typedef struct {
		ulong		        version; /* for this table, set to 1.0 */
		FWord		        yAscender;
		FWord		        yDescender;
		FWord		        yLineGap; /* linespacing = ascender - descender + linegap */
		uFWord		        advanceWidthMax;	
		FWord		        minLeftSideBearing;
		FWord		        minRightSideBearing;
		FWord		        xMaxExtent; /* Max of ( LSBi + (XMAXi - XMINi) ), for all glyphs */
		int16		        horizontalCaretSlopeNumerator;
		int16		        horizontalCaretSlopeDenominator;
		FWord		        caretOffset;
		uint16		        reserved1;
		uint16		        reserved2;
		uint16		        reserved3;
		uint16		        reserved4;
		int16		        metricDataFormat; /* set to 0 for current format */
		uint16		        numberOf_LongHorMetrics; /* if format == 0 */
	} sfnt_HorizontalHeader;
  	
	typedef struct {
		ulong		        version; /* for this table, set to 1.0 */
		ulong		        fontRevision; /* For Font Manufacturer */
		uint32		        checkSumAdjustment;
		uint32		        magicNumber; /* signature, should always be 0x5F0F3CF5  == MAGIC */
		uint16		        flags;
		uint16		        unitsPerEm; /* Specifies how many in Font Units we have per EM */
		BigDate		        created;
		BigDate		        modified;
		/* This is the font wide bounding box in ideal space
		   (baselines and metrics are NOT worked into these numbers) */
		FWord		        xMin;
		FWord		        yMin;
		FWord		        xMax;
		FWord		        yMax;
		uint16		        macStyle; /* macintosh style word */
		uint16		        lowestRecPPEM; /* lowest recommended pixels per Em */
		/* 0: fully mixed directional glyphs, 1: only strongly L->R or T->B glyphs, 
		   -1: only strongly R->L or B->T glyphs, 2: like 1 but also contains neutrals,
		   -2: like -1 but also contains neutrals */
		int16		        fontDirectionHint;
		int16		        indexToLocFormat;
		int16		        glyphDataFormat;
	} sfnt_FontHeader;
	
	typedef struct {
		ushort		        version; /* for this table, set to 1.0 */
		short				xAvgCharWidth;
		ushort				usWeightClass;
		ushort				usWidthClass;
		short				fsType;
		short				ySubscriptXSize;
		short				ySubscripyYSize;
		short				ySubscripyXOffset;
		short				ySubscripyYOffset;
		short				ySuperscriptXSize;
		short				ySuperscripyYSize;
		short				ySuperscripyXOffset;
		short				ySuperscripyYOffset;
		short				yStrikeoutSize;
		short				yStrikeoutPosition;
		short				sFamilyClass;
		uint8				panose[10];
		uint8				UnicodeRange[16];
		char				achVendID[4];
		ushort				fsSelection;
		ushort				usFirstCharIndex;
		ushort				usLastCharIndex;
		ushort				sTypeAscender;
		ushort				sTypoDescender;
		ushort				sTypeLineGap;
		ushort				usWinAscent;
		ushort				usWinDescent;
		ulong				ulCodePageRange[2];
	} sfnt_OS2Header;
	
	typedef struct {
		ushort				version;
		ushort				numRanges;
	} sfnt_GaspHeader;
	
	typedef struct {
		ushort				rangeMaxPPEM;
		ushort				rangeGaspBehavior;
	} sfnt_GaspEntry;
	
	typedef struct {
		ushort				version;
		ushort				listCount;
	} sfnt_CmapHeader;
	
	typedef struct {
		ushort				platformID;
		ushort				encodingID;
		ulong				offset;
	} sfnt_CmapList;
	
	int 				  i, j, family_name_length=0, style_name_length=0, count=0;
	int					  imin, imax, value;
	char                  name_buffer[128];
	char                  *name;
	char				  *utf8Name;
	bool 			      gotFamily, gotStyle, gotMeasurements, gotHeader;
	bool                  gotHMetric, gotFaces, gotUnicode, gotGasp, monospaced, dualspaced, gotCmap;
  	long		    	  curseek;
  	size_t                readsize;
	short                 ascender=0, descender=0, leading=0;
	float                 converter;
	int32                 w, gCount, seg_count;
	int32                 width[4], cnt[4];
  	ulong                 cTables, family_name_offset=0, style_name_offset=0;
	uint16     	    	  numNames, unitsPerEm=0;
	int16				  xmin=0, xmax=0, ymin=0, ymax=0;
	uint16                *buffer;
	uint16				  fsSelection, macStyle;
	uint16				  *glyphArray;
	uint32				  offsetCmap;
	uint32				  block_mask;
	uint32				  ublocks[4];
	sfnt_CmapList		  cList;
	sfnt_GaspEntry		  *gList;
  	sfnt_OS2Header		  OS2Header;
	fc_cmap_segment		  *segs, *sg;
 	sfnt_CmapHeader		  cHeader;
 	sfnt_FontHeader       fHeader;
  	sfnt_GaspHeader		  gHeader;
  	sfnt_NameRecord       nameRecord;
  	sfnt_OffsetTable      offsetTable;
  	sfnt_NamingTable      namingTable;
  	sfnt_DirectoryEntry   table;
  	sfnt_HorizontalHeader hHeader;

  	/* First off, read the initial directory header on the TTF.  We're only
	   interested in the "numOffsets" variable to tell us how many tables
	   are present in this file.  */
	if (fseek(fp, 0, SEEK_SET) != 0) {
		_bad_font_printf1("##### ERROR ##### Can't seek to begining of file !\n");
		return false;
	}
	if (fread((char *)&offsetTable, 1, sizeof(offsetTable) - sizeof(sfnt_DirectoryEntry), fp) !=
		sizeof(offsetTable) - sizeof(sfnt_DirectoryEntry)) {
		_bad_font_printf1("##### ERROR ##### Can't read offset table !\n");
		return false;
	}
  	cTables = (ushort)fc_read16(&offsetTable.numOffsets);
	
	gotFamily = FALSE;
	gotStyle = FALSE;
	gotMeasurements = FALSE;
	gotHeader = FALSE;
	gotHMetric = FALSE;
	gotFaces = FALSE;
	gotUnicode = FALSE;
	gotGasp = FALSE;
	gotCmap = FALSE;

	monospaced = FALSE;
	dualspaced = FALSE;
	fsSelection = 0;
	gList = NULL;
	gCount = 0;
	offsetCmap = 0;
	
//	_sPrintf("Table: ");
	for (i = 0; (i < cTables) && (i < 40); i++) {
		if (fread((char *)&table, 1, sizeof(table), fp) != sizeof(table)) {
			_bad_font_printf1("##### ERROR ##### Can't read table !\n");
			goto abort_all;
		}
/*		_sPrintf("%c%c%c%c ",
				 ((char*)&table.tag)[0],
				 ((char*)&table.tag)[1],
				 ((char*)&table.tag)[2],
				 ((char*)&table.tag)[3]);*/
		switch (fc_read32(&table.tag)) {
		case tag_NamingTable :	
			if (gotFamily || gotStyle)
				break;
			curseek = ftell(fp);
			if (fseek(fp, fc_read32(&table.offset), SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (naming table) !\n");
				goto abort_all;
			}
			if (fread((char *)&namingTable, 1, sizeof(namingTable), fp) != sizeof(namingTable)) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (naming table) !\n");
				goto abort_all;
			}
			numNames = fc_read16(&namingTable.count);
			while (numNames--) {
				if (fread((char *)&nameRecord, 1, sizeof(nameRecord), fp) != sizeof(nameRecord)) {
					_bad_font_printf1("##### ERROR ##### Unexected EOF (naming table) !\n");
					goto abort_all;
				}
				if (fc_read16(&nameRecord.platformID) == 1) {
					switch (fc_read16(&nameRecord.nameID)) {
					case 1 :
						family_name_offset = fc_read16(&nameRecord.offset) +
							fc_read16(&namingTable.stringOffset) + fc_read32(&table.offset);
						family_name_length = fc_read16(&nameRecord.length);
						gotFamily = TRUE;
						break;
					case 2 :
						style_name_offset = fc_read16(&nameRecord.offset) +
							fc_read16(&namingTable.stringOffset) + fc_read32(&table.offset);
						style_name_length = fc_read16(&nameRecord.length);
						gotStyle = TRUE;
						break;
					}
				}
			}
			if (fseek(fp, curseek, SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Can't seek back (naming table) !\n");
				goto abort_all;
			}
			break;
			
		case tag_HoriHeader :
			if (gotMeasurements) break;
			curseek = ftell(fp);
			if (fseek(fp, fc_read32(&table.offset), SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (horizontal header) !\n");
				goto abort_all;
			}
			if (fread((char *)&hHeader, 1, sizeof(hHeader), fp) != sizeof(hHeader)) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (horizontal header) !\n");
				goto abort_all;
			}
			ascender = (short)fc_read16(&hHeader.yAscender);
			descender = -((short)fc_read16(&hHeader.yDescender));
			leading = (short)fc_read16(&hHeader.yLineGap);
			count = fc_read16(&hHeader.numberOf_LongHorMetrics);
			gotMeasurements = TRUE;
			if (fseek(fp, curseek, SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Can't seek back (horizontal header) !\n");
				goto abort_all;
			}
			/* replay again */
			if (gotHMetric) {
				gotHMetric = FALSE;
				if (fseek(fp, 0, SEEK_SET) != 0) {
					_bad_font_printf1("##### ERROR ##### Unexected EOF (horizontal header) !\n");
					goto abort_all;
				}
				if (fread((char *)&offsetTable, 1,
					sizeof(offsetTable) - sizeof(sfnt_DirectoryEntry), fp) !=
					sizeof(offsetTable) - sizeof(sfnt_DirectoryEntry)) {
					_bad_font_printf1("##### ERROR ##### Unexected EOF (horizontal header) !\n");
					goto abort_all;
				}
				i = -1;
			}
			break;

		case tag_HMetHeader :
			if (gotHMetric)
				break;
			if (!gotMeasurements) {
				gotHMetric = TRUE;
				break;
			}
			curseek = ftell(fp);
			// Note that we do -not- completely fail on error here, since
			// this information is optional.
			if (fseek(fp, fc_read32(&table.offset), SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (HMetric header) !\n");
				goto skip_HMetric;
			}
			buffer = (uint16*)grMalloc(4*count,"fc:buffer",MALLOC_CANNOT_FAIL);
			if (fread((char *)buffer, 1, 4*count, fp) != 4*count) {
				grFree(buffer);
				_bad_font_printf1("##### ERROR ##### Unexected EOF (HMetric header) !\n");
				goto skip_HMetric;
			}
			monospaced = TRUE;
			dualspaced = TRUE;
			for (j=0; j<4; j++) {
				width[j] = -1;
				cnt[j] = 0;
			}
			for (j=1; j<count; j++) {
				w = buffer[2*j];
				if (w == 0)
					continue;
				else if (w == width[0])
					cnt[0]++;
				else if (w == width[1])
					cnt[1]++;
				else if (w == width[2])
					cnt[2]++;
				else if (width[0] == -1)
					width[0] = w;
				else if (width[1] == -1)
					width[1] = w;
				else if (width[2] == -1)
					width[2] = w;
				else
					cnt[3]++;
			}
			if (cnt[0] < cnt[1])
				count = cnt[0];
			else
				count = cnt[1];
			count += cnt[2]+cnt[3];
			if (count > 1)
				monospaced = FALSE;
			if (count != 2)
				dualspaced = FALSE;
			grFree(buffer);
			gotHMetric = TRUE;
		skip_HMetric:
			if (fseek(fp, curseek, SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Can't seek back (HMetric header) !\n");
				goto abort_all;
			}
			break;

		case tag_FontHeader :
			if (gotHeader)
				break;
			curseek = ftell(fp);
			if (fseek(fp, fc_read32(&table.offset), SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (Font header) !\n");
				goto abort_all;
			}
			if (fread((char *)&fHeader, 1, sizeof(fHeader), fp) != sizeof(fHeader)) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (Font header) !\n");
				goto abort_all;
			}
			unitsPerEm = fc_read16(&fHeader.unitsPerEm);
			xmin = (int16)fc_read16(&fHeader.xMin);
			xmax = (int16)fc_read16(&fHeader.xMax);
			ymin = (int16)fc_read16(&fHeader.yMin);
			ymax = (int16)fc_read16(&fHeader.yMax);
			macStyle = fc_read16(&fHeader.macStyle);
			if (macStyle & 1)
				fsSelection |= (1<<5);
			if (macStyle & 2)
				fsSelection |= 1;
			gotHeader = TRUE;
			if (fseek(fp, curseek, SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Can't seek back (Font header) !\n");
				goto abort_all;
			}
			break;
		
		case tag_OS2Header :
			if (gotFaces && gotUnicode)
				break;
			// Note that we do -not- completely fail on error here, since
			// this information is optional.
			curseek = ftell(fp);
			if (fseek(fp, fc_read32(&table.offset), SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (OS2 table) !\n");
				goto skip_OS2Header;
			}
			// Some font files seem to have a truncated OS/2 header, so we
			// will deal with that here.
			if ((readsize=fread((char *)&OS2Header, 1, sizeof(OS2Header), fp)) != sizeof(OS2Header)) {
				_bad_font_printf3("WARNING: Truncated data (OS2 table got %ld bytes, expected %ld) !\n",
									readsize, sizeof(OS2Header));
			}
			if (readsize >= SIZE_TO(sfnt_OS2Header, OS2Header, fsSelection)) {
				fsSelection |= fc_read16(&OS2Header.fsSelection);
				gotFaces = TRUE;
			}
			if (readsize >= SIZE_TO(sfnt_OS2Header, OS2Header, UnicodeRange)) {
				ublocks[0] = fc_read32(&OS2Header.UnicodeRange[0]);
				ublocks[1] = fc_read32(&OS2Header.UnicodeRange[4]);
				ublocks[2] = fc_read32(&OS2Header.UnicodeRange[8]);
				ublocks[3] = fc_read32(&OS2Header.UnicodeRange[12]);
				gotUnicode = TRUE;
			}
		skip_OS2Header:
			if (fseek(fp, curseek, SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Can't seek back (OS2 table) !\n");
				goto abort_all;
			}
			break;

		case tag_GaspTable :
			if (gotGasp)
				break;
			// Note that we do -not- completely fail on error here, since
			// this information is optional.
			curseek = ftell(fp);
			if (fseek(fp, fc_read32(&table.offset), SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (GASP table) !\n");
				goto skip_Gasp;
			}
			if (fread((char *)&gHeader, 1, sizeof(gHeader), fp) != sizeof(gHeader)) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (GASP table) !\n");
				goto skip_Gasp;
			}
			gCount = fc_read16(&gHeader.numRanges);
			if ((gCount < 0) || (gCount > 16)) {
				_bad_font_printf1("##### ERROR ##### Too many entries (GASP table) !\n");
				goto skip_Gasp;
			}
			gList = (sfnt_GaspEntry*)grMalloc(sizeof(sfnt_GaspEntry)*gCount,"fc:gList",MALLOC_CANNOT_FAIL);
			if (gList == NULL) {
				_bad_font_printf1("##### ERROR ##### Malloc failed (GASP table) !\n");
				goto skip_Gasp;
			}
			if (fread((char *)gList, 1, sizeof(sfnt_GaspEntry)*gCount, fp) != sizeof(sfnt_GaspEntry)*gCount) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (GASP table) !\n");
				goto skip_Gasp;
			}
			gotGasp = TRUE;
		skip_Gasp:
			if (fseek(fp, curseek, SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Can't seek back (GASP table) !\n");
				goto abort_all;
			}
			break;

		case tag_CmapHeader :
			if (gotCmap)
				break;
			curseek = ftell(fp);
			// Note that we do -not- completely fail on error here, since
			// this information is optional.
			if (fseek(fp, fc_read32(&table.offset), SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (CMAP table) !\n");
				goto skip_Cmap;
			}
			if (fread((char *)&cHeader, 1, sizeof(cHeader), fp) != sizeof(cHeader)) {
				_bad_font_printf1("##### ERROR ##### Unexected EOF (CMAP table) !\n");
				goto skip_Cmap;
			}
			count = fc_read16(&cHeader.listCount);
			for (j=0; j<count; j++) {
				if (fread((char *)&cList, 1, sizeof(cList), fp) != sizeof(cList)) {
					_bad_font_printf1("##### ERROR ##### Unexected EOF (CMAP table) !\n");
					goto skip_Cmap;
				}
				if ((fc_read16(&cList.platformID) == 3) && (fc_read16(&cList.encodingID) == 1)) {
					offsetCmap = fc_read32(&cList.offset)+fc_read32(&table.offset);
					break;
				}
			}
 			gotCmap = TRUE;
 		skip_Cmap:
			if (fseek(fp, curseek, SEEK_SET) != 0) {
				_bad_font_printf1("##### ERROR ##### Can't seek back (CMAP table) !\n");
				goto abort_all;
			}
			break;
		}		
		
		if (gotFamily && gotStyle && gotMeasurements && gotHeader &&
			gotHMetric && gotFaces && gotUnicode && gotGasp && gotCmap)
			break;
	}

	if (gotFamily && gotStyle && gotMeasurements && gotHeader) {
		/* allocate name buffer if necessary */
		name = name_buffer;
		if ((family_name_length > sizeof(name_buffer)) || (style_name_length > sizeof(name_buffer))) {
			if (family_name_length > style_name_length)
				name = (char*)grMalloc(family_name_length+1,"fc:name",MALLOC_CANNOT_FAIL);
			else
				name = (char*)grMalloc(style_name_length+1,"fc:name",MALLOC_CANNOT_FAIL);
		}
		/* copy metric infos */
		converter = 1.0/(float)unitsPerEm;
		fe->ascent = converter*(float)ascender;
		fe->descent = converter*(float)descender;
		fe->leading = converter*(float)leading;
		fe->bbox[0] = converter*(float)xmin;
		fe->bbox[1] = converter*(float)ymin;
		fe->bbox[2] = converter*(float)xmax;
		fe->bbox[3] = converter*(float)ymax;
		if (monospaced)
			fe->flags = FC_MONOSPACED_FONT;
		else
			fe->flags = 0;
		if (dualspaced)
			fe->flags |= FC_DUALSPACED_FONT;
		fe->faces = fsSelection;
		fe->size = 10.0;		/* meaningless value, just to make the bounds checker
		                           happy */
		/* store Gasp information, if any */
		if (gCount == 0)
			goto default_gasp;
		if (fc_read16(&gList[gCount-1].rangeMaxPPEM) < 2048)
			goto default_gasp;
		count = gCount;
		if (count > 4)
			count = 4;
		for (i=0; i<count; i++) {
			fe->gasp[i*2+0] = fc_read16(&gList[i].rangeMaxPPEM);
			fe->gasp[i*2+1] = fc_read16(&gList[i].rangeGaspBehavior);
		}
		for (i=count-1; i<4; i++)
			fe->gasp[i*2] = 0xffff;
		for (i=count; i<4; i++)
			fe->gasp[i*2+1] = 0x0;
	default_gasp:
//	_sPrintf("GASP: %04x %04x %04x %04x %04x %04x %04x %04x\n",
//			 fe->gasp[0], fe->gasp[1], fe->gasp[2], fe->gasp[3], 
//			 fe->gasp[4], fe->gasp[5], fe->gasp[6], fe->gasp[7]);
		/* check and scan the cmap table */
		if (gotUnicode && ((ublocks[0]|ublocks[1]|ublocks[2]) != 0)) {
			fe->blocks[0] = ublocks[0];
			fe->blocks[1] = ublocks[1];
			fe->blocks[2] = ublocks[2];
		}
		
		//printf("Font %s: offsetCmap=%ld\n", fe->filename, offsetCmap);
		if (offsetCmap != 0 &&
				load_cmap_table(fp, offsetCmap, &segs, &seg_count, &glyphArray)) {
			uint32* bitmap = (uint32*)
				grMalloc(sizeof(fc_block_bitmap)*256,"fc:temp block_bitmap",MALLOC_MEDIUM);
			if (bitmap) {
				// first turn the character code segments into a raw bitmap.
				memset(bitmap, 0, sizeof(fc_block_bitmap)*256);
				sg = segs;
				//printf("Building raw character map...\n");
				for (j=0; j<seg_count; j++) {
					if (sg->idRangeOffset == 0) {
						const uint16 startEntry = sg->startCount/32;
						const uint16 endEntry = sg->endCount/32;
						//printf("Character range %d-%d (block %d-%d)\n",
						//		sg->startCount, sg->endCount,
						//		sg->startCount/0x100, sg->endCount/0x100);
						for (value=startEntry; value<=endEntry; value++) {
							uint32 bits = 0xFFFFFFFF;
							if (value == startEntry)
								bits &= 0xFFFFFFFF << (sg->startCount&31);
							if (value == endEntry)
								bits &= 0xFFFFFFFF >> (31-(sg->endCount&31));
							bitmap[value] |= bits;
							//printf("Setting characters at %d to 0x%lx; entry %d is now: 0x%08lx\n",
							//		value*32, bits, value, bitmap[value]);
						}
					}
					else {
						//printf("Character map %d-%d (block %d-%d)\n",
						//		sg->startCount, sg->endCount,
						//		sg->startCount/0x100, sg->endCount/0x100);
						for (i=0; i<=(sg->endCount-sg->startCount); i++) {
							value = i+sg->startCount;
							if (glyphArray[sg->idRangeOffset+i] != 0)
								bitmap[value/32] |= 1<<(value&31);
							else
								bitmap[value/32] &= ~(1<<(value&31));
							//if ((value&31) == 31 || value == sg->endCount) {
							//	printf("Bits at %d are now: 0x%08lx\n",
							//			value/32, bitmap[value/32]);
							//}
						}
					}
					sg++;
				}
				
				fc_set_char_map(fe, bitmap);
				grFree(bitmap);
			}
			grFree(segs);
			if (glyphArray != NULL)
				grFree(glyphArray);
		}

		/* read family name */
		if (fseek(fp, family_name_offset, SEEK_SET) != 0) {
			_bad_font_printf1("##### ERROR ##### Error seeking for family name !\n");
			goto bad_exit_free_name;
		}
		if (fread((char *)name, 1, family_name_length, fp) != family_name_length) {
			_bad_font_printf1("##### ERROR ##### Error reading family name !\n");
			goto bad_exit_free_name;
		}
		name[family_name_length] = 0;
		utf8Name = fc_convert_font_name_to_utf8(name, family_name_length, B_FONT_FAMILY_LENGTH,
												fc_read16(&nameRecord.platformID), 
												fc_read16(&nameRecord.languageID));
		fe->family_id = fc_register_family(utf8Name, "TT");
		grFree(utf8Name);
		/* read style name */
		if (fseek(fp, style_name_offset, SEEK_SET) != 0) {
			_bad_font_printf1("##### ERROR ##### Error seeking for style name !\n");
			goto bad_exit_free_name;
		}
		if (fread((char *)name, 1, style_name_length, fp) != style_name_length) {
			_bad_font_printf1("##### ERROR ##### Error reading style name !\n");
			goto bad_exit_free_name;
		}
		name[style_name_length] = 0;
		utf8Name = fc_convert_font_name_to_utf8(name, style_name_length, B_FONT_STYLE_LENGTH,
												fc_read16(&nameRecord.platformID), 
												fc_read16(&nameRecord.languageID));
		fe->style_id = fc_set_style(fe->family_id, utf8Name, fe-file_table,
									fe->flags, fe->faces, fe->gasp, TRUE);
		grFree(utf8Name);
		if (fe->style_id == 0xffff) {
			fc_remove_style(fe->family_id, -1);
	bad_exit_free_name:
			if (name != name_buffer)
				grFree(name);
			goto abort_all;
		}
		/* free name buffer if necessary */
		if (name != name_buffer)
			grFree(name);
		if (gList != NULL)
			grFree(gList);
		return true;
	}
abort_all:
	if (gList != NULL)
		grFree(gList);
	return false;
}

void fc_set_char_map(fc_file_entry* fe, const uint32* bitmap, bool update_unicode_blocks)
{
	int32 value;
	int32 num_unique = 0;
	uint8 raw_index[256];
	
	if (fe->block_bitmaps != NULL)
		grFree(fe->block_bitmaps);
	fe->block_bitmaps = NULL;
	
	// first compress block entries in the bitmap by removing
	// duplicate items, and create the index for each block.
	//printf("Building block index...\n");
	for (value=0; value<(0x10000/32); value+=(0x100/32)) {
		bool match = false;
		for (int32 prev=0; prev<value; prev+=(0x100/32)) {
			match = true;
			for (int32 i=0; i<(0x100/32); i++) {
				if (bitmap[value+i] != bitmap[prev+i]) {
					match = false;
					break;
				}
			}
			if (match) {
				fe->char_blocks[value/(0x100/32)] = fe->char_blocks[prev/(0x100/32)];
				break;
			}
		}
		if (!match) {
			raw_index[num_unique] = value/(0x100/32);
			fe->char_blocks[value/(0x100/32)] = num_unique++;
		}
		//printf("Block %ld is at index %d\n",
		//		value/(0x100/32), fe->char_blocks[value/(0x100/32)]);
	}
	
	// now store the raw bitmaps.
	fe->block_bitmaps = (fc_block_bitmap*)
		grMalloc(sizeof(fc_block_bitmap)*num_unique, "fc:block_bitmap", MALLOC_MEDIUM);
	if (fe->block_bitmaps) {
		//printf("Building block bitmap with %ld entries\n", num_unique);
		for (value=0; value<num_unique; value++) {
			//printf("Copy from offset %d to offset %ld\n",
			//		raw_index[value]*(0x100/8),
			//		value*sizeof(fc_block_bitmap));
			memcpy(fe->block_bitmaps+value,
					bitmap+(raw_index[value]*(0x100/32)),
					sizeof(fc_block_bitmap));
		}
	}
	
	// finally, if requested, update the unicode blocks from this character map.
	if (update_unicode_blocks) {
		//printf("Setting unicode blocks...\n");
		fe->blocks[0] = fe->blocks[1] = fe->blocks[2];
		int32 last_code = 0;
		for (int32 i=1; i<BLOCK_TABLE_COUNT; i++) {
			const int32 block = block_dispatch_table[i]>>24;
			const int32 code = block_dispatch_table[i] & 0xffffff;
			if (block < 127) {
				const bool has = fc_has_glyph_range(fe, last_code, code-1);
				//printf("Block #%ld (%ld-%ld): has=%d\n", block, last_code, code-1, has);
				if (has)
					fe->blocks[block/32] |= 1<<(block&31);
			}
			last_code = code;
		}
	}
	
}

bool fc_has_glyph_range(fc_file_entry *fe, uint16 first, uint16 last)
{
	if (!fe->block_bitmaps) return false;
	
	const uint16 firstVal = first/32;
	const uint16 lastVal = last/32;
	for (int32 i=firstVal; i<=lastVal; i++) {
		uint32 mask = 0xFFFFFFFF;
		if (i == firstVal)
			mask &= 0xFFFFFFFF << (first&31);
		if (i == lastVal)
			mask &= 0xFFFFFFFF >> (31-(last&31));
		if (fe->block_bitmaps[fe->char_blocks[i/(0x100/32)]][i&(0xff/32)] & mask)
			return true;
	}
	return false;
}

static bool get_tuned_info(fc_file_entry *fe, FILE *fp) {
	int                  family_length, style_length, length, size, bpp, hmask;
	char                 *names;
	uchar                *data;
	float                angles[2]; 
	fc_tuned_entry       te;
	fc_tuned_file_header tfh;

	names = 0L;
	data = 0L;
	/* read header */
	if (fseek(fp, 4, SEEK_SET) != 0) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : Unexpected EOF !\n");
		goto exit_false;
	}
	if (fread(&tfh, 1, sizeof(fc_tuned_file_header), fp) != sizeof(fc_tuned_file_header)) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : Unexpected EOF !\n");
		goto exit_false;
	}
	/* read names length */
	family_length = fc_read16(&tfh.family_length);
	style_length = fc_read16(&tfh.style_length);
	if ((family_length < 0) || (family_length > B_FONT_FAMILY_LENGTH) ||
		(style_length < 0) || (style_length > B_FONT_STYLE_LENGTH)) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : invalid name length !\n");
		goto exit_false;
	}
	/* read parameters of the header */
	((int32*)angles)[0] = fc_read32(&tfh.rotation);
	((int32*)angles)[1] = fc_read32(&tfh.shear);
 	hmask = fc_read32(&tfh.hmask);
	if (((hmask&(hmask+1)) != 0) || (hmask < 3)) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : invalid hmask !\n");
		goto exit_false;
	}
	size = fc_read16(&tfh.size);
	if ((size <= 1) || (size > FC_BW_MAX_SIZE)) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : invalid size !\n");
		goto exit_false;
	}
	bpp = tfh.bpp;
	if ((bpp != FC_GRAY_SCALE) && (bpp != FC_TV_SCALE) && (bpp != FC_BLACK_AND_WHITE)) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : invalid bpp !\n");
		goto exit_false;
	}
	if (tfh.version != 0) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : invalid version !\n");
		goto exit_false;
	}
	/* read family and style names */
	length = family_length+style_length+2;
	names = (char*)grMalloc(length,"fc:tuned_info:names",MALLOC_CANNOT_FAIL);
	if (fread(names, 1, length, fp) != length) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : Unexpected EOF !\n");
		goto exit_false;
	}
	if ((names[family_length] != 0) || (names[style_length+family_length+1] != 0)) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : invalid names !\n");
		goto exit_false;
	}
	if ((strlen(names+0) != family_length) || (strlen(names+family_length+1) != style_length)) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : invalid names !\n");
		goto exit_false;
	}
	/* the font is okay. Register it */
	te.file_id = fe-file_table;
	te.offset = 4+sizeof(fc_tuned_file_header)+length;
	te.rotation = angles[0];
	te.shear = angles[1];
	te.hmask = hmask;
	te.size = size;
	te.bpp = bpp;
	
	fe->family_id = fc_register_family(names, "TT");
	fe->style_id = fc_add_sub_style(fe->family_id, names+family_length+1, &te);
	fe->size = size;
	
	if (fe->style_id == 0xffff) {
		_bad_font_printf1("##### ERROR ##### Tuned Font : invalid style !\n");
		goto exit_false;
	}
	
	grFree(names);
	grFree(data);
	return TRUE;
	
 exit_false:
	if (names != 0L)
		grFree(names);
	if (data != 0L)
		grFree(data);
	return FALSE;
}

bool fc_init_file_entry(fc_file_entry *fe) {		
	char         fullname[PATH_MAX];
	FILE         *fp;
	int32		 index;
	uchar        tag[1024];
	struct stat  stat_buf;

	/* initialize to default values */
	fe->family_id = 0xffff;
	fe->style_id = 0xffff;
	fe->flags = 0;
	fe->ascent = 0.8;
	fe->descent = 0.2;
	fe->leading = 0.1;
	fe->size = 10;
	fe->faces = 0;
	fe->gasp[0] = 0x0008;
	fe->gasp[1] = 0x0002;
	fe->gasp[2] = 0xffff;
	fe->gasp[3] = 0x0003;
	fe->gasp[4] = 0xffff;
	fe->gasp[5] = 0x0000;
	fe->gasp[6] = 0xffff;
	fe->gasp[7] = 0x0000;
	fe->bbox[0] = 0.0;
	fe->bbox[1] = 0.0;
	fe->bbox[2] = 1.0;
	fe->bbox[3] = 1.0;
	
	if (fe->block_bitmaps)
		grFree(fe->block_bitmaps);
	fe->block_bitmaps = NULL;
	if (fe->char_ranges)
		grFree(fe->char_ranges);
	fe->char_ranges = NULL;
	
	/* get the complete access name */
	memset(tag, 0, 1024);
	fc_get_pathname(fullname, fe);
	/* open/create the file */
	if ((fp = fopen(fullname, "rb")) == 0L) {
		if ((fp = fopen(fullname, "wb")) == 0L)
			return FALSE;
		fe->file_type = FC_BAD_FILE;		
	}
	/* try to read the first 4 bytes */
	else if (fread((char *)tag, 1, 1024, fp) < 4)
		fe->file_type = FC_BAD_FILE;
	/* check for type 1 format */
	else if (find_ps_header((char*)tag, 1024, &index)) {
		if (get_ps_info(fe, fp, index))
			fe->file_type = FC_TYPE1_FONT;
		else
			fe->file_type = FC_BAD_FILE;
	}
	/* look for a tuned bitmap font file */
	else if ((tag[0] == '|') && (tag[1] == 'B') && (tag[2] == 'e') && (tag[3] == ';')) {
		if (get_tuned_info(fe, fp))
			fe->file_type = FC_BITMAP_FONT;
		else
			fe->file_type = FC_BAD_FILE;
	}
	/* Stroke fonts look like TTF files sometimes, so check the filename explicitly. */
	else if (strcasecmp(fullname + strlen(fullname) - 4, ".ffs") == 0 && get_stroke_info(fe, fp))
		fe->file_type = FC_STROKE_FONT;
	else if (get_tt_info(fe, fp))
		fe->file_type = FC_TRUETYPE_FONT;
	else 
		fe->file_type = FC_BAD_FILE;
	fclose(fp);
	if (stat(fullname, &stat_buf) == 0)
		fe->mtime = stat_buf.st_mtime;
	else
		return FALSE;

	return TRUE;
}
