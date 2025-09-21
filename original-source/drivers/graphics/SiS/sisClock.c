#include <math.h>
#include "sisClock.h"

#include "driver.h"
#include "sis620defs.h"

// Original SiS hardware
void sisReadVCLK() ;
void sisSetupVCLK(uchar numerator, uchar denumerator, uchar divider, uchar postscaler) ;

// SiS630 Hardware
void sis630_Read_ClockRegisters(char which_clock) ;
void sis630_Write_ClockRegisters(char which_clock, uchar numerator, uchar denumerator, uchar divider, uchar postscaler) ;



/*
//////////////////////////////////////////////////////////
// setClock() : Sets the clock to desired freq (in kHz) //
//////////////////////////////////////////////////////////

// For a given frequency, one must find corresponding parameters :
//				numerator, denumerator, divider, postscaler

	fd : desired freq
	fr : reference freq
	fd = fr*(Numerator/Denumerator)*(Divider/PostScaler)

Same constraint for all setups routines :
	Divider = 1 or 2 
	Numerator : 1-128
	Denumerator : 1-32

	*/



uint32 setClock(uint32 clock, uint16 deviceid) {
	uint32 Numerator, Denumerator, Divider, PostScaler;
	uint32 bestNum=0,bestDenum=0,bestDiv=0;
	float clk;
	float bestError=135999999.0;
	float maxClk=135000000.0;
	float refClk=14318180.0;

	vddprintf(("sis: setClock ( %d kHz)\n",(int)clock));
	if ((clock<25000)|(clock>135000)) return(B_ERROR);

	// Maximize accuracy by using postscaler

	clk=(float)clock*1000;
	PostScaler=1;
	if (clk<(maxClk/2)) PostScaler=2;
	if (clk<(maxClk/3)) PostScaler=3;
	if (clk<(maxClk/4)) PostScaler=4;
	if (clk<(maxClk/6)) PostScaler=6;
	if (clk<(maxClk/8)) PostScaler=8;
	clk=clk*PostScaler;
	
	
	// Try each Denumerator
	
	for(Denumerator=2;Denumerator<=32;Denumerator++) {
		float erreur;
		
		// For each Denumerator find the best matching numerator
		
		Numerator=(int)((clk*Denumerator)/refClk);
		if (Numerator>128) {
			Numerator=Numerator>>1;
			Divider=2;
			}
		else Divider=1;
		erreur=abs(clk-(((refClk*Numerator)/Denumerator)*Divider));
		
		if (erreur<bestError) {
			bestNum=Numerator;
			bestDenum=Denumerator;
			bestDiv=Divider;
			bestError=erreur;
			}
		}
	
	Divider=bestDiv;
	Numerator=bestNum;
	Denumerator=bestDenum;
	vddprintf(("sis : Num=%d, Denum=%d, Divider=%d, PostScaler=%d\n",(int)Numerator,(int)Denumerator,(int)Divider,(int)PostScaler));	

	// Best Set of parameter found
	vddprintf(("sis : clock will be programed to %d kHz\n",(int) ((refClk*Divider*Numerator)/(PostScaler*Denumerator)) >>10 ));	

	// Hardware Clock Setup
		
	if (deviceid!=SIS630_DEVICEID) {
		sisSetupVCLK(Numerator,Denumerator,Divider,PostScaler); 
		}
	else {
		isa_outb(MISC_OUT_W,0x0f);	// 0x0f select Internal VCLK registers
	 								// 0x03 : 25 MHz V-Clock
	 								
		isa_outb(SEQ_INDEX, 0x31);
		isa_outb(SEQ_DATA, 0x00);	// 0x00 : Normal DCLK
									// 0x10 : 25 MHz DLCK
									// 0x20 : 28 MHz DCLK

		sis630_Read_ClockRegisters('M') ;
		sis630_Read_ClockRegisters('D') ;
		sis630_Write_ClockRegisters('D',Numerator,Denumerator,Divider,PostScaler) ;
		sis630_Read_ClockRegisters('D') ;
		}
	return(B_OK);
	}

/*	-------- Hardware specific routines --------
	SiS 5598
	SiS 530
	SiS 6326
	SiS 620
 */
 
// -------- SiS 5598, 530, 6326, 620 Read VCLK
 
void sisReadVCLK() {
	uchar numerator, denumerator;
	uchar divider, postscaler;
	uchar t;
	int clock;
	vddprintf(("sis:\n*** Reading Actual Clock config\n"));
	isa_outb(SEQ_INDEX,0x38);
	t=isa_inb(SEQ_DATA);
	t=t&0x03;
	if (t==1) {
		vvddprintf(("sis:25 MHz registers\n"));
		clock=25175;
		}
	else if (t==2) {
		vvddprintf(("sis:28 MHz registers\n"));
		clock=28322;
		}
	else {
		vvddprintf(("sis:Internal VCLK registers\n"));
		clock=14318;
		}
	isa_outb(SEQ_INDEX,0x2a);
	t=isa_inb(SEQ_DATA); /* VCLK reg 0 */
	vvddprintf(("sis:VCLKreg0=0x%02x\n",t));
	divider=1;
	if (t&128) {
		vvddprintf(("sis:divider bit set\n"));
		clock=clock*2;
		divider=2;
		}
	numerator=t&0x7f;
	numerator+=1;
	vvddprintf(("sis:numerator=%d\n",(int)numerator));
	clock=clock*numerator;
	isa_outb(SEQ_INDEX,0x13);
	t=isa_inb(SEQ_DATA);
	vvddprintf(("sis:VCLK reg 2=0x%02x\n",t));
	postscaler=0;
	if (t&64) {
		postscaler+=4;
		}
	isa_outb(SEQ_INDEX,0x2b);
	t=isa_inb(SEQ_DATA);
	vvddprintf(("sis:VCLKreg1=0x%02x\n",t));
	if (t&128) vvddprintf(("sis:Gain for high freq operation\n"));
	else vvddprintf(("sis:Gain for low freq operation\n"));
	t=t&0x7f;
	denumerator=t&0x1f;
	denumerator+=1;
	vvddprintf(("sis:denumerator=%d\n",(int)denumerator));
	if (denumerator>0) clock=clock/denumerator;
	postscaler=(t>>5)|postscaler;
	postscaler+=1;
	vvddprintf(("sis:postscaler=%d\n",(int)postscaler));
	if (postscaler>0) clock=clock/postscaler;
	vddprintf(("sis:clock is read as %d but should be %d kHz\n",clock,(clock*143)/250));
	}

