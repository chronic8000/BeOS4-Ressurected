//////////////////////////////////////////////////////////////////////////////
// Vertical Blanking Interrupt Functions
//
//    This file contains hardware-specific functions called by the driver
// during a vertical blanking interrupt.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <common_includes.h>
#include <driver_includes.h>

#include <definesR128.h>
#include <registersR128.h>


void Stubb_Palette ( SHARED_INFO *si );


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#define PALETTE_DATA_R 2
#define PALETTE_DATA_G 1
#define PALETTE_DATA_B 0
#define PALETTE_W_INDEX PALETTE_INDEX 

//////////////////////////////////////////////////////////////////////////////
// Set the Palette
//    This function sets the colour lookup table used for palette generation
// at 8 bpp. 
// "first" indicates the index of the first colour to be written,
// "count" specifies the number of palette entries to set, and 
// "data"  points to an array of bytes defining the red, green, and blue values 
//         of the palette entries that are to be written (in that order). 
// "si" points to the SHARED_INFO structure associated with the card to be 
//         programmed.
void set_palette(int first, int count, uint8 *data, SHARED_INFO *si)
{
    vuint8 *regs = si->card.regs;	// 8bit Register aperture pointer.
    uint32 Ltmp;					// temp value for palette data
	
    
  // Tattletale.
  ddprintf(("[R128 GFX]  set_palette() called.\n"));

	// HACK - from Trey/Todd make this do nothing in >8bpp, 
	// as the palette table doubles as a gamma table and 
	// so this will mess it up. If Be requires that the palette
	// be preserved through mode switches, make our own array
	// to hold the palette and copy it over at mode switch time.	
	if (si->display.dm.space != B_CMAP8) 
	{
		// gamma code update
		
	}
//	else
	{
		READ_REG(DAC_CNTL, Ltmp);
		Ltmp &= ~0x8000ul; 		            // Make sure DAC is powered up (Power Management)
		Ltmp |= (unsigned long)DAC_8BIT_EN ;//0x0100ul Set DAC to 8-bit, not 6 bit vga mode				
		WRITE_REG(DAC_CNTL,Ltmp);	
						
		regs[PALETTE_W_INDEX] = (vuint8)first; 	// set write index to first

		READ_REG(DAC_CNTL, Ltmp);	
		Ltmp |= (0xff << 24);
		WRITE_REG(DAC_CNTL, Ltmp); 			// Dac usage mask (default)
		
	// write 32 bits (dword) sequentially (auto indexing) 
	// for each palette data value from our stored array of palette
	// byte data. ( data bytes are stored in our array as R[0], G[1], B[2]
	// and are used by ATI as B[0], G[1], R[2] )
		while (count--) {
			Ltmp = (*data << 16 ) | ( *(data+PALETTE_DATA_G) << 8 ) | *(data+ PALETTE_DATA_R) ;

			WRITE_REG(PALETTE_DATA,Ltmp);   // write one 32 bit value for each index
			data += 3;
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// Hardware Specific VBI Work
//    This function performs hardware-specific activities that must be
// attended to to service the vertical blanking interrupt.
int32 int_count =0;
void hardware_specific_vblank_work(SHARED_INFO *si)
{
	if (int_count++ > 100 )
	{	int_count = 0;
		//ddprintf(("[R128] VB Int!\n"));
	}

  // Tattletale.
//  ddprintf(("[Dummy GFX]  hardware_specific_vblank_work() called.\n"));

  // Do nothing.
}

//////////////////////////////////////////////////////////////////////////////
// Hardware Specific VBI Work
//    This function fills the VGA palette with dummy values, in order
// set the palette to a known value on mode set.
void Stubb_Palette ( SHARED_INFO *si )
{
	static int was_set = 0;		// only do this the first time
	uint8 palette_table [768];
	int x;
	
	if( !was_set)
	{
		for	(x = 0; x < 256; x ++)
		{
			palette_table [x*3 ] = x;
			palette_table [x*3 + 1] = x;
			palette_table [x*3 + 2] = x;			
		}
		
		set_palette ( 0 , 255, palette_table, si	);
		
		was_set = TRUE;
	}


}
//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
