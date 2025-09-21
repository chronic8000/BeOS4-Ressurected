#include "ntfs.h"

uint64 ntfs_time_to_posix_time(uint64 posix_time)
{
	// Move to 1970 time
	posix_time -= 0x019db1ded53e8000;
	
	// Divide by 10000000
	posix_time /= 10000000;
	
	return posix_time;
}

#define toupper_unicode(u) (ntfs->upcase[(u)])

// This function does a strcasecmp using unicode text. It also uses the
// upcase table that comes with every volume to handle the toupper function.
// The second string is assumed to be in Little Endian format, since it's
// coming straight from the disk in the application of this function.
int ntfs_strcasecmp_unicode(ntfs_fs_struct *ntfs, uint16 *a, int a_len, uint16 *b, int b_len)
{
	int i;

//	dprintf("ntfs_strcasecmp_unicode comparison:\n");		
	for(i=0; i<(a_len/2) && i<(b_len/2); i++) {
//		dprintf("\ta[i] = %d b[i] = %d", a[i], B_LENDIAN_TO_HOST_INT16(b[i]));
//		dprintf("\t%d (%c) to %d (%c)\n", toupper_unicode(a[i]), toupper_unicode(a[i]), toupper_unicode(B_LENDIAN_TO_HOST_INT16(b[i])), toupper_unicode(B_LENDIAN_TO_HOST_INT16(b[i])));
		if(toupper_unicode(a[i]) < toupper_unicode(B_LENDIAN_TO_HOST_INT16(b[i]))) return -1;
		if(toupper_unicode(a[i]) > toupper_unicode(B_LENDIAN_TO_HOST_INT16(b[i]))) return 1;
	}
	
//	dprintf("\ta_len = %d b_len = %d\n", a_len, b_len);
	
	if(a_len < b_len) return -1;
	if(a_len > b_len) return 1;	
	
	return 0;
}



// This function does a unicode strcmp. 
// The second string is assumed to be in Little Endian format, since it's
// coming straight from the disk in the application of this function.
int ntfs_strcmp_unicode(uint16 *a, int a_len, uint16 *b, int b_len)
{
	int i;

//	dprintf("ntfs_strcmp_unicode comparison:\n");		
	for(i=0; i<(a_len/2) && i<(b_len/2); i++) {
//		dprintf("\ta[i] = %d to b[i] = %d\n", a[i], B_LENDIAN_TO_HOST_INT16(b[i]));
		if(a[i] < B_LENDIAN_TO_HOST_INT16(b[i])) return -1;
		if(a[i] > B_LENDIAN_TO_HOST_INT16(b[i])) return 1;
	}
	
	if(a_len < b_len) return -1;
	if(a_len > b_len) return 1;	
	
	return 0;
}

// From EncodingComversions.cpp

// Pierre's Uber Macro
#define u_to_utf8(str, uni_str)\
{\
	if ((B_LENDIAN_TO_HOST_INT16(uni_str[0])&0xff80) == 0)\
		*str++ = B_LENDIAN_TO_HOST_INT16(*uni_str++);\
	else if ((B_LENDIAN_TO_HOST_INT16(uni_str[0])&0xf800) == 0) {\
		str[0] = 0xc0|(B_LENDIAN_TO_HOST_INT16(uni_str[0])>>6);\
		str[1] = 0x80|(B_LENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 2;\
	} else if ((B_LENDIAN_TO_HOST_INT16(uni_str[0])&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(B_LENDIAN_TO_HOST_INT16(uni_str[0])>>12);\
		str[1] = 0x80|((B_LENDIAN_TO_HOST_INT16(uni_str[0])>>6)&0x3f);\
		str[2] = 0x80|(B_LENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((B_LENDIAN_TO_HOST_INT16(uni_str[0])-0xd7c0)<<10) | (B_LENDIAN_TO_HOST_INT16(uni_str[1])&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}	

// Another Uber Macro
#define utf8_to_u(str, uni_str, err_flag) \
{\
	err_flag = 0;\
	if ((str[0]&0x80) == 0)\
		*uni_str++ = *str++;\
	else if ((str[1] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=1;\
	} else if ((str[0]&0x20) == 0) {\
		*uni_str++ = ((str[0]&31)<<6) | (str[1]&63);\
		str+=2;\
	} else if ((str[2] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=2;\
	} else if ((str[0]&0x10) == 0) {\
		*uni_str++ = ((str[0]&15)<<12) | ((str[1]&63)<<6) | (str[2]&63);\
		str+=3;\
	} else if ((str[3] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=3;\
	} else {\
		err_flag = 1;\
	}\
}

// Count the number of bytes of a UTF-8 character
#define utf8_char_len(c) (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1) 

status_t ntfs_unicode_to_utf8(const char *src, int32 *srcLen, char *dst, int32 *dstLen)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

/*
	{
		int i;
		uint16 *in = (uint16 *)src;

		dprintf("ntfs_unicode_to_utf8 entry\n");
		dprintf("src (%d):", *srcLen);
		for(i=0; i<(*srcLen/2); i++) dprintf(" %04x", (uint16 *)in[i]);
		dprintf("\n");
	}
*/	
	for (srcCount = 0; srcCount < srcLimit; srcCount += 2) {
		uint16  *UNICODE = (uint16 *)&src[srcCount];
		uchar	utf8[4];
		uchar	*UTF8 = utf8;
		int32 utf8Len;
		int32 j;

		u_to_utf8(UTF8, UNICODE);

		utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

/*
	{
		int i;

		dprintf("dst (%d):", *dstLen);
		for(i=0; i<*dstLen; i++) dprintf(" %02x", dst[i]);
		dprintf(" (");
		for(i=0; i<*dstLen; i++) dprintf("%c", dst[i]);
		dprintf(")\n");
	}
*/

	return ((dstCount > 0) ? B_NO_ERROR : B_ERROR);
}

status_t 
ntfs_utf8_to_unicode(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen - 1;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while ((srcCount < srcLimit) && (dstCount < dstLimit)) {
		uint16	unicode;
		uint16	*UNICODE = &unicode;
		uchar	*UTF8 = (uchar *)src + srcCount;
		int     err_flag;

		if ((srcCount + utf8_char_len(src[srcCount])) > srcLimit)
			break; 

		utf8_to_u(UTF8, UNICODE, err_flag);
		if(err_flag == 1) {
			ERRPRINT(("ntfs: >2 byte unicode string found.\n"));
			return EINVAL;
		}

		unicode = B_LENDIAN_TO_HOST_INT16(unicode);

		dst[dstCount++] = unicode & 0xFF;
		dst[dstCount++] = unicode >> 8;		

		srcCount += UTF8 - ((uchar *)(src + srcCount));
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : B_ERROR);
}
