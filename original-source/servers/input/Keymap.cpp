#include "Keymap.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#ifdef __INTEL__
#include <encoding_tables.h>
#else
extern "C" uint16 *b_symbol_set_table[];
#endif
#include <Debug.h>
#include <FindDirectory.h>
#include <Path.h>


// defines
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


const uint32 kFF = (uint32)-1;

uint32 defaultTables[] = {
// Version...
	2,

// Modifier key table...

/* caps lock key: */		59,
/* scroll lock key: */		15,
/* num lock key: */			34,
/* left shift key: */		75,
/* right shift key: */		86,
/* left command key: */		93,
/* right command key: */	95,
/* left control key: */		92,
/* right control key: */	0,
/* left option key: */		102,
/* right option key: */		96,
/* menu key: */				104,
/* lock setting: */			0,

// Control table...
/*      -0- -1- -2- -3- -4- -5- -6- -7- -8- -9- -a- -b- -c- -d- -e- -f-   */

/* 0 */kFF, 27, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
/* 1 */ 16,kFF,kFF,  0,kFF,kFF,kFF, 30,kFF,kFF,kFF,kFF, 31,kFF,127,  5,
/* 2 */  1, 11,kFF,'/','*','-',  9, 17, 23,  5, 18, 20, 25, 21,  9, 15,
/* 3 */ 16, 27, 29, 28,127,  4, 12,  1, 30, 11,'+',kFF,  1, 19,  4,  6,
/* 4 */  7,  8, 10, 11, 12,kFF,kFF, 10, 28,kFF, 29,kFF, 26, 24,  3, 22,
/* 5 */  2, 14, 13,kFF,kFF,kFF,kFF, 30,  4, 31, 12, 10,kFF,kFF,  0,kFF,
/* 6 */kFF, 28, 31, 29,  5,127,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,
/* 7 */kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,200,202,

// Option-Caps-Shift table...
/*      -0-kFF- -2- -3- -4- -5- -6- -7- -8- -9- -a- -b- -c- -d- -e- -f-   */

/* 0 */kFF, 27, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
/* 1 */ 16,'`',kFF,kFF,kFF,kFF,kFF,246,164,161,187,188,209,177,  8,  5,
/* 2 */  1, 11,kFF,'/','*','-',  9,207,kFF,171,kFF,224,kFF,kFF,kFF,191,
/* 3 */185,210,211,kFF,127,  4, 12,'7','8','9','+',kFF,140,kFF,kFF,kFF,
/* 4 */kFF,kFF,kFF,kFF,190,172,247, 10,'4','5','6',kFF,kFF,kFF,141,kFF,
/* 5 */kFF,150,kFF,178,179,192,kFF, 30,'1','2','3', 10, (uint32)-2,kFF,202,kFF,
/* 6 */kFF, 28, 31, 29,'0','.',kFF,kFF,kFF,'|',kFF,kFF,kFF,kFF,kFF,kFF,
/* 7 */kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,200,202,

// Option-Caps table...
/*      -0-kFF- -2- -3- -4- -5- -6- -7- -8- -9- -a- -b- -c- -d- -e- -f-   */

/* 0 */kFF, 27, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
/* 1 */ 16,'`',193,170,163,162,176,246,166,165,199,200,208,173,  8,  5,
/* 2 */  1, 11,kFF,'/','*','-',  9,206,183,171,168,160,180,kFF,kFF,175,
/* 3 */184,212,213,194,127,  4, 12,  1, 30, 11,'+',kFF,203,186,182,196,
/* 4 */169,kFF,198,215,174,172,247, 10, 28,kFF, 29,kFF,189,197,130,195,
/* 5 */167,132,181,kFF,201,214,kFF, 30,  4, 31, 12, 10,kFF,kFF,202,kFF,
/* 6 */kFF, 28, 31, 29,  5,127,kFF,kFF,kFF, 92,kFF,kFF,kFF,kFF,kFF,kFF,
/* 7 */kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,200,202,

// Option-Shift table...
/*      -0-kFF- -2- -3- -4- -5- -6- -7- -8- -9- -a- -b- -c- -d- -e- -f-   */

/* 0 */kFF, 27, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
/* 1 */ 16,'`',kFF,kFF,kFF,kFF,228,246,164,161,187,188,209,177,  8,  5,
/* 2 */  1, 11,kFF,'/','*','-',  9,206,kFF,171,kFF,224,kFF,kFF,kFF,175,
/* 3 */184,210,211,kFF,127,  4, 12,'7','8','9','+',kFF,129,kFF,kFF,kFF,
/* 4 */kFF,kFF,kFF,kFF,174,172,247, 10,'4','5','6',kFF,kFF,kFF,130,kFF,
/* 5 */kFF,132,kFF,178,179,192,kFF, 30,'1','2','3', 10,kFF,kFF,202,kFF,
/* 6 */kFF, 28, 31, 29,'0','.',kFF,kFF,kFF,'|',kFF,kFF,kFF,kFF,kFF,kFF,
/* 7 */kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,200,202,

// Option table...
/*      -0-kFF- -2- -3- -4- -5- -6- -7- -8- -9- -a- -b- -c- -d- -e- -f-   */

/* 0 */kFF, 27, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
/* 1 */ 16,'`',193,170,163,162,176,246,166,165,199,200,208,173,  8,  5,
/* 2 */  1, 11,kFF,'/','*','-',  9,207,183,171,168,160,180,kFF,222,191,
/* 3 */185,212,213,194,127,  4, 12,  1, 30, 11,'+',kFF,140,186,182,196,
/* 4 */169,kFF,198,215,190,172,247, 10, 28,kFF, 29,kFF,189,197,141,195,
/* 5 */167,150,181,kFF,201,214,kFF, 30,  4, 31, 12, 10,kFF,kFF,202,kFF,
/* 6 */kFF, 28, 31, 29,  5,127,kFF,kFF,kFF, 92,kFF,kFF,kFF,kFF,kFF,kFF,
/* 7 */kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,200,202,

// Caps-Shift table...
/*      -0-kFF- -2- -3- -4- -5- -6- -7- -8- -9- -a- -b- -c- -d- -e- -f-   */

/* 0 */kFF, 27, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
/* 1 */ 16,'~','!','@','#','$','%','^','&','*','(',')','_','+',  8,  5,
/* 2 */  1, 11,kFF,'/','*','-',  9,'q','w','e','r','t','y','u','i','o',
/* 3 */'p','{','}','|',127,  4, 12,'7','8','9','+',kFF,'a','s','d','f',
/* 4 */'g','h','j','k','l',':','"', 10,'4','5','6',kFF,'z','x','c','v',
/* 5 */'b','n','m','<','>','?',kFF, 30,'1','2','3', 10,kFF,kFF,' ',kFF,
/* 6 */kFF, 28, 31, 29,'0','.',kFF,kFF,kFF,'|',kFF,kFF,kFF,kFF,kFF,kFF,
/* 7 */kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,200,202,

// Caps table...
/*      -0-kFF- -2- -3- -4- -5- -6- -7- -8- -9- -a- -b- -c- -d- -e- -f-   */

/* 0 */kFF, 27, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
/* 1 */ 16,'`','1','2','3','4','5','6','7','8','9','0','-','=',  8,  5,
/* 2 */  1, 11,kFF,'/','*','-',  9,'Q','W','E','R','T','Y','U','I','O',
/* 3 */'P','[',']', 92,127,  4, 12,  1, 30, 11,'+',kFF,'A','S','D','F',
/* 4 */'G','H','J','K','L',';', 39, 10, 28,kFF, 29,kFF,'Z','X','C','V',
/* 5 */'B','N','M',',','.','/',kFF, 30,  4, 31, 12, 10,kFF,kFF,' ',kFF,
/* 6 */kFF, 28, 31, 29,  5,127,kFF,kFF,kFF, 92,kFF,kFF,kFF,kFF,kFF,kFF,
/* 7 */kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,200,202,

// Shift table...
/*      -0-kFF- -2- -3- -4- -5- -6- -7- -8- -9- -a- -b- -c- -d- -e- -f-   */

/* 0 */kFF, 27, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
/* 1 */ 16,'~','!','@','#','$','%','^','&','*','(',')','_','+',  8,  5,
/* 2 */  1, 11,kFF,'/','*','-',  9,'Q','W','E','R','T','Y','U','I','O',
/* 3 */'P','{','}','|',127,  4, 12,'7','8','9','+',kFF,'A','S','D','F',
/* 4 */'G','H','J','K','L',':','"', 10,'4','5','6',kFF,'Z','X','C','V',
/* 5 */'B','N','M','<','>','?',kFF, 30,'1','2','3', 10,kFF,kFF,' ',kFF,
/* 6 */kFF, 28, 31, 29,'0','.',kFF,kFF,kFF,'|',kFF,kFF,kFF,kFF,kFF,kFF,
/* 7 */kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,200,202,

// No mods table...
/*      -0-kFF- -2- -3- -4- -5- -6- -7- -8- -9- -a- -b- -c- -d- -e- -f-   */

/* 0 */kFF, 27, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
/* 1 */ 16,'`','1','2','3','4','5','6','7','8','9','0','-','=',  8,  5,
/* 2 */  1, 11,kFF,'/','*','-',  9,'q','w','e','r','t','y','u','i','o',
/* 3 */'p','[',']', 92,127,  4, 12,  1, 30, 11,'+',kFF,'a','s','d','f',
/* 4 */'g','h','j','k','l',';', 39, 10, 28,kFF, 29,kFF,'z','x','c','v',
/* 5 */'b','n','m',',','.','/',kFF, 30,  4, 31, 12, 10,kFF,kFF,' ',kFF,
/* 6 */kFF, 28, 31, 29,  5,127,kFF,kFF,kFF, 92,kFF,kFF,kFF,kFF,kFF,kFF,
/* 7 */kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,kFF,200,202,

// Acute dead key table...
' ', 171,
'A', 231,
'E', 131,
'I', 234,
'O', 238,
'U', 242,
'a', 135,
'e', 142,
'i', 146,
'o', 151,
'u', 156,
  0,   0,
  0,   0,
  0,   0,
  0,   0,
  0,   0,

// Grave dead key table...
' ', '`',
'A', 203,
'E', 233,
'I', 237,
'O', 241,
'U', 244,
'a', 136,
'e', 143,
'i', 147,
'o', 152,
'u', 157,
  0,   0,
  0,   0,
  0,   0,
  0,   0,
  0,   0,

// Circumflex dead key table...
' ', 246,
'A', 229,
'E', 230,
'I', 235,
'O', 239,
'U', 243,
'a', 137,
'e', 144,
'i', 148,
'o', 153,
'u', 158,
  0,   0,
  0,   0,
  0,   0,
  0,   0,
  0,   0,

// Diaeresis dead key table...
' ', 172,
'A', 128,
'E', 232,
'I', 236,
'O', 133,
'U', 134,
'Y', 217,
'a', 138,
'e', 145,
'i', 149,
'o', 154,
'u', 159,
'y', 216,
  0,   0,
  0,   0,
  0,   0,

// Tilde dead key table...
' ', 247,
'A', 204,
'N', 132,
'O', 205,
'a', 139,
'n', 150,
'o', 155,
  0,   0,
  0,   0,
  0,   0,
  0,   0,
  0,   0,
  0,   0,
  0,   0,
  0,   0,
  0,   0,

// Acute table mask...
B_OPTION_CAPS_SHIFT_TABLE | B_OPTION_CAPS_TABLE |
B_OPTION_SHIFT_TABLE | B_OPTION_TABLE,

// Grave table mask...
B_OPTION_CAPS_SHIFT_TABLE | B_OPTION_CAPS_TABLE |
B_OPTION_SHIFT_TABLE | B_OPTION_TABLE,

// Circumflex table mask...
B_OPTION_CAPS_SHIFT_TABLE | B_OPTION_CAPS_TABLE |
B_OPTION_SHIFT_TABLE | B_OPTION_TABLE,

// Diaeresis table mask...
B_OPTION_CAPS_SHIFT_TABLE | B_OPTION_CAPS_TABLE |
B_OPTION_SHIFT_TABLE | B_OPTION_TABLE,

// Tilde dead key table...
B_OPTION_CAPS_SHIFT_TABLE | B_OPTION_CAPS_TABLE |
B_OPTION_SHIFT_TABLE | B_OPTION_TABLE
};


