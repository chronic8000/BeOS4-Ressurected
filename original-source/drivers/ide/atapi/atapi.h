#ifndef _ATAPI_H
#define _ATAPI_H

#include <lendian_bitfield.h>

/* CD-ROM Capabilities & Mechanical Status Page */

typedef struct {
/* header */
	uint16	mode_data_length;	/* big endian */
	uint8	medium_type;
	uint8	reserved_3to7[5];
/* CD-ROM Capabilities & Mechanical Status Page */
	LBITFIELD8_3(
		Page_Code						: 6,
		reserved_0_6					: 1,
		PS								: 1
	);
	uint8	Page_Length;
	LBITFIELD8_4(
		CD_R_Rd							: 1,
		CD_E_Rd							: 1,
		Method2							: 1,
		reserved_2_3to7					: 5
	);
	LBITFIELD8_3(
		CD_R_Wr							: 1,
		CD_E_Wr							: 1,
		reserved_3_2to7					: 6
	);
	LBITFIELD8_8(
		AudioPlay						: 1,
		Composite						: 1,
		DigitalPort1					: 1,
		DigitalPort2					: 1,
		Mode2Form1						: 1,
		Mode2Form2						: 1,
		MultiSession					: 1,
		reserved_4_7					: 1
	);
	LBITFIELD8_8(
		CD_DA							: 1,
		DAAccu							: 1,
		RW_Supported					: 1,
		RW_Deinterleaved_and_Corrected	: 1,
		C2_Pointers						: 1,
		ISRC							: 1,
		UPC								: 1,
		reserved_5_7					: 1
	);
	LBITFIELD8_6(
		Lock							: 1,
		Lock_State						: 1,
		Prevent_Jumper					: 1,
		Eject							: 1,
		reserved_6_4					: 1,
		Loading_Mechanism_Type			: 3
	);
	LBITFIELD8_5(
		Separate_Volume_Levels			: 1,
		Separate_Channel_Mute			: 1,
		Support_Disc_Present			: 1,
		Software_Slot_Selection			: 1,
		reserved_7_4to7					: 4
	);
	uint16	Maximum_Speed_Supported;			// KBps big-endian
	uint16	Number_of_Volume_Levels_Supported;	// big-endian
	uint16	Buffer_Size_supported_by_Drive;		// KBytes big-endian
	uint16	Current_Speed_Selected;				// KBps big-endian
	uint8	reserved_16;
	LBITFIELD8_6(
		reserved_17_0					: 1,
		BCK								: 1,
		RCK								: 1,
		LSBF							: 1,
		Length							: 1,
		reserved_17_6to7				: 2
	);
	uint8	reserved_18;
	uint8	reserved_19;
} cdrom_capabilities;

#endif
