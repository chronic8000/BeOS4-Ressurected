#ifndef _FONT_DATA_H
#define _FONT_DATA_H

#ifdef __cplusplus
extern "C" {
#endif
/* macro to convert a uchar* utf8_string into a ushort* uni_string,
   one character at a time. Move the pointer. You can check terminaison on
   the string by testing : if (string[0] == 0) */
#define UTF8_2_UNICODE(str, uni_str) \
{\
	if ((str[0]&0x80) == 0)\
		*uni_str++ = *str++;\
	else if ((str[1] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str++;\
	} else if ((str[0]&0x20) == 0) {\
		*uni_str++ = ((str[0]&31)<<6) | (str[1]&63);\
		str += 2;\
	} else if ((str[2] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=2;\
	} else if ((str[0]&0x10) == 0) {\
		*uni_str++ = ((str[0]&15)<<12) | ((str[1]&63)<<6) | (str[2]&63);\
		str += 3;\
	} else if ((str[3] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=3;\
	} else {\
		int   val;\
		val = ((str[0]&7)<<18) | ((str[1]&63)<<12) | ((str[2]&63)<<6) | (str[3]&63);\
		uni_str[0] = 0xd7c0+(val>>10);\
		uni_str[1] = 0xdc00+(val&0x3ff);\
		uni_str += 2;\
		str += 4;\
	}\
}

/* macro to convert a ushort* uni_string into a char* or uchar* utf8_string,
   one character at a time. Move the pointer. You can check terminaison on
   the uni_string by testing : if (uni_string[0] == 0) */
#define UNICODE_2_UTF8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|((*uni_str++)&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|((*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}	

enum {
	FC_SYMBOL_SET_COUNT = 12,
	FC_LANGUAGE_COUNT = 61
};

extern unsigned short b_language2symbol_set[FC_LANGUAGE_COUNT];
extern unsigned short *b_symbol_set_table[FC_SYMBOL_SET_COUNT];

#ifdef __cplusplus
}
#endif

#endif