static void
check_endianess_4(
	uint32	*buf,
	int32	count) 
{	
	union endi_test {
		uint8	bytes[2];
		uint16	word;
	};

	endi_test test;
	test.bytes[0] = 0;
	test.bytes[1] = 1;
	if (test.word == 0x100) {
		for (int32 i = 0; i < count; i++) {
			uint32 val = buf[i]; 
			buf[i] = (val >> 24) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000) | (val << 24);
		}
	}
}


static bool
convert_key_table(
	uint32	*t,
	int32	count) 
{
	for (int32 i = 0; i < count; i++) {
		if (t[i] > 0xfffffff0)
			t[i] = 1;
		else if (t[i] < 128)
			t[i] *= 2;
		else if (t[i] < 256)
			t[i] = 5 * t[i] - 3 * 128;
		else
			return (false);
	}

	return (true);
}


void
init_keymap(
	key_map	*keyMap, 
	char	**keyMapChars, 
	int32	*keyMapCharsSize)
{
	int32 size;

	memset(keyMap, 0, sizeof(keyMap));
	*keyMapChars = NULL;
	*keyMapCharsSize = 0;

	// try to read the current key_map file (the user one)
	BPath keyMapPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &keyMapPath) != B_NO_ERROR)
		return;

	keyMapPath.Append("Key_map");
	FILE *fp = fopen(keyMapPath.Path(), "rb");
	if (fp == NULL)
		// if no file, use the default key_map
		goto bad_file;

	// try to read the key_map struct
	size = fread(keyMap, 1, sizeof(key_map), fp);
	if (size != sizeof(key_map)) {
		fclose(fp);
		goto bad_file;
	}

	check_endianess_4((uint32 *)keyMap, sizeof(key_map) / 4);

	if (keyMap->version == 3) {
		// for version 3, try to read the string buffer 
		size = fread(keyMapCharsSize, 1, sizeof(int32), fp);
		check_endianess_4((uint32 *)keyMapCharsSize, 1);
		if ((size != sizeof(int32)) || (*keyMapCharsSize > 1048576)) {
			fclose(fp);
			goto bad_file;
		}

		*keyMapChars = (char *)malloc(*keyMapCharsSize);
		size = fread(*keyMapChars, 1, *keyMapCharsSize, fp);
		if (size != *keyMapCharsSize) {
			free(*keyMapChars);
			*keyMapChars = NULL;
			*keyMapCharsSize = 0;
			fclose(fp);
			goto bad_file;
		}
	}

	fclose(fp);
	goto good_file;

