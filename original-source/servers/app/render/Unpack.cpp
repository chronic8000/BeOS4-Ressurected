//--------------------------------------------------------------------
//	
//	LBX_Unpack.cpp
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

//#include "BitPack.h"
#include "Unpack.h"

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Unpack::LBX_Unpack(uint16 *color_table) {
	table_rgb = color_table;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Unpack::~LBX_Unpack() {
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::GetNextSource() {
	uint8		val;
	uint32		offset;
	
	val = *next_src++;
	if (val < 0x60) {
		src = next_src;
		src_end = src+(val+1);
		next_src = src_end;
	}
	else if (val >= 0xc0) {
		src = similar_src2 + *next_src++;
		src_end = src+(val-0xbd);
	}
	else if (val >= 0x80) {
		src = similar_src1 + *next_src++;
		src_end = src+(val-0x7d);
	}
	else {
		offset = *next_src++;
		offset += ((uint32)(*next_src++))<<8;
		src = (next_src-3) - (offset + (val-0x5c));
		src_end = src + (val-0x5c);
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::ExtractTo32(uint8 *src_input, uint32 *dst, uint32 dst_left, uint32 dst_right) {
	src = src_input;
	src_end = src+1000000;
	dst_offset = dst_left;
	dst32 = dst;
	ExtractTo32(dst_right);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::ExtractTo32(uint8 *src_input, uint8 *src1, uint8 *src2,
							  uint32 *dst, uint32 dst_left, uint32 dst_right) {
	similar_src1 = src1;
	similar_src2 = src2;
	next_src = src_input;
	src = src_end = NULL;
	dst_offset = dst_left;
	dst32 = dst;
	ExtractTo32(dst_right);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::RGB16To32(int32 count) {
	uint8		val;
	int32		len;
	uint16		val16 = 0;
	uint32		val32;
	
	while (count > 0) {
		val = ReadByte();
		if (val < 0x80) {
		//	00-7f: 1 pixel, relative offset in the ring buffer
			val16 = table_rgb[val];		
			Write32(((uint32)(val16 & RED_MASK)<<8) | ((uint32)(val16 & GREEN_MASK)<<5) |
					((uint32)(val16 & BLUE_MASK)<<3) | 0xff000000);
			count--;
		}
		else if (val < 0xe0) {
		//  80-df: pixel repetition, relative to last ring buffer entry
			val16 = table_rgb[val-0x80];
			len = 2;
			goto fill_len_without_alpha;
		}
		else if (val < 0xf0) {
			if (val >= 0xec)
		//	ec-ef: [+ val2]	(val*256+val2+(14-236*256)) repetitions of the last ring buffer entry
				len = ((val-236)<<8) + (int32)ReadByte() + 14;
			else
		//	e0-eb: (val-222) repetitions of the last ring buffer entry
				len = val-222;
			val16 = last_color;
fill_len_without_alpha:
			count -= len;
			len += count & (count>>31);
			val32 = ((uint32)(val16 & RED_MASK)<<8) | ((uint32)(val16 & GREEN_MASK)<<5) |
					((uint32)(val16 & BLUE_MASK)<<3) | 0xff000000;
			for (;len>0; len--) {
				Write32(val32);
			}
		}
		else {
		//	f0-ff: [+ RGB]	(val-239) uncompressed RGB16 values
			len = val-239;
			count -= len;
			len -= (count & (count>>31));
			for (; len>0; len--) {
				val16 = (uint16)ReadByte() | ((uint16)ReadByte()<<8);
				Write32(((uint32)(val16 & RED_MASK)<<8) | ((uint32)(val16 & GREEN_MASK)<<5) |
						((uint32)(val16 & BLUE_MASK)<<3) | 0xff000000);
			}
		}
		last_color = val16;
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::RGBA16To32(int32 count) {
	uint8		val;
	int32		len;
	uint32		alpha;
	uint16		val16 = 0;
	uint32		val32;

	SyncAlpha();
	while (count > 0) {
		val = ReadByte();
		if (val < 0x80) {
		//	00-7f: 1 pixel, relative offset in the ring buffer
			val16 = table_rgb[val];		
			alpha = ReadAlpha();
			Write32(((uint32)(val16 & RED_MASK)<<8) | ((uint32)(val16 & GREEN_MASK)<<5) |
					((uint32)(val16 & BLUE_MASK)<<3) | (alpha << 28) | (alpha << 24));
			count--;
		}
		else if (val < 0xe0) {
		//  80-df: pixel repetition, relative to last ring buffer entry
			val16 = table_rgb[val-0x80];
			len = 2;
			goto fill_len_with_alpha;
		}
		else if (val < 0xf0) {
			if (val >= 0xec)
		//	ec-ef: [+ val2]	(val*256+val2+(14-236*256)) repetitions of the last ring buffer entry
				len = ((val-236)<<8) + (int32)ReadByte() + 14;
			else
		//	e0-eb: (val-222) repetitions of the last ring buffer entry
				len = val-222;
			val16 = last_color;
fill_len_with_alpha:
			count -= len;
			len -= (count & (count>>31));
			val32 = ((uint32)(val16 & RED_MASK)<<8) | ((uint32)(val16 & GREEN_MASK)<<5) |
					((uint32)(val16 & BLUE_MASK)<<3);
			for (;len>0; len--) {
				alpha = ReadAlpha();
				Write32(val32 | (alpha << 28) | (alpha << 24));
			}
		}
		else {
		//	f0-ff: [+ RGB]	(val-239) uncompressed RGB16 values
			len = val-239;
			count -= len;
			len -= (count & (count>>31));
			for (; len>0; len--) {
				alpha = ReadAlpha();
				val16 = (uint16)ReadByte() | ((uint16)ReadByte()<<8);
				Write32(((uint32)(val16 & RED_MASK)<<8) | ((uint32)(val16 & GREEN_MASK)<<5) |
						((uint32)(val16 & BLUE_MASK)<<3) | (alpha << 28) | (alpha << 24));
			}
		}
		last_color = val16;
	}
}

//	a0:[+ val2] 3 bits binary pattern, key (val2&15), 3 bits
//	a1:[+ val2] 4 bits binary pattern, key (val2&15), 4 bits
//	a2-a3:[+ val2] 5 bits binary pattern, key (val2&15), 5 bits
//	a4-a7:[+ val2] 6 bits binary pattern, key (val2&15), 6 bits
//	a8-af:[+ val2] 7 bits binary pattern, key (val2&15), 7 bits
//	b0-bf:[+ val2] 8 bits binary pattern, key (val2&15), 8 bits
static uint8 binary_code_to_length[32] = {
	3, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

static uint8 alpha_coeff1[16] = { 0, 2, 4, 6, 8, 11, 13, 15, 17, 19, 21, 24, 26, 28, 30, 32};
static uint8 alpha_coeff2[16] = { 32, 30, 28, 26, 24, 21, 19, 17, 15, 13, 11, 8, 6, 4, 2, 0};

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::ExtractTo32(int32 total_count) {
	uint8		val;
	int32		i, count, length, bit_length;
	uint16		val16;
	uint32		val32, alpha, bit_field;
	uint32		v32[2];
	
// decompress in destination buffer
	while (total_count > 0) {
		last_color = table_rgb[0];
		val = ReadByte();
		if (val < 0xf0) {
			if (val < 0x70) {
		//  00-6f: (val+1) fully transparent pixels (no data)
				count = val+1;
	process_transparent:
				total_count -= count;
				count += total_count & (total_count>>31);
				for (i=0; i<count; i++)
					Write32(0);
			}
			else if (val < 0xd7) {
		//  70-df: (val-111) fully opaque pixels (rgb stream)
				count = val-111;
	process_opaque_stream:
				total_count -= count;
				count += total_count & (total_count>>31);
				RGB16To32(count);
			}
			else if (val >= 0xe0) {
		//  e0-ef: (val-223) dropped shadow (uncompressed alpha stream)
				count = val-223;
	process_shadow_stream:
				total_count -= count;
				count += total_count & (total_count>>31);
				for (i=count; i>1; i-=2) {
					val = ReadByte();
					Write32(((uint32)val<<28) | ((uint32)(val&15)<<24));
					val >>= 4;
					Write32(((uint32)val<<28) | ((uint32)(val&15)<<24));
				}
				if (i == 1) {
					val = ReadByte();
					Write32(((uint32)val<<28) | ((uint32)(val&15)<<24));
				}
			}
		}
		else {
			if (val < 0xf4) {
		//  f0-f3: [+ val2]	(val*256+val2+(113-240*256)) fully transparent pixels (no data)
				count = ((int32)(val-0xf0)<<8) + (int32)ReadByte() + 113;
				goto process_transparent;
			}
			else if (val < 0xf8) {
		//  f4-f7: [+ val2]	(val*256+val2+(104-244*256)) fully opaque pixels (rgb stream)
				count = ((int32)(val-0xf4)<<8) + (int32)ReadByte() + 104;
				goto process_opaque_stream;
			}
			else if (val >= 0xfc) {
		//  fc-ff: [+ val2] (val*256+val2+(5-252*256)) dropped shadow (compressed alpha stream)
				count = (((int32)(val-0xfc)<<8) + (int32)ReadByte() + 5);
				total_count -= count;
				count += total_count & (total_count>>31);
				while (count > 0) {
					val = ReadByte();
					if (val < 0x70) {
				//  00-6f: repeat (val&15) (val>>4)+1 times
						val32 = ((uint32)val<<28) | ((uint32)(val&15)<<24);
						i = (val>>4)+1;
						count -= i;
						i += count & (count>>31);
						for (; i>0; i--)
							Write32(val32);
					}
					else if (val >= 0xa0) {
						if (val < 0xc0) {
					//	a0:[+ val2] 3 bits binary pattern, key (val2&15), 3 bits
					//	a1:[+ val2] 4 bits binary pattern, key (val2&15), 4 bits
					//	a2-a3:[+ val2] 5 bits binary pattern, key (val2&15), 5 bits
					//	a4-a7:[+ val2] 6 bits binary pattern, key (val2&15), 6 bits
					//	a8-af:[+ val2] 7 bits binary pattern, key (val2&15), 7 bits
					//	b0-bf:[+ val2] 8 bits binary pattern, key (val2&15), 8 bits
							val16 = ((uint32)val<<8) + ReadByte();
							length = binary_code_to_length[val-0xa0];
							alpha = val16;
							bit_field = val16>>4;
							bit_length = length;
							goto process_binary_compression;
						}
						else if (val < 0xe0) {
					//	c0-df:[+ val2] (val&1)<<5+(val2>>3)+9 binary stream, key (val>>1)&15, 3 bits.
							val16 = ((uint32)val<<8) + ReadByte();
							length = ((val16>>3) & 63)+9;
							alpha = val16>>9;
							bit_field = val16;
							bit_length = 3;
	process_binary_compression:
							count -= length;
							length += count & (count>>31);
							v32[0] = (alpha<<28) | ((alpha&15)<<24);
							v32[1] = v32[0] + 0x11000000;
							while (length > 0) {
								length--;
								if (bit_length == 0) {
									bit_field = ReadByte();
									bit_length = 8;
								}
								Write32(v32[bit_field & 1]);
								bit_length--;
								bit_field >>= 1;
							}
						}
						else {
					//	e0-ff: binary pair (n, n+1) if 0xe_, (n+1, n) if 0xf_, key (val&15)
							length = 2;
							alpha = val;
							bit_field = (0x0f-val)>>4;
							bit_length = 2;
							goto process_binary_compression;
						}
					}
					else if (val == 0x9f) {
				//	9f: [+ val16] repeat (val16&15) (val16>>4)+8 times
						val16 = (uint16)ReadByte() | ((uint16)ReadByte()<<8);
						val32 = ((uint32)val16<<28) | ((uint32)(val16&15)<<24);
						i=(val16>>4)+8;
						count -= i;
						i += count & (count>>31);
						for (; i>0; i--)
							Write32(val32);
					}
					else {
				//	70-9e: [+ data]	copy (val-110) value from a 4 bits data stream (aligned on a byte)
						i=val-110;
						count -= i;
						i += count & (count>>31);
						for (; i>1; i-=2) {
							val = ReadByte();
							Write32(((uint32)val<<28) | ((uint32)(val&15)<<24));
							val >>= 4;
							Write32(((uint32)val<<28) | ((uint32)(val&15)<<24));
						}
						if (i == 1) {
							val = ReadByte();
							Write32(((uint32)val<<28) | ((uint32)(val&15)<<24));
						}
					}
				}
			}
			else switch (val) {
			case 0xf8:
		//  f8: [+ val2] (val2+16) dropped shadow (uncompressed alpha stream)
				count = (int32)ReadByte() + 16;
				goto process_shadow_stream;
			case 0xf9:
				total_count--;
		//  f9: (1) anti-aliased pixel (1 rgb + 1 alpha)
				val = ReadByte();
				if (val < 0x80) {
					alpha = ReadByte();
			//	00-7f:[+ alpha]	relative offset in the ring buffer, + alpha
					val16 = table_rgb[val];
				}
				else {
			//	80-8f:[+ RGB] (val-128) alpha, + RGB
					alpha = val - 0x80;
					val16 = (uint16)ReadByte() | ((uint16)ReadByte()<<8);
				}
			// Record the value in the ring buffer
				Write32(((uint32)(val16 & RED_MASK)<<8) | ((uint32)(val16 & GREEN_MASK)<<5) |
						((uint32)(val16 & BLUE_MASK)<<3) | (alpha << 28) | (alpha << 24));
				last_color = val16;
				break;
			case 0xfa:
		//  fa: [+ val2] (val2+2) anti-aliased pixel (rgb stream + uncompressed alpha stream)
				count = (int32)ReadByte()+2;
				total_count -= count;
				count += total_count & (total_count>>31);
				RGBA16To32(count);
				break;
			}
		}
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::DrawTo16(uint8 *src_input, uint8 *src1, uint8 *src2,
						   uint16 *dst, uint32 dst_left, uint32 dst_right) {
	similar_src1 = src1;
	similar_src2 = src2;
	next_src = src_input;
	src = src_end = NULL;
	dst_offset = dst_left;
	dst16 = dst;
	DrawTo16(dst_right);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::RGB16To16(int32 count) {
	uint8		val;
	int32		len;
	uint16		val16 = 0;
	
	while (count > 0) {
		val = ReadByte();
		if (val < 0x80) {
		//	00-7f: 1 pixel, relative offset in the ring buffer
			val16 = table_rgb[val];		
			Write16(val16);
			count--;
		}
		else if (val < 0xe0) {
		//  80-df: pixel repetition, relative to last ring buffer entry
			val16 = table_rgb[val-0x80];
			len = 2;
			goto fill_len_without_alpha;
		}
		else if (val < 0xf0) {
			if (val >= 0xec)
		//	ec-ef: [+ val2]	(val*256+val2+(14-236*256)) repetitions of the last ring buffer entry
				len = ((val-236)<<8) + (int32)ReadByte() + 14;
			else
		//	e0-eb: (val-222) repetitions of the last ring buffer entry
				len = val-222;
			val16 = last_color;
fill_len_without_alpha:
			count -= len;
			len += count & (count>>31);
			for (;len>0; len--)
				Write16(val16);
		}
		else {
		//	f0-ff: [+ RGB]	(val-239) uncompressed RGB16 values
			len = val-239;
			count -= len;
			len -= (count & (count>>31));
			for (; len>0; len--) {
				val16 = (uint16)ReadByte() | ((uint16)ReadByte()<<8);
				Write16(val16);
			}
		}
		last_color = val16;
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::RGBA16To16(int32 count) {
	uint8		val;
	int32		len;
	uint32		alpha;
	uint16		val16 = 0;

	SyncAlpha();
	while (count > 0) {
		val = ReadByte();
		if (val < 0x80) {
		//	00-7f: 1 pixel, relative offset in the ring buffer
			val16 = table_rgb[val];		
			alpha = ReadAlpha();
			Draw16(val16, alpha);
			count--;
		}
		else if (val < 0xe0) {
		//  80-df: pixel repetition, relative to last ring buffer entry
			val16 = table_rgb[val-0x80];
			len = 2;
			goto fill_len_with_alpha;
		}
		else if (val < 0xf0) {
			if (val >= 0xec)
		//	ec-ef: [+ val2]	(val*256+val2+(14-236*256)) repetitions of the last ring buffer entry
				len = ((val-236)<<8) + (int32)ReadByte() + 14;
			else
		//	e0-eb: (val-222) repetitions of the last ring buffer entry
				len = val-222;
			val16 = last_color;
fill_len_with_alpha:
			count -= len;
			len += (count & (count>>31));
			for (;len>0; len--) {
				alpha = ReadAlpha();
				Draw16(val16, alpha);
			}
		}
		else  {
		//	f0-ff: [+ RGB]	(val-239) uncompressed RGB16 values
			len = val-239;
			count -= len;
			len -= (count & (count>>31));
			for (; len>0; len--) {
				alpha = ReadAlpha();
				val16 = (uint16)ReadByte() | ((uint16)ReadByte()<<8);
				Draw16(val16, alpha);
			}
		}
		last_color = val16;
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::DrawTo16(int32 total_count) {
	uint8		val;
	int32		i, count, length, bit_length;
	uint16		val16;
	uint32		val32, alpha, bit_field;
	uint32		v32[2];
	
// decompress in destination buffer
	while (total_count > 0) {
		last_color = table_rgb[0];
		val = ReadByte();
		if (val < 0xf0) {
			if (val < 0x70) {
		//  00-6f: (val+1) fully transparent pixels (no data)
				count = val+1;
	process_transparent:
				total_count -= count;
				count += total_count & (total_count>>31);
				if ((int32)dst_offset > 0) {
					dst_offset -= count;
					dst16 -= dst_offset & ((int32)dst_offset>>31);
				}
				else
					dst16 += count;
			}
			else if (val < 0xd7) {
		//  70-df: (val-111) fully opaque pixels (rgb stream)
				count = val-111;
	process_opaque_stream:
				total_count -= count;
				count += total_count & (total_count>>31);
				RGB16To16(count);
			}
			else if (val >= 0xe0) {
		//  e0-ef: (val-223) dropped shadow (uncompressed alpha stream)
				count = val-223;
	process_shadow_stream:
				total_count -= count;
				count += total_count & (total_count>>31);
				for (i=count; i>1; i-=2) {
					val = ReadByte();
					Shade16(val&15);
					Shade16(val>>4);
				}
				if (i == 1) {
					val = ReadByte();
					Shade16(val&15);
				}
			}
		}
		else {
			if (val < 0xf4) {
		//  f0-f3: [+ val2]	(val*256+val2+(113-240*256)) fully transparent pixels (no data)
				count = ((int32)(val-0xf0)<<8) + (int32)ReadByte() + 113;
				goto process_transparent;
			}
			else if (val < 0xf8) {
		//  f4-f7: [+ val2]	(val*256+val2+(104-244*256)) fully opaque pixels (rgb stream)
				count = ((int32)(val-0xf4)<<8) + (int32)ReadByte() + 104;
				goto process_opaque_stream;
			}
			else if (val >= 0xfc) {
		//  fc-ff: [+ val2] (val*256+val2+(5-252*256)) dropped shadow (compressed alpha stream)
				count = (((int32)(val-0xfc)<<8) + (int32)ReadByte() + 5);
				total_count -= count;
				count += total_count & (total_count>>31);
				while (count > 0) {
					val = ReadByte();
					if (val < 0x70) {
				//  00-6f: repeat (val&15) (val>>4)+1 times
						i = (val>>4)+1;
						count -= i;
						i += count & (count>>31);
						val &= 15;
						for (; i>0; i--)
							Shade16(val);
					}
					else if (val >= 0xa0) {
						if (val < 0xc0) {
					//	a0:[+ val2] 3 bits binary pattern, key (val2&15), 3 bits
					//	a1:[+ val2] 4 bits binary pattern, key (val2&15), 4 bits
					//	a2-a3:[+ val2] 5 bits binary pattern, key (val2&15), 5 bits
					//	a4-a7:[+ val2] 6 bits binary pattern, key (val2&15), 6 bits
					//	a8-af:[+ val2] 7 bits binary pattern, key (val2&15), 7 bits
					//	b0-bf:[+ val2] 8 bits binary pattern, key (val2&15), 8 bits
							val16 = ((uint32)val<<8) + ReadByte();
							length = binary_code_to_length[val-0xa0];
							alpha = val16;
							bit_field = val16>>4;
							bit_length = length;
							goto process_binary_compression;
						}
						else if (val < 0xe0) {
					//	c0-df:[+ val2] (val&1)<<5+(val2>>3)+9 binary stream, key (val>>1)&15, 3 bits.
							val16 = ((uint32)val<<8) + ReadByte();
							length = ((val16>>3) & 63)+9;
							alpha = val16>>9;
							bit_field = val16;
							bit_length = 3;
	process_binary_compression:
							count -= length;
							length += count & (count>>31);
							v32[0] = alpha&15;
							v32[1] = v32[0] + 1;
							while (length > 0) {
								length--;
								if (bit_length == 0) {
									bit_field = ReadByte();
									bit_length = 8;
								}
								Shade16(v32[bit_field & 1]);
								bit_length--;
								bit_field >>= 1;
							}
						}
						else {
					//	e0-ff: binary pair (n, n+1) if 0xe_, (n+1, n) if 0xf_, key (val&15)
							length = 2;
							alpha = val;
							bit_field = (0x0f-val)>>4;
							bit_length = 2;
							goto process_binary_compression;
						}
					}
					else if (val == 0x9f) {
				//	9f: [+ val16] repeat (val16&15) (val16>>4)+8 times
						val16 = (uint16)ReadByte() | ((uint16)ReadByte()<<8);
						val32 = val16&15;
						i=(val16>>4)+8;
						count -= i;
						i += count & (count>>31);
						for (; i>0; i--)
							Shade16(val32);
					}
					else {
				//	70-9e: [+ data]	copy (val-110) value from a 4 bits data stream (aligned on a byte)
						i=val-110;
						count -= i;
						i += count & (count>>31);
						for (; i>1; i-=2) {
							val = ReadByte();
							Shade16(val&15);
							Shade16(val>>4);
						}
						if (i == 1) {
							val = ReadByte();
							Shade16(val&15);
						}
					}
				}
			}
			else switch (val) {
			case 0xf8:
		//  f8: [+ val2] (val2+16) dropped shadow (uncompressed alpha stream)
				count = (int32)ReadByte() + 16;
				goto process_shadow_stream;
			case 0xf9:
				total_count--;
		//  f9: (1) anti-aliased pixel (1 rgb + 1 alpha)
				val = ReadByte();
				if (val < 0x80) {
					alpha = ReadByte();
			//	00-7f:[+ alpha]	relative offset in the ring buffer, + alpha
					val16 = table_rgb[val];
				}
				else {
			//	80-8f:[+ RGB] (val-128) alpha, + RGB
					alpha = val - 0x80;
					val16 = (uint16)ReadByte() | ((uint16)ReadByte()<<8);
				}
			// Record the value in the ring buffer
				Draw16(val16, alpha);
				last_color = val16;
				break;
			case 0xfa:
		//  fa: [+ val2] (val2+2) anti-aliased pixel (rgb stream + uncompressed alpha stream)
				count = (int32)ReadByte()+2;
				total_count -= count;
				count += total_count & (total_count>>31);
				RGBA16To16(count);
				break;
			}
		}
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::Shade16(uint32 alpha) {
	uint32		val32;
	
	if ((int32)dst_offset <= 0) {
		val32 = *dst16;
		val32 = (val32 | (val32<<16)) & 0x07e0f81f;
		val32 *= alpha_coeff2[alpha];
		val32 += 0x0100700e;
		val32 &= 0xfc1f03e0;
		val32 |= val32>>16;
		*dst16++ = val32>>5;
	}
	else dst_offset--;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Unpack::Draw16(uint16 val16, uint32 alpha) {
	uint32		val32, val32b;
	
	if ((int32)dst_offset <= 0) {
		val32 = *dst16;
		val32b = val16;
		val32 = (val32 | (val32<<16)) & 0x07e0f81f;
		val32b = (val32b | (val32b<<16)) & 0x07e0f81f;
		val32 *= alpha_coeff2[alpha];
		val32b *= alpha_coeff1[alpha];
		val32 += val32b;
		val32 += 0x0100700e;
		val32 &= 0xfc1f03e0;
		val32 |= val32>>16;
		*dst16++ = val32>>5;
	}
	else dst_offset--;
}
