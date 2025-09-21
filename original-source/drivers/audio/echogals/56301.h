// 56301.h - header file describing the hardware

#define MOTOROLA_VENDOR_ID					0x1057
#define MOTOROLA_56301_ID					0x1801
#define ECHO_VENDOR_ID						0xecc0

#define	CHI32_CONTROL_REG					4
#define	CHI32_STATUS_REG					5
#define	CHI32_VECTOR_REG					6
#define	CHI32_DATA_REG						7

#define	VECTOR_READY						0x00000001
#define	CHI32_STATUS_REG_HF3				0x00000008
#define	CHI32_STATUS_REG_HF4				0x00000010
#define	CHI32_STATUS_REG_HF5				0x00000020
#define	CHI32_STATUS_WAVE_ACK			CHI32_STATUS_REG_HF4
#define	HOST_READ_FULL						0x00000004
#define	HOST_WRITE_EMPTY					0x00000002
#define 	CHI32_HINT			      		0x00000040