bad_file:
	_sPrintf("input_server: bad key map file!\n");
	memcpy(keyMap, defaultTables, sizeof(key_map));

good_file:
		// for version 1 or 2, do the conversion to version 3 format
	if (keyMap->version != 3) {
		*keyMapCharsSize = 256 + 5 * 128;
		*keyMapChars = (char *)malloc(*keyMapCharsSize);
		uint16 *macTable = b_symbol_set_table[11] + 128;
		for (int32 i = 0; i < 128; i++) {
			(*keyMapChars)[2 * i + 0] = 1;
			(*keyMapChars)[2 * i + 1] = i;
			uchar	*str = (uchar *)(*keyMapChars + 256 + 5 * i + 1);
			uchar	*str0 = str;
			uint16	*uniStr = macTable + i;
			UNICODE_2_UTF8(str, uniStr);
			str0[-1] = (str - str0);
		}

		if (!convert_key_table((uint32 *)keyMap->control_map, 128))
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->option_caps_shift_map, 128)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->option_caps_map, 128)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->option_shift_map, 128)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->option_map, 128)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->caps_shift_map, 128)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->caps_map, 128)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->shift_map, 128)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->normal_map, 128)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->acute_dead_key, 32)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->grave_dead_key, 32)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->circumflex_dead_key, 32)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->dieresis_dead_key, 32)) 
			goto bad_file;
		if (!convert_key_table((uint32 *)keyMap->tilde_dead_key, 32)) 
			goto bad_file;

		keyMap->version = 3;
		
		// save the current key_map as the new user key_map
		int32 theKeyMapCharsSize = *keyMapCharsSize;
		check_endianess_4((uint32 *)keyMapCharsSize, 1);
		check_endianess_4((uint32 *)keyMap, sizeof(key_map) / 4);
		if ((fp = fopen(keyMapPath.Path(), "w")) != NULL) {
			fwrite(keyMap, 1, sizeof(key_map), fp);
			fwrite(keyMapCharsSize, 1, sizeof(int32), fp);
			fwrite(*keyMapChars, 1, theKeyMapCharsSize, fp);
			fclose(fp);
		}
		check_endianess_4((uint32 *)keyMapCharsSize, 1);
		check_endianess_4((uint32 *)keyMap, sizeof(key_map) / 4);
	}
}

