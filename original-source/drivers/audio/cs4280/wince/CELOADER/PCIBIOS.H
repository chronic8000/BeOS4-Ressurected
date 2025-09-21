//*******************************************************************
//
//    Description: Header file for Pci bios functions. 
//
//    File: pcibios.h 
//
//    Copyright(c) 1996 Crystal Semiconductor Corp. All Rights Reserved.
//
//    Date:  02/06/97
//*******************************************************************
// Definitions 
//#ifndef PCIBIOS_H
//#define PCIBIOS_H

#define INTERRUPT_PCIBIOS       0x1a

#define PCI_FUNCTION_ID         0xb1

#define PCI_BIOS_PRESENT        0x1
#define PCI_FIND_PCI_DEVICE     0x2
#define PCI_READ_CONFIG_BYTE    0x8
#define PCI_READ_CONFIG_WORD    0x9
#define PCI_READ_CONFIG_DWORD   0xa
#define PCI_WRITE_CONFIG_BYTE   0xb
#define PCI_WRITE_CONFIG_WORD   0xc
#define PCI_WRITE_CONFIG_DWORD  0xd
#define GET_IRQ_ROUTING_OPTIONS 0xe
#define PCI_SET_INTERRUPT       0xf

#define PCI_SUCCESSFUL          0
#define PCI_SET_FAILED          0x88
#define PCI_BUFFER_TOO_SMALL    0x89
// Structure prototypes

#pragma pack(1)
// Used in functions 
typedef struct
{
    unsigned char
        bBusNum;
    unsigned char
        bDevFuncNum;
}  PCI_BUS_DEVICE_FUNCTION;

typedef struct
{
    BYTE
        bPciBusNum;
    BYTE
        bPCIDevNum;
    BYTE
        bLinkInt_A;
    unsigned short
        wIrqBitmapInt_A;
    BYTE
        bLinkInt_B;
    unsigned short
        wIrqBitmapInt_B;
    BYTE
        bLinkInt_C;
    unsigned short
        wIrqBitmapInt_C;
    BYTE
        bLinkInt_D;
    unsigned short
        wIrqBitmapInt_D;
    BYTE
        bSlotNumber;
    BYTE
        bReserved;
} IRQRoutingTableEntry;

typedef struct
{
    unsigned short
        wBufferSize;
    IRQRoutingTableEntry _far
        *pData;
} IRQRoutingOptionsBuffer;


#pragma pack()


// Function Prototypes
extern "C" short has_pci_bios(void);

extern "C" short find_pci_device(PCI_BUS_DEVICE_FUNCTION *ri,
                unsigned short device_id, unsigned short vendor_id);
                     
extern "C" short ReadConfigBYTE(BYTE bBusNum, BYTE bDevFunc, 
                unsigned short register_number, BYTE *value_read);
                     
extern "C" short ReadConfigWORD(BYTE bBusNum, BYTE bDevFunc,  
                unsigned short register_number, unsigned short *value_read);
                     
extern "C" short ReadConfigDWORD(BYTE bBusNum, BYTE bDevFunc,  
                unsigned short register_number, DWORD *value_read);
                     
extern "C" short WriteConfigBYTE(BYTE bBusNum, BYTE bDevFuncNum,  
                unsigned short register_number, BYTE bValue);

extern "C" short WriteConfigWORD(BYTE bBusNum, BYTE bDevFuncNum,  
                unsigned short register_number, unsigned short wValue);
                      
extern "C" short WriteConfigDWORD(BYTE bBusNum, BYTE bDevFuncNum,  
                unsigned short register_number, DWORD dwValue);
                      
extern "C" BYTE GetIrqRoutingOptions(IRQRoutingOptionsBuffer far *pRouteBuffer);

extern "C" BYTE GetHardwareInt(BYTE bBusNum, BYTE bDevFuncNum);

extern "C" short SetHardwareInterrupt(BYTE bBusNum, BYTE bDevFuncNum, BYTE bInt);


#define PROT32MODE		1
/*
 * Bit Definitions For Register Consistency Checking
 *
 * Note that certain registers imply that other registers are consistent.  For
 * example, consistency of ax implies consistency of ah and al.  These
 * definitions reflect this.
 */