void sisSetupVCLK(uchar numerator, uchar denumerator, uchar divider, uchar postscaler) {
	uchar t;
	vddprintf(("sis: clock settings required: (num,denum,div,postsc) %d %d %d %d\n",numerator,denumerator,divider,postscaler));
#ifdef SIS_VERBOSE
	sisReadVCLK();
#endif

	isa_outb(SEQ_INDEX,0x00); // Synchronous Reset while switching clocks
	isa_outb(SEQ_DATA,0x03);

	// all this is buggy : should decleare to use Internal VCLK,
	// but this doesn't work on... Anyway, VCLK is 14.38 MHz...
	isa_outb(MISC_OUT_W,0x03);	// 0x0f select Internal VCLK registers
	 							// 0x03 : 25 MHz V-Clock
	isa_outb(SEQ_INDEX,0x38);
	t=isa_inb(SEQ_DATA);
	t&=~0x03;
	t=t|0x01; // 0x01 : 25 MHz registers
				// 0x10 : 28 MHz registers
				// 0x00 : Internal VCLK registers
	// w* : keep 0x00
	isa_outb(SEQ_DATA,t);

	isa_outb(SEQ_INDEX,0x2a);
	numerator-=1;
	t=((divider-1)<<7)|numerator;
	isa_outb(SEQ_DATA,t);

	isa_outb(SEQ_INDEX,0x2b);
	t=isa_inb(SEQ_DATA)&128;
	postscaler-=1;
	denumerator-=1;
	t=0;
	t=t|((postscaler&3)<<5)|denumerator;
	t&=~0x80; // doesn't seem to work well - (gain for high freq operations)
	isa_outb(SEQ_DATA,t);

	isa_outb(SEQ_INDEX,0x13);
	t=isa_inb(SEQ_DATA);
	t=t&(0xbf);
	t=t+((postscaler&4)<<4);
	isa_outb(SEQ_DATA,t);
#ifdef SIS_VERBOSE
	sisReadVCLK();
#endif
	}

/*	-------- Hardware Specific Routines --------
	SiS 630
 */

// -------- SiS 630 Read Clock

void sis630_Read_ClockRegisters(char which_clock) {
	uchar numerator, denumerator;
	uchar divider, postscaler;
	uchar t;
	uchar clockRegIndex ;
	float clock;
	float refClock=14318180.0;

	vddprintf(("sis630: Read %cCLK Clocks Registers\n",which_clock));

	switch(which_clock) {
		case 'M':
			clockRegIndex = 0x28 ;
			break ;
		case 'D':
			clockRegIndex = 0x2b ;
			break ;
		case 'E':
			clockRegIndex = 0x2e ;
		} ;
	
		// register1
	isa_outb(SEQ_INDEX, clockRegIndex );
	t=isa_inb(SEQ_DATA);
	if (t&0x80) divider=2; else divider=1;
	numerator=(t&0x7f)+1;
	
		// register2
	isa_outb(SEQ_INDEX, clockRegIndex + 1 );
	t=isa_inb(SEQ_DATA);
	switch(t>>5) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 7:
			postscaler = (t>>5)+1 ;
			break;
		case 6:
			postscaler = 6 ;
			break;
		default:
			ddprintf(("sis: warning - postscaler invalid : SEQ(0x29)=0x%02x\n",t));
		}
	denumerator = (t&0x1f)+1;
	
	clock = refClock*(float)numerator/(float)denumerator ;
	clock = clock / (float)postscaler ;
	clock = clock * (float)divider ;
	vddprintf(("sis: %cCLK=%d MHz\n",which_clock,(int)(clock/1000000.0)));
	}

// -------- SiS 630 Read Clock

void sis630_Write_ClockRegisters(	char which_clock,
									uchar numerator,
									uchar denumerator,
									uchar divider,
									uchar postscaler
								) {
	uchar t;
	uchar clockRegIndex ;

	vddprintf(("sis630: Writing %cCLK Clocks Registers\n",which_clock));

	switch(which_clock) {
		case 'M':
			clockRegIndex = 0x28 ;
			break ;
		case 'D':
			clockRegIndex = 0x2b ;
			break ;
		case 'E':
			clockRegIndex = 0x2e ;
		} ;
	
		// register1
	isa_outb(SEQ_INDEX, clockRegIndex );
	if (divider==2) t=0x80; else t=0x00 ;
	t |= (numerator-1)&0x7f ;
	isa_outb(SEQ_DATA, t );
		
		// register2
	isa_outb(SEQ_INDEX, clockRegIndex + 1 );
	switch(postscaler) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 8:
			t = (postscaler-1) & 0x07 ;
			break;
		case 6:
			t = 0x06 ;
			break;
		default:
			ddprintf(("sis: warning - postscaler invalid %d\n",postscaler));
		}
	t = t << 5 ;
	t |= (denumerator-1)&0x1f ;
	isa_outb(SEQ_DATA, t );
	}