#define AH_M	0x80000000
#define AL_M	0x40000000
#define BH_M	0x20000000
#define BL_M	0x10000000
#define CH_M	0x08000000
#define CL_M	0x04000000
#define DH_M	0x02000000
#define DL_M	0x01000000

#define AX_M	( AH_M | AL_M )
#define BX_M	( BH_M | BL_M )
#define CX_M	( CH_M | CL_M )
#define DX_M	( DH_M | DL_M )
#define SI_M	0x00800000
#define DI_M	0x00400000

#define EAX_M	( 0x00200000 | AX_M )
#define EBX_M	( 0x00100000 | BX_M )
#define ECX_M	( 0x00080000 | CX_M )
#define EDX_M	( 0x00040000 | DX_M )
#define ESI_M	( 0x00020000 | SI_M )
#define EDI_M	( 0x00010000 | DI_M )

/*
 * Isolation Masks For Register Consistency Checking
 */
#define HBYTE_M	0xffff00ff
#define LBYTE_M 0xffffff00
#define LWORD_M	0xffff0000
#define HWORD_M 0x0000ffff

/*
 * Description Of Which Registers Can Be Changed For Each Supported Function
 */
#define PCI_BIOS_PRESENT_M		AX_M | BX_M | CL_M | EDX_M
#define FIND_PCI_DEVICE_M		AH_M | BX_M
#define FIND_PCI_CLASS_CODE_M		AH_M | BX_M
#define GENERATE_SPECIAL_CYC_M		AH_M
#define READ_CONFIG_BYTE_M		AH_M | CL_M
#define READ_CONFIG_WORD_M		AH_M | CX_M
#define READ_CONFIG_DWORD_M		AH_M | ECX_M
#define WRITE_CONFIG_BYTE_M		AH_M
#define WRITE_CONFIG_WORD_M		AH_M 
#define WRITE_CONFIG_DWORD_M		AH_M
#define	GET_IRQ_ROUTING_OPTIONS_M	AH_M | BX_M
#define	SET_PCI_HW_INT_M		AH_M


//extern	struct PCIBios *FindPCIBios(void );
extern	int PCIBios32BitSupport(void );
extern	unsigned long FindPCIBios32Entry(unsigned long *EntryPoint);

//extern	int PCIBiosPresent(union x86Regs *Regs);
extern  int FindPCIDevice(PCI_BUS_DEVICE_FUNCTION *ri, unsigned short DevId, unsigned short VenId);

//extern	int FindPCIClassCode(union x86Regs *Regs,unsigned long ClassCode,unsigned short Index);
//extern	int GenSpecialCycle(union x86Regs *Regs,unsigned char BusNum,unsigned long Data);
//extern	int ReadConfigByte(union x86Regs *Regs,unsigned char BusNum,unsigned char DevNum,unsigned char FuncNum,unsigned short Offset);
//extern	int ReadConfigWord(union x86Regs *Regs,unsigned char BusNum,unsigned char DevNum,unsigned char FuncNum,unsigned short Offset);
//extern	int ReadConfigDWord(union x86Regs *Regs,unsigned char BusNum,unsigned char DevNum,unsigned char FuncNum,unsigned short Offset);
//extern	int WriteConfigByte(union x86Regs *Regs,unsigned char BusNum,unsigned char DevNum,unsigned char FuncNum,unsigned short Offset,unsigned char Value);
//extern	int WriteConfigWord(union x86Regs *Regs,unsigned char BusNum,unsigned char DevNum,unsigned char FuncNum,unsigned short Offset,unsigned short Value);
//extern	int WriteConfigDWord(union x86Regs *Regs,unsigned char BusNum,unsigned char DevNum,unsigned char FuncNum,unsigned short Offset,unsigned long Value);
//extern	int GetIRQRoutingOptions(union x86Regs *Regs,unsigned short BufferSize);
//extern	int SetPCIHardwareInt(union x86Regs *Regs,unsigned char BusNum,unsigned char DevNum,unsigned char FuncNum,unsigned char IRQNum,unsigned char IntPinNum);
//extern	int BIOSInt(X86REG *,X86REG *,DWORD, int) ;
//extern	void ValidateReg(union x86Regs *InRegs,union x86Regs *OutRegs,unsigned char Function,unsigned long CompareMask,unsigned char Mode);

//#endif
