///#define DEBUG
#include "debug.h"
#include "ess.h"


void ess_Speaker (sb_hw* hw, bool state);
int ess_Command( sb_hw *hw, unsigned char  tmp );
int ess_Write(sb_hw *hw, unsigned char reg, unsigned char data); // with polling
uint8 ess_Read(sb_hw *hw, unsigned char reg);	// with polling
uint8 ess_Get_Byte(sb_hw *hw);
inline void ess_Mixer_Write(sb_hw *hw, unsigned char reg, unsigned char data);
inline unsigned char ess_Mixer_Read(sb_hw *hw, unsigned char reg);
status_t  ess_Try_Card (sound_card_t *card );
void ess_Audio1_EXT (sound_card_t *card, int direction, int blksize );	/* channel 1 programmed in extended mode */ 
void ess_Audio2_EXT (sound_card_t *card, int direction, int blksize );	/* channel 2 programmed in extended mode */ 
void ess_Sample_Rate (sound_card_t *card, int sample_rate, int channel, int direction );



/* 
	these functions imposed by the audio_drive structure, some of the functions beeing called 
	from this module
*/
static status_t 	ess_Init(sound_card_t *card);
static status_t	 	ess_Uninit(sound_card_t *card);

static void 		ess_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	ess_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** addr);
static void 		ess_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch);

static void 		ess_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		ess_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** addr);
static void 		ess_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch);

static void 		ess_StartPcm(sound_card_t *card);
static void 		ess_StopPcm(sound_card_t *card);

static status_t		ess_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		ess_GetSoundSetup(sound_card_t *card, sound_setup *ss);
static void 		ess_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);

static status_t		ess_InitJoystick(sound_card_t * card);
static void 		ess_enable_gameport(sound_card_t * card);
static void 		ess_disable_gameport(sound_card_t * card);

static status_t 	ess_InitMPU401(sound_card_t * card);
static void 		ess_enable_mpu401(sound_card_t *card);
static void 		ess_disable_mpu401(sound_card_t *card);
static void 		ess_enable_mpu401_interrupts(sound_card_t *card);
static void 		ess_disable_mpu401_interrupts(sound_card_t *card);


static sound_card_functions_t ess_functions = 
{
 		&ess_Init,
 		&ess_Uninit,
 		&ess_StartPcm,
 		&ess_StopPcm,
 		&ess_GetClocks,		
 		&ess_SoundSetup,
 		&ess_GetSoundSetup,
		&ess_SetPlaybackSR,
 		&ess_SetPlaybackDmaBuf,
 		&ess_SetPlaybackFormat,
 		&ess_SetCaptureSR,
		&ess_SetCaptureDmaBuf,
 		&ess_SetCaptureFormat,	
		&ess_InitJoystick,
 		&ess_enable_gameport,
 		&ess_disable_gameport,
	 	&ess_InitMPU401,
 		&ess_enable_mpu401,
 		&ess_disable_mpu401,
 		&ess_enable_mpu401_interrupts,
     	&ess_disable_mpu401_interrupts,
     	NULL,
     	NULL,
     	NULL,
     	NULL
} ;

#define 		ESS_VENDOR_ID			0x3007316
#define 		ESS_DEVICE_ID			0xffffffff

#define 		ESS_LOGICAL_ID_68		0x68187316
#define 		ESS_LOGICAL_ID_69		0x69187316
#define 		ESS_LOGICAL_ID_78		0x78187316
#define 		ESS_LOGICAL_ID_79		0x79187316


	card_descrtiptor_t ess1868_descriptor = {			
		"ess1868",
		ISA, 
		&ess_functions,
		ESS_VENDOR_ID, 								
		ESS_DEVICE_ID,								
		ESS_LOGICAL_ID_68
	};

	card_descrtiptor_t ess1869_descriptor = {			
		"ess1869",
		ISA, 
		&ess_functions,
		ESS_VENDOR_ID, 								
		ESS_DEVICE_ID,								
		ESS_LOGICAL_ID_69
	};

	card_descrtiptor_t ess1878_descriptor = {			
		"ess1878",
		ISA, 
		&ess_functions,
		ESS_VENDOR_ID, 								
		ESS_DEVICE_ID,								
		ESS_LOGICAL_ID_78
	};

	card_descrtiptor_t ess1879_descriptor = {			
		"ess1879",
		ISA, 
		&ess_functions,
		ESS_VENDOR_ID, 								
		ESS_DEVICE_ID,								
		ESS_LOGICAL_ID_79
	};




// ------------------------------  all global variables  --------------------------- // 
static struct isa_module_info *isainfo;	// to read/write in ports 


 
///////////////////////////////        conventions in code     //////////////////////
// internal used functions  
// functions for ESS chips ess_
/////////////////////////////////////////////////////////////////////////////////////

int32 ess_Intr(void *data)
{ 
	uint8 tmp ;
	int32 ret = B_UNHANDLED_INTERRUPT;
   sound_card_t *card = (sound_card_t*)data ;
   sb_hw* hw = (sb_hw*)((sound_card_t *)data)->hw;


   acquire_spinlock ( &(hw->spinlock) ); 

// isainfo->write_io_8 ( hw->sb_config_base + 7, 0 ) ;
   tmp = isainfo->read_io_8 ( hw->sb_config_base + 6 );			
// isainfo->write_io_8 ( hw->sb_config_base + 7, 0x0f ) ;   
  

	if(tmp == 0)
	{
		ret = B_UNHANDLED_INTERRUPT;
		goto end_tag;
	}
	else 
	{
		DB_PRINTF(("Acknoledge interrupt......\n"));
		isainfo->read_io_8 ( hw->sb_port +  0xE ); // ack the interrupt 
		ret = B_HANDLED_INTERRUPT;
	}
	if ( tmp & 0x1 )
	{

	    if(card->pcm_capture_interrupt(card, hw->read_buf.whichHalf))
	    {
			ret = B_INVOKE_SCHEDULER;  		// someone need to records
		}	
		hw->read_buf.whichHalf++;
		hw->read_buf.whichHalf%=2;
//       	memset(hw->read_buf.data+hw->read_buf.whichHalf*hw->read_buf.size/2,0,hw->read_buf.size/2);		 
		goto end_tag;

   	}

    if ( ( tmp & 0x2 ) && ( ess_Mixer_Read (hw,  0x7A ) & 0x40 ) )
    {
		ret = B_HANDLED_INTERRUPT;
	    ess_Mixer_Write ( hw, 0x7A, 0x6f ) ;		// 0x6f
		{// Store internal time for synchronization 
			bigtime_t time = system_time();
			
			acquire_spinlock(&(hw->timers_data.Lock));	
			hw->timers_data.SystemClock = time;
			hw->timers_data.IntCount++;
			release_spinlock(&(hw->timers_data.Lock));
		}			

	    if(card->pcm_playback_interrupt(card, hw->write_buf.whichHalf))
	    {  // someone playbacks 
//	    	memset(hw->write_buf.data+hw->write_buf.whichHalf*hw->write_buf.size/2,0,hw->write_buf.size/2);
			ret = B_INVOKE_SCHEDULER;
		}
		hw->write_buf.whichHalf++;
		hw->write_buf.whichHalf%=2;
		goto end_tag;
    }    
 
	if ( tmp & ~0x3 )
	{
		ret = B_HANDLED_INTERRUPT;
	    DB_PRINTF(("Unwaited Interrupt\n"));
   	}
  	
end_tag :  	release_spinlock ( &(hw->spinlock) ); 
			return ret;         
}


/* ------------------------
	init_driver called every time the driver is loaded. Now you use ISA module 
------------------------ */

static status_t ess_Init (sound_card_t *card )
{
		status_t err = B_ERROR;
		sb_hw *hw ;
	
		uint64 cookie=0;
		int32 result, num;
		int irq, i;
		cpu_status st ;
			
		struct device_info info, *dev_info ;
		struct device_configuration *dev_config;
	    struct isa_info *isa_info;
		config_manager_for_driver_module_info *config_info;
		resource_descriptor resource[5];
	
dprintf("ess_Init()\n");

		DB_PRINTF(("I'm in Init SBPRO \n sizeof sb_hw %d = ", sizeof(sb_hw))) ; 
	
		card->hw = NULL ;
		card->hw = malloc(sizeof(sb_hw)) ;
		if ( card->hw == NULL )
		{
		   DB_PRINTF(("init_driver -------> error allocating memory\n")) ; 
		   return ENOMEM ;
		}
	
		memset(card->hw, 0, sizeof(sb_hw));				// zero it out
		hw = (sb_hw*)(card->hw);
		
	/* 		Write/Capture area 	 */
		hw->write_buf.area = -1 ;
	
	/*		Read/Playback area 	 */
		hw->read_buf.area = -1 ;
	
	
	/*	init config manager*/
		err=get_module(	B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&config_info);
		if ( err != B_OK) 
		{
			dprintf("ess1869: cannot get config manager\n");
			return err;
		}
	
		err=B_ERROR;
	/*	check for supported card*/
		while(config_info->get_next_device_info(B_ISA_BUS, &cookie, &info,
				sizeof(info)) == B_OK) 
		{
		
		if(info.config_status!=B_OK) continue;
			
	/* get ISA device information */
		dev_info = (struct device_info *) malloc(info.size);
		config_info->get_device_info_for(cookie, dev_info, info.size);
		isa_info = (struct isa_info *) ((char *) dev_info +
					dev_info->bus_dependent_info_offset);

		DB_PRINTF(("Loop for dev_info %lx\n",isa_info->logical_device_id  )) ; 

/*	get card configuration*/
		result=config_info->get_size_of_current_configuration_for(cookie);
		if(result<0) 
		{
			put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
			return B_ERROR;
		}	

		dev_config = NULL ;
		dev_config =(struct device_configuration*) malloc(result);
	
		if(!dev_config) 
		{
			free(dev_info);
			put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
			return B_ERROR;
		}
		
		if(config_info->get_current_configuration_for(cookie, dev_config, info.size)!=B_OK) 
		{
			free(dev_config);
			put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
			return B_ERROR;
		}

		DB_PRINTF(("Looking for resources ..........\n"));

		if (config_info->get_nth_resource_descriptor_of_type(
		    dev_config, 0, B_IO_PORT_RESOURCE, &resource[0], sizeof(resource[0])) < B_OK ||
		    config_info->get_nth_resource_descriptor_of_type(
			dev_config, 1, B_IO_PORT_RESOURCE, &resource[4], sizeof(resource[4])) < B_OK ||
		    config_info->get_nth_resource_descriptor_of_type(
			dev_config, 0, B_IRQ_RESOURCE, &resource[1], sizeof(resource[1])) < B_OK ||
		    config_info->get_nth_resource_descriptor_of_type(
			dev_config, 0, B_DMA_RESOURCE, &resource[2], sizeof(resource[2])) < B_OK ||
		    config_info->get_nth_resource_descriptor_of_type(
			dev_config, 1, B_DMA_RESOURCE, &resource[3], sizeof(resource[3])) < B_OK) 
		{
		    free(dev_config);
		    free(dev_info);
		    continue;
		}
	
		hw->sb_port = resource[0].d.r.minbase;
		hw->sb_mpu_port = resource[4].d.r.minbase;
		for ( i = 0; i < 16; i++)
		    if (resource[1].d.m.mask & (1 << i))
			break;
		hw->sb_irq = i;
		for (i = 0; i < 8; i++)
		    if (resource[2].d.m.mask & (1 << i))
			break;
		hw->sb_dma8 = i;
		for (i = 0; i < 8; i++)
		    if (resource[3].d.m.mask & (1 << i))
			break;
		hw->sb_dma16 = i;
		dprintf("ess1869: port 0x%x  mpu = 0x%x  irq = %d  dma8 = %d  dma16 = %d\n",
			hw->sb_port, hw->sb_mpu_port,  hw->sb_irq, hw->sb_dma8, hw->sb_dma16);
		free(dev_config);
		free(dev_info);
    	
    	}

		/* PnP ISA device found */
		put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
	
	/*	init isa*/
		err = B_OK ;
		if (isainfo == NULL) {
			err=get_module(B_ISA_MODULE_NAME, (module_info **)&(isainfo));
		}
		if(err != B_OK) 
		{
			dprintf("ess1869: cannot get ISA module\n");
		  	return err;
		} 
	 	 	
		hw->sb_init = false ;
	 	hw->SetupLockSem = create_sem (1, "SetupLockSem") ;		// access to internal resources
	
		if ( ess_Try_Card (card) != B_OK  )
		    return B_ERROR ;
		
		DB_PRINTF(("\tEnd of ESS_Init ......\n"));
		install_io_interrupt_handler(hw->sb_irq, ess_Intr, card, 0) ;
	    return B_OK;

}



/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
status_t ess_Uninit(sound_card_t * card) 
{
	sb_hw* hw = (sb_hw*)card->hw;
	DB_PRINTF(("ess_Uninit.......\n"));
	remove_io_interrupt_handler(hw->sb_irq, ess_Intr ,card);		
	if (hw->write_buf.area > 0 )
	   delete_area(hw->write_buf.area);
	if (hw->read_buf.area > 0 )
	   delete_area(hw->read_buf.area);
 	delete_sem( hw->SetupLockSem );
	if (isainfo != NULL) put_module(B_ISA_MODULE_NAME);
	isainfo = NULL;
	return B_OK ;
}



//////////////////////////////////////////////////////////////////////////
//				Functions which match with audio_drive library 			//
//////////////////////////////////////////////////////////////////////////

static void ess_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
  sb_hw* hw = (sb_hw*)card->hw;
  DB_PRINTF(("ess_SetPlaybackSR........%d....\n", sample_rate));

  hw->write_buf.SampleRate = sample_rate;
  hw->write_buf.SetSampleRate = true;
  ess_Sample_Rate (card, sample_rate, 2, SB_OUTPUT); 
} 


status_t 	ess_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** addr)
{ 
	int v_tmp,i  ;
	unsigned char tmp ;
	physical_entry table1;	
	uint32 nsize ;
	status_t err = B_OK ;
	area_id mem_area;
	sb_hw *hw = (sb_hw*)(card->hw) ;

	DB_PRINTF(("\tSetPlaybackDmaBuf allocation ............... \n"));	
	if ( hw->write_buf.area != -1 )
	    delete_area ( hw->write_buf.area ) ;    
	if ( (mem_area = find_area("SB_Write_area") ) > 0 )
	{
		DB_PRINTF(("Area already allocated"));   
	    delete_area (mem_area) ;    	
	}

	hw->write_buf.area = 0 ;
	hw->write_buf.data = NULL ;
	nsize = (size + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);	/* multiple of page size */
	hw->write_buf.size =  nsize;
	
	err = (hw->write_buf.area = create_area("SB_Write_area",
              (void **)&(hw->write_buf.data),
              B_ANY_KERNEL_ADDRESS, hw->write_buf.size, B_LOMEM, B_READ_AREA | B_WRITE_AREA));  // B_CONTIGUOUS 
	

	if ( err < B_OK ) 
	{
		 DB_PRINTF(("SetPlaybackDmaBuffer -------> error allocating Write area \n")) ; 
		 goto lab_err ; 
	}
	
	err = B_OK ;
	if(get_memory_map(hw->write_buf.data, hw->write_buf.size, &table1, 1) < B_OK) 
	{
		DB_PRINTF(("Playback-allocate_buffer(): get_memory_map() failed!\n"));	
		 goto lab_err ; 
	}
/*
    table1.size = 0x10000L - ((uint32) table1.address & 0xffff);
    if (table1.size < hw->write_buf.size) 
    {
	   if (table1.size > hw->write_buf.size/ 2)
	       hw->write_buf.size = table1.size;
	    else 
	    {
	       hw->write_buf.data += table1.size;
	       hw->write_buf.size -= table1.size;
		}
    }
*/
/*	
	if((uint32)table_w.address & 0xff000000) 
		 goto lab_err ; 

	if ((((uint32)table_w.address)+B_PAGE_SIZE) & 0xff000000) 
		 goto lab_err ; 
*/

 
	*addr = (void*)hw->write_buf.data ;  // you send the address 
	memset(*addr,0,hw->write_buf.size);
	DB_PRINTF(("Playback Before AudioX_EXT ......%d...\n", hw->write_buf.size >> 1 ));		
	ess_Audio2_EXT ( card, SB_OUTPUT, hw->write_buf.size >> 1 ) ;
	DB_PRINTF(("Playback After AudioX_EXT .........\n"));		

	DB_PRINTF(("Success allocating Playback buffer\n"));
	return B_OK ;	

lab_err :
	DB_PRINTF(("Fail allocating Playback buffer\n"));
	return B_ERROR ;	
	
}



static void	ess_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{ // they are not so many formats out there 8/16 bits  1/2 channels
  	sb_hw *hw = (sb_hw*)card->hw ; 

	if(num_of_bits != 8 && num_of_bits != 16) return; //bad format	
	if (num_of_ch != 2 && num_of_ch != 1) return; //bad format

    hw->sb_data_format = 0 ;
	
	if ( num_of_bits == 8 ) 
	{
	  if (num_of_ch == 2 ) 
	     hw->sb_data_format += DATA_8_STEREO ;
	  else
		 hw->sb_data_format += DATA_8_MONO ;        
	}	 
	
	if ( num_of_bits == 16 )
	{
	  if (num_of_ch == 2 ) 
	     hw->sb_data_format += DATA_16_STEREO ;
	  else
		 hw->sb_data_format += DATA_16_MONO ;    
	}

}


static void	ess_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{ 
  sb_hw *hw = (sb_hw*)card->hw ; 

  DB_PRINTF(("SetCaptureSampleRate ...........\n"));  
  
    hw->read_buf.SampleRate = sample_rate;
    hw->read_buf.SetSampleRate = true;
    ess_Sample_Rate (card, sample_rate, 1, SB_INPUT);   // ch1 for capturing
} 

static status_t	ess_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** addr)
{
	int v_tmp ;
	uint32 nsize ;
	area_id mem_area ;
	physical_entry table2 ;	
	status_t err = B_OK ;
	sb_hw *hw = (sb_hw*)(card->hw) ;
	hw->read_buf.size = size;	

	if (hw->read_buf.area != -1 ) 
	       delete_area(hw->read_buf.area);
	       
	if ( (mem_area = find_area("SB_Read_area") ) > 0 )
	{
		DB_PRINTF(("Area already allocated"));   
	    delete_area (mem_area) ;    	
	}

	hw->read_buf.area = 0 ;
	hw->read_buf.data = NULL ;
	nsize = (size + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);	/* multiple of page size */
	hw->read_buf.size =  nsize;
	DB_PRINTF(("Capture ->You have size %ld buffer...\n",size ));
	
	err = (hw->read_buf.area = create_area("SB_Read_area",
              (void **)&(hw->read_buf.data),
              B_ANY_KERNEL_ADDRESS, hw->read_buf.size, B_LOMEM , B_READ_AREA | B_WRITE_AREA));  // B_CONTIGUOUS 
	
	if ( err < B_OK ) 
	{
		 DB_PRINTF(("SetCaptureDmaBuf -------> error allocating Read area \n")) ; 
		 goto lab_err ; 
	}
	
	if ( get_memory_map(hw->read_buf.data, hw->read_buf.size, &table2, 1) < B_OK) 
	{
		DB_PRINTF(("allocate_buffer(): get_memory_map() failed!\n"));
		 goto lab_err ; 
	}

   table2.size = 0x10000L - ((uint32) table2.address & 0xffff);
    if (table2.size < hw->read_buf.size) 
    {
	   if (table2.size > hw->read_buf.size/ 2)
	       hw->read_buf.size = table2.size;
	    else 
	    {
	       hw->read_buf.data += table2.size;
	       hw->read_buf.size -= table2.size;
		}
    }

/*	
	if((uint32)table2.address & 0xff000000) 
		 goto lab_err ; 
	if ((((uint32)table2.address)+B_PAGE_SIZE) & 0xff000000) 
		 goto lab_err ; 
*/

	*addr = hw->read_buf.data ;
	memset(hw->read_buf.data, 0, hw->read_buf.size);
	
	ess_Audio1_EXT (card , SB_INPUT , hw->read_buf.size >> 1 );		

	DB_PRINTF(("Success allocating Record buffer\n"));
	return B_OK ;
	
lab_err :
	DB_PRINTF(("Fail allocating Record buffer\n"));
	return B_ERROR ; 
	
}

static void ess_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	ess_SetPlaybackFormat(card, num_of_bits, num_of_ch) ;
}



void	ess_StartPcm(sound_card_t *card)
{  
	sb_hw* hw = card->hw ;
	unsigned char tmp;
	int blksize, i, j  ;
	physical_entry table1, table2 ;	
	cpu_status st; 

	ess_Speaker (hw, false);

	DB_PRINTF(("START-----Counter before start DMA is hi = 0x%x, lo = 0x%x\n", 
			ess_Mixer_Read ( hw, 0x74 ), 
			ess_Mixer_Read ( hw, 0x76 ) ));
	blksize = 0x10000 - ( hw->write_buf.size >> 1 );
	ess_Mixer_Write( hw, ESS_2_COUNT_LOW, blksize & 0xff ) ;  
	ess_Mixer_Write( hw, ESS_2_COUNT_HIGH, (blksize & 0xff00) >> 8 ) ; 
	DB_PRINTF(("STOP-----Counter before start DMA is hi = 0x%x, lo = 0x%x\n", 
			ess_Mixer_Read ( hw, 0x74 ), 
			ess_Mixer_Read ( hw, 0x76 ) ));

	DB_PRINTF(("StartPcm...w/playback..%d..\n",hw->write_buf.size));  // ch2
	get_memory_map(hw->write_buf.data, hw->write_buf.size, &table1, 1);
	isainfo->start_isa_dma(hw->sb_dma16, table1.address, table1.size, 0x58, 0);

	DB_PRINTF(("StartPcm...w/capture .....\n"));	// ch1
	get_memory_map(hw->read_buf.data, hw->read_buf.size, &table2, 1);
	isainfo->start_isa_dma(hw->sb_dma8, table2.address, table2.size, 0x54, 0);					



	/* start the DMA transfer */  
   st = disable_interrupts() ;
   acquire_spinlock ( &(hw->spinlock) ); 

	hw->timers_data.IntCount = 0 ;		// no. of interrupts 
	
// for playback ch2
   tmp = ess_Mixer_Read ( hw, 0x78 ) ;		
   ess_Mixer_Write ( hw, 0x78, tmp | 3 ) ;
   hw->write_buf.whichHalf = 0 ;		


// for capture ch1
     tmp = ess_Read ( hw, 0xB8 ) ;		   
     ess_Write ( hw, 0xB8, (tmp | 1 ) & 0x0f ) ;		// start DMA for Audio1 ch.
     hw->read_buf.whichHalf = 0 ;		

   release_spinlock ( &(hw->spinlock) ); 
   restore_interrupts(st);
   spin ( 100000 ) ; 			// 100 msec
		
   ess_Mixer_Write ( hw, 0x7C, 0x99 ) ;	 	// vol for ch 2
   ess_Speaker (hw, true);

}


void	ess_StopPcm(sound_card_t *card)
{  

   sb_hw* hw = card->hw ;
   uint8 tmp ;
   cpu_status st ;
   
   DB_PRINTF(("\nStopPCM.........."));

   st = disable_interrupts() ;
   acquire_spinlock ( &(hw->spinlock) ); 
 
   tmp = ess_Read ( hw, 0xB8 ) ;	
   ess_Write ( hw, 0xB8, tmp & ~1  ) ;		// stop DMA for channel 1
   hw->read_buf.whichHalf = 0 ;		


   tmp = ess_Mixer_Read ( hw, ESS_2_CTRL1 ) ;		
   ess_Mixer_Write ( hw, ESS_2_CTRL1, tmp & ~3 ) ;		// stop DMA for channel 2
   hw->write_buf.whichHalf = 0 ;		

  release_spinlock ( &(hw->spinlock) ); 
  restore_interrupts(st);
	
}



void ess_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock)
{
	cpu_status cp;
	uint32 num_samples_per_half_buf;
 	double half_buf_time;
	uint64 int_count;
    sb_hw *hw = card->hw ;
	cp = disable_interrupts();	// critical section
 	acquire_spinlock(&(hw->timers_data.Lock));
 	
	*pSystemClock = hw->timers_data.SystemClock;	// system time
	int_count = hw->timers_data.IntCount;			// interrupt count
	
	release_spinlock(&(hw->timers_data.Lock));	
	restore_interrupts(cp); 	// end of the critical section
	
	// number of samples in half-buffer
	num_samples_per_half_buf = hw->write_buf.size 
								/2		//half of the buffer
								/2 		//16-bit format of samples
								/2;		//stereo

	// compute the time elapsed using the number of played-back samples in microsec
	half_buf_time = (double)num_samples_per_half_buf /(double)(hw->write_buf.SampleRate) * 1000000.0;  	
 	*pSampleClock = (bigtime_t)((double)int_count * half_buf_time + 0.5);
 	DB_PRINTF(("Count = %ld\tSystemTime - SampleTime = %ld\n",int_count, (long)(*pSystemClock - *pSampleClock) ));
}



static status_t		ess_InitJoystick(sound_card_t * card)
{
	int base_port = 0x201; // hardcoded for now
	card->joy_base = base_port;
	return B_OK;
}


static void 		ess_enable_gameport(sound_card_t * card)
{

}

static void 		ess_disable_gameport(sound_card_t * card)
{

}


static status_t 	ess_InitMPU401(sound_card_t * card)
{
    sb_hw *hw = (sb_hw*)card->hw ;
	card->mpu401_base = hw->sb_mpu_port ;
	return B_OK;
}


static void 		ess_enable_mpu401(sound_card_t *card)
{
}

static void 		ess_disable_mpu401(sound_card_t *card)
{
}

static void 		ess_enable_mpu401_interrupts(sound_card_t *card)
{
 int8 tmp ; 
 sb_hw *hw = (sb_hw*)card->hw ;
 tmp = isainfo->read_io_8 ( hw->sb_config_base + 7 ) ;
 isainfo->write_io_8 ( hw->sb_config_base + 7, tmp | 0x08 ) ;
}

static void 		ess_disable_mpu401_interrupts(sound_card_t *card)
{
 int8 tmp ; 
 sb_hw *hw = (sb_hw*)card->hw ;
 tmp = isainfo->read_io_8 ( hw->sb_config_base + 7 ) ;
 isainfo->write_io_8 ( hw->sb_config_base + 7, tmp | ~0x08 ) ;
}



void ess_Speaker (sb_hw* hw, bool state) 
{ 
  if (state) 
    ess_Command (hw, DSP_CMD_SPKON); 
  else 
    ess_Command (hw, DSP_CMD_SPKOFF);     
} 
 
 
/*	
	ESS card will be programmed only in extended mode because 
	only that mode supports 16 bit 44Khz DAC/ADC 
*/

/////////////////////////////////////////////////////////////////////////////////////////
//		
//                        functions for ESS 1868/1869/1878/1879
//////////////////////////////////////////////////////////////////////////////////////// 
int ess_Command( sb_hw *hw, unsigned char  tmp )
{
	int i ;
	for (i = 0 ; i < ESS_DELAY ; i ++ )
	{   
	   if ( ( isainfo->read_io_8 ( hw->sb_port + ESS_COMMAND ) & 0x80 ) ==  0  )  		// bit 7
	   {
	  		isainfo->write_io_8 ( hw->sb_port + ESS_COMMAND , tmp ) ;	
	        return 1 ;
	   }
     	else spin(ESS_PAUSE ) ;
	}
	DB_PRINTF(("ESS Commnand %02x timeout\n", tmp));	   
    return 0 ;
}


int ess_Write(sb_hw *hw, unsigned char reg, unsigned char data) // with polling
{
	DB_PRINTF(("Reg %02x := %02x\n", reg, data));
    if (!ess_Command (hw, reg)) 
    		return 0;
    return ess_Command (hw, data);
}



uint8 ess_Read(sb_hw *hw, unsigned char reg)	// with polling
{
	uint8 data;
    /* Read a byte from an extended mode register of ES18xx */
    if (0xA0 <=reg && reg <=0xBF && !ess_Command(hw , ESS_READ_EXTENDED ))
            return -1;
    if (!ess_Command (hw , reg))
            return -1;
	data = ess_Get_Byte(hw);
	DB_PRINTF(("Reg %02x == %02x\n", reg, data));
	return data;
}


uint8 ess_Get_Byte(sb_hw *hw)
{
	int i ;
	for (i = 0 ; i < ESS_DELAY ; i ++ )
	      if ( isainfo->read_io_8(hw->sb_port +  ESS_COMMAND ) & 0x40)
	            return isainfo->read_io_8(hw->sb_port + ESS_READ_DATA );
	         else 
	    	    spin(ESS_PAUSE ) ;
    DB_PRINTF(("ESS Get_Byte failed : 0x%x !!!\n", isainfo->read_io_8(hw->sb_port + ESS_READ_DATA)));
    return -1 ;
}

inline void ess_Mixer_Write(sb_hw *hw, unsigned char reg, unsigned char data)
{
	DB_PRINTF(("Mixer_Write reg %02x set to %02x\n", reg, data));
    isainfo->write_io_8(hw->sb_port + ESS_MIXER_ADDR, reg );
    isainfo->write_io_8(hw->sb_port + ESS_MIXER_DATA, data);
}

inline unsigned char ess_Mixer_Read(sb_hw *hw, unsigned char reg)
{
	unsigned char data;
    isainfo->write_io_8(hw->sb_port + ESS_MIXER_ADDR, reg);
	data = isainfo->read_io_8(hw->sb_port + ESS_MIXER_DATA);
	DB_PRINTF(("Mixer_Read  reg %02x now is %02x\n", reg, data));
    return data;
}


unsigned char ess_Config_Read(sb_hw *hw, unsigned char reg)
{
	isainfo->write_io_8(hw->sb_config_base , reg );
	return isainfo->read_io_8(hw->sb_config_base +1 ) ;
}


void ess_Config_Write(sb_hw *hw,unsigned char reg, unsigned char data)
{
	isainfo->write_io_8(hw->sb_config_base , reg );
	isainfo->write_io_8(hw->sb_config_base +1 , data );
}
 

status_t ess_Reset(sb_hw *hw) 
{
	int i;
	cpu_status st ;

	st = disable_interrupts() ;
	acquire_spinlock ( &(hw->spinlock) ); 
		
	isainfo->write_io_8(hw->sb_port + ESS_STATUS_INTR , 3 ) ;    // 0x01 - normal , 0x02 - FIFO 
	isainfo->read_io_8 (hw->sb_port + ESS_STATUS_INTR );
	spin(10000);
	isainfo->write_io_8(hw->sb_port + ESS_STATUS_INTR , 0 ) ;
	for ( i = 0 ; i < ESS_DELAY ; i++ )
	{ 
		if (isainfo->read_io_8 ( hw->sb_port + ESS_READ_BUF_STATUS ) & 0x80 )
		  if ( isainfo->read_io_8 (hw->sb_port + ESS_READ_DATA )	== 0xAA )  
		   {
		    release_spinlock ( &(hw->spinlock) ); 
    		restore_interrupts(st);
 		    return B_OK ;
 		   }
		spin(ESS_PAUSE);
	}

    release_spinlock ( &(hw->spinlock) ); 
    restore_interrupts(st); 		  
	return B_ERROR ;		   
}		   
	

status_t ess_Reset_fifo (sb_hw *hw)
{
    isainfo->write_io_8(hw->sb_port + ESS_STATUS_INTR , 0x02);
    isainfo->read_io_8(hw->sb_port + ESS_STATUS_INTR );
	spin(10000);    
    isainfo->write_io_8(hw->sb_port + ESS_STATUS_INTR, 0x00 );
	return B_OK;
}


status_t ess_Init_Config (sound_card_t *card )        /* ESS18XX  Initialization */ 
{ 
  	unsigned char cfg, 
  				irq_bits = 0, 
  				dma8_bits = 0, 
  				dma16_bits = 0, tmp ;
  	status_t st ;  
  	sb_hw		*hw = card->hw ; 					

  	DB_PRINTF(("+++++++++++ess_Init now ++++++++++++\n"));

  	st = disable_interrupts() ;
  	acquire_spinlock ( &(hw->spinlock) ); 
 
 
  	ess_Command (hw, ESS_EXTENDED ) ;			/* enable extended commands */
  	ess_Mixer_Write( hw , 0x00, 0x00)	;		/*  reset mixer registers */
  	ess_Write ( hw, ESS_1_TRANSF_TYPE, 2 ) ;	/* 4 bytes on request */
 
	// chose 1st LDN - audio 
	ess_Config_Write(hw, 0x7, 1 );
	// activate it LDN
	ess_Config_Write ( hw, 0x30, 1) ;
	
	DB_PRINTF(("Your CSN = %d \t your LDN = %d\n", ess_Config_Read(hw,0x6), ess_Config_Read(hw,0x7)));
	DB_PRINTF(("Your Audio I/O Addr = %lx\tMPU401 Addrd = %lx\n", 
	           ((ess_Config_Read(hw,0x60) & 0xf ) <<8) |  ess_Config_Read(hw,0x61) ,
	  	           ((ess_Config_Read(hw,0x64) & 0xf ) <<8) |  ess_Config_Read(hw,0x65) 
	           ));
	DB_PRINTF(("Your FM I/O Addr = %lx\n", 
	           ((ess_Config_Read(hw,0x62) & 0xf ) <<8) |  ( ess_Config_Read(hw,0x63) & 0xfc) 
	           ));
	
	DB_PRINTF(("Your IRQ Channel 1 = %d\tIRQ Channel 2 = %d\n", ess_Config_Read(hw,0x70) , ess_Config_Read(hw,0x72) 
	           ));
	
	DB_PRINTF(("Your DMA Channel 1 = %d\tDMA Channel 2 = %d\n", ess_Config_Read(hw,0x74) , ess_Config_Read(hw,0x75) 
	           ));
	 
	  /* in fact for extended commands */
	  	           	  	  
	/* Audio1 IRQ */							/* you have to have LDN#1 activated */
	ess_Config_Write(hw, 0x70, hw->sb_irq);
	/* Audio2 IRQ */
	ess_Config_Write(hw, 0x72, hw->sb_irq);
	/* Audio1 DMA */
	ess_Config_Write(hw, 0x74, hw->sb_dma8);	
	/* Audio2 DMA */
	ess_Config_Write(hw, 0x75, hw->sb_dma16);
	
	/* Enable Audio 1 DMA  */	
	ess_Write( hw, 0xB2 , 0x50);   		// ESS_DRQ_CONFIG 

	/* Enable Audio 1 IRQ */  
	ess_Write ( hw, 0xB1, 0x50 );		//  ESS_INTR_CONFIG		

	/* Enable Audio 2 IRQ */	  
	ess_Mixer_Write ( hw, ESS_2_CTRL2	, 0x43 );	

	/* Enable and set hardware volume interrupt */
	ess_Mixer_Write( hw, ESS_MIXER_MAST_VOL, 0x42);
	
	hw->sb_init = true ;
	
	DB_PRINTF(("Your IRQ Channel 1 = %d\tIRQ Channel 2 = %d\n", ess_Config_Read(hw,0x70) , ess_Config_Read(hw,0x72) 
	           ));
	
	DB_PRINTF(("Your DMA Channel 1 = %d\tDMA Channel 2 = %d\n", ess_Config_Read(hw,0x74) , ess_Config_Read(hw,0x75) 
	          ));
	
	if ( hw->sb_caps & ESS_CAP_NEW_RATE )					// communication between channels 
	  ess_Mixer_Write ( hw, 0x71, 0x30 ) ;
	
	  
	if ( hw->sb_caps & ESS_CAP_IS_3D )
	{
	  ess_Mixer_Write ( hw, 0x54, 0x8f ) ;
  	  ess_Mixer_Write ( hw, 0x56, 0x95 ) ;
	  ess_Mixer_Write ( hw, 0x58, 0x94 ) ;
	  ess_Mixer_Write ( hw, 0x5a, 0x80 ) ;
	}  

//	ess_Mixer_Write ( hw, 0x1C, 0x15 ); 

	tmp = ess_Read (hw, 0xA8 ) ;
	DB_PRINTF(("tmp = 0x%x", tmp));
	ess_Write(hw, 0xA8, ( (tmp & ~3 ) | 1) & 0x1b ) ;		// stereo      
    
    release_spinlock ( &(hw->spinlock) ); 
    restore_interrupts(st);
	
	
   DB_PRINTF(("+++++++++++ Already after init ESS ++++++++++\n"));
   return B_OK ;
} 



status_t ess_Identify(sb_hw *hw)
{
	cpu_status st ;
	int8 hi, lo ;
	
	
	if ( ess_Reset(hw) != B_OK )
	{
	   DB_PRINTF(("\tUnable to reset the card ....\n"));
	}

	st = disable_interrupts() ;
	acquire_spinlock ( &(hw->spinlock) ); 

	/* from here you will find anything else */	
	isainfo->write_io_8 (hw->sb_port + ESS_MIXER_ADDR , 0x40 ) ; 
	hi = isainfo->read_io_8 ( hw->sb_port + ESS_MIXER_DATA ) ;
	lo = isainfo->read_io_8 ( hw->sb_port + ESS_MIXER_DATA ) ;

	if ( hi != lo )	
	{
		hw->sb_version = (hi << 8) | lo ;
		hw->sb_dsp_ver = (hi << 8) | lo ;
		hw->sb_config_base  = (isainfo->read_io_8 ( hw->sb_port + ESS_MIXER_DATA )) << 8  ;
		hw->sb_config_base += isainfo->read_io_8 ( hw->sb_port + ESS_MIXER_DATA ) ;
		dprintf("ess1869: sb_version %x sb_dsp_ver %x sb_config_base %x \n", hw->sb_version, hw->sb_dsp_ver, hw->sb_config_base);
	    release_spinlock ( &(hw->spinlock) ); 
        restore_interrupts(st); 		  
  	    return B_OK ;
	}
err_ :
	DB_PRINTF(("Values are 0x%x....0x%x", hi, lo ));
    release_spinlock ( &(hw->spinlock) ); 
    restore_interrupts(st); 		  
	DB_PRINTF(("Something wrong with me ........\n"));  
	return B_ERROR ;
}




/* fills in fact structure ess_caps and programms some registers */
status_t  ess_Try_Card (sound_card_t *card )
{
	sb_hw* hw = (sb_hw*)card->hw;
	
dprintf("ess1869L ess_Try_Card()\n");

	if ( ess_Identify(hw) != B_OK ) 
	{
          DB_PRINTF(("ESS chip not found\n"));
          return B_ERROR ;
	}

	switch (hw->sb_version) 
	{
	case 0x1868:
		hw->sb_caps = ESS_CAP_DUPLEX_MONO | ESS_CAP_DUPLEX_SAME | ESS_CAP_CONTROL | ESS_CAP_HWV;
		break;
	case 0x1869:
		hw->sb_caps = ESS_CAP_PCM2 | ESS_CAP_IS_3D | ESS_CAP_RECMIX | ESS_CAP_NEW_RATE | ESS_CAP_AUXB | ESS_CAP_SPEAKER | ESS_CAP_MONO | ESS_CAP_MUTEREC | ESS_CAP_CONTROL | ESS_CAP_HWV;
		break;
	case 0x1878:
		hw->sb_caps = ESS_CAP_DUPLEX_MONO | ESS_CAP_DUPLEX_SAME | ESS_CAP_I2S | ESS_CAP_CONTROL | ESS_CAP_HWV;
		break;
	case 0x1879 | ESS_CAP_HWV:
		hw->sb_caps = ESS_CAP_PCM2 | ESS_CAP_IS_3D | ESS_CAP_RECMIX | ESS_CAP_NEW_RATE | ESS_CAP_AUXB | ESS_CAP_SPEAKER | ESS_CAP_I2S | ESS_CAP_CONTROL | ESS_CAP_HWV;
		break;
	case 0x1887:
		hw->sb_caps = ESS_CAP_PCM2 | ESS_CAP_RECMIX | ESS_CAP_AUXB | ESS_CAP_SPEAKER | ESS_CAP_DUPLEX_SAME | ESS_CAP_HWV;
		break;
	case 0x1888:
		hw->sb_caps = ESS_CAP_PCM2 | ESS_CAP_RECMIX | ESS_CAP_AUXB | ESS_CAP_SPEAKER | ESS_CAP_DUPLEX_SAME | ESS_CAP_HWV;
		break;
	default:
           DB_PRINTF(("Unsupported chip ESS%x\n", hw->sb_version));
           return B_ERROR ;
    }

   DB_PRINTF(("ESS%x chip found at address 0x%x\n", hw->sb_version, hw->sb_port));

   return ess_Init_Config( card ) ;
}
	

void ess_Audio1_EXT (sound_card_t *card, int direction, int blksize )	/* channel 1 programmed in extended mode */ 
{
	uint8 tmp ;
	sb_hw *hw = card->hw ;
	cpu_status st ;

	while (!hw->sb_init)
	    snooze(100);

	st = disable_interrupts();		
	acquire_spinlock ( &(hw->spinlock) ); 
	ess_Reset_fifo( hw );
	
	ess_Command ( hw, ESS_EXTENDED ) ;
			
	if ( direction == SB_OUTPUT )	 // playback 
	{		
		DB_PRINTF(("+++++\tAudio1_EXT playback %d\n", blksize));		

		ess_Write (hw,0xB8 , 0x04 ) ;			// ESS_1_CTRL2 DMA autoinitialize, DAC mode, first DMA is write 
				
		ess_Write(hw, ESS_1_TRANSF_TYPE, 2 ) ;			// demand DMA transfer 4 bytes 
		
		/* you must fill the sample_rate & transfer count */ 
		blksize = 0x10000 - blksize ;
		ess_Write ( hw,ESS_1_COUNT_HIGH, blksize & 0xff ) ;	  
		ess_Write ( hw,  ESS_1_COUNT_LOW, (blksize & 0xff00) >> 8 ) ;  
		
		ess_Write (hw, ESS_1_DIRECT_ACCESS, 0x00 ) ;  // stereo , 16bit , unsigned
		ess_Write (hw, ESS_1_CTRL1, 0x71 ) ;
		ess_Write (hw, ESS_1_CTRL1, 0xBC ) ;			

	}
	else
	{
		DB_PRINTF(("\t+++++++++++Audio1_EXT capture .......\n"));		
				
		/*program the input volume	*/
		ess_Write (hw , ESS_1_IN_VOL , 0xaa ) ;

		/* register  B8h */
		ess_Write (hw, ESS_1_CTRL2 , 0x0E ) ;			/* DMA autoinitialize, ADC mode */

		ess_Write(hw, ESS_1_TRANSF_TYPE , 2 ) ;			// demand DMA transfer 4 bytes 

		
		/* you must fill the sample_rate & transfer count */ 
		blksize = 0x10000 - blksize ;
		ess_Write ( hw, ESS_1_COUNT_LOW, blksize & 0xff ) ;
		ess_Write ( hw, ESS_1_COUNT_HIGH, (blksize & 0xff00) >> 8 ) ;
		
		ess_Write (hw, ESS_1_DIRECT_ACCESS, 0x00 ); // 0x80
		ess_Write (hw, ESS_1_CTRL1, 0x71 ) ;   // 0x71 stereo , 16bit , unsigned  was -->  0x51
		ess_Write (hw, ESS_1_CTRL1, 0xBC ) ;  //  ---> 0x9C
	}

    release_spinlock ( &(hw->spinlock) ); 
   	restore_interrupts(st);    	
	DB_PRINTF(("\t+++++++++++End of Audio1 EXT........\n"));
}

void ess_Audio2_EXT (sound_card_t *card, int direction, int blksize )		/* channel 2 programmed in extended mode */ 
{
	sb_hw * hw = card->hw ;
	cpu_status st ;
	unsigned char tmp ;

	while (!hw->sb_init)
	    snooze(100);

	st = disable_interrupts();		
	acquire_spinlock ( &(hw->spinlock) ); 			

	ess_Reset_fifo( hw );
	
	ess_Command ( hw, ESS_EXTENDED ) ;
	if ( direction == SB_OUTPUT )   // works like playback ch
	{
		
		DB_PRINTF(("+++++++++++\t\tAudio2_EXT  playback .......\n"));		
		blksize = 0x10000 - blksize ;
		ess_Mixer_Write( hw, ESS_2_COUNT_LOW, blksize & 0xff ) ;  
		ess_Mixer_Write( hw, ESS_2_COUNT_HIGH, (blksize & 0xff00) >> 8 ) ; 

		ess_Mixer_Write( hw, 0x7A , 0x67 ) ; /* unsigned , stereo, 16 bit samples ESS_2_CTRL2*/ 		
		ess_Mixer_Write( hw, 0x78 , 0x90 ) ; /* 4 bytes/transfer, autoinit( write 3 when the DMA starts)  ESS_2_CTRL1 */
		
	}
	else			
	{								//	works like capture ch
		DB_PRINTF(("+++++++++++\t\tAudio2_EXT  capture .......\n"));			
	}

    release_spinlock ( &(hw->spinlock) ); 
   	restore_interrupts(st);    		
}




void ess_Sample_Rate (sound_card_t *card, int sample_rate, int channel, int direction ) 
{ 
  int             divider; 
  unsigned char   bits = 0; 
  cpu_status st ;
  int dsp_current_sample_rate  = sample_rate ;
  sb_hw		*hw = (sb_hw*)card->hw ; 	
  	
  st = disable_interrupts() ;
  acquire_spinlock ( &(hw->spinlock) ); 
   	
  if (sample_rate < 4000) 
    dsp_current_sample_rate = 4000; 
  else if (sample_rate > 48000) 
    dsp_current_sample_rate = 48000; 


  if (hw->sb_caps & ESS_CAP_NEW_RATE) 
  {
 	    int div0, rate0, diff0, div1, rate1, diff1 ;
		// let us calculate the best clock choice 
		div0 = (793800 + (dsp_current_sample_rate / 2)) / dsp_current_sample_rate;
		rate0 = 793800 / div0;
		diff0 = (rate0 > dsp_current_sample_rate) ? (rate0 - dsp_current_sample_rate) : (dsp_current_sample_rate - rate0);
		
		div1 = (768000 + (dsp_current_sample_rate/ 2)) / dsp_current_sample_rate;
		rate1 = 768000 / div1;
		diff1 = (rate1 > dsp_current_sample_rate) ? (rate1 - dsp_current_sample_rate) : (dsp_current_sample_rate - rate1);
		
		if (diff0 < diff1) 
		{
			bits = 128 - div0;
			dsp_current_sample_rate = rate0;
		}
		else 
		{
			bits = 256 - div1;
			dsp_current_sample_rate = rate1;
		}		  
	}
	else
	{
	  if (sample_rate > 22000) 
	    { 
	      bits = 0x80;
	      divider = 256 - (uint)(795444.0/sample_rate + 0.5);   // 795500 <--> 795444 
	 	  DB_PRINTF(("Divider 0x%x\t filter clock 0x%x", bits, divider));
	      dsp_current_sample_rate = 795444 / (256 - divider); 
	    } 
	  else 
	    { 
	      divider = 128 - (397722 + sample_rate / 2) / sample_rate;  // 397700 <--> 397722
	      dsp_current_sample_rate = 397722 / (128 - divider); 
	    } 	

	  bits = (unsigned char) divider; 					// Set filter divider register        
	  if (sample_rate > 22000 )
		  bits |=0x80 ;
	}
		
    dsp_current_sample_rate = (dsp_current_sample_rate * 80) / 200;       // Set filter rolloff to 80% of sample_rate/2 
    divider = 256 - 7160000 / (dsp_current_sample_rate * 82);


	 if ( channel == 1 ) 
	 {
	 	  ess_Write (hw, ESS_1_SAMPLE_RATE , bits); 
		  ess_Write (hw, ESS_1_FILTER_CLOCK, divider );  	
	 }
	
	 if ( channel ==2 ) 
	 {
	 	    ess_Mixer_Write ( hw, ESS_2_SAMPLE_RATE, bits );
	     	ess_Mixer_Write (hw, ESS_2_FILTER_CLOCK, divider); 		
	 }
	   
	 DB_PRINTF(("Divider for channel.%d.0x%x\t filter clock 0x%x\n", channel, bits, divider));
	
	 release_spinlock ( &(hw->spinlock) ); 
	 restore_interrupts(st);
} 


status_t ess_SoundSetup(sound_card_t *card, sound_setup *sound) 
{
	int tmp;
	sb_hw *hw = card->hw ;
	
	status_t err = acquire_sem_etc(hw->SetupLockSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,5000000);
	if(err != B_OK) return err;

	tmp=0;
	if(!sound->left.aux1_mix_mute)
			tmp+=224-((sound->left.aux1_mix_gain<<3)&0xe0);
	if(!sound->right.aux1_mix_mute)
			tmp+=14-((sound->right.aux1_mix_gain>>1)&0x0e);
	ess_Mixer_Write(hw,ESS_AUX1_VOL , tmp);
  
	tmp=0;
	if(!sound->left.aux2_mix_mute)
			tmp+=224-((sound->left.aux2_mix_gain<<3)&0xe0);
	if(!sound->right.aux2_mix_mute)
			tmp+=14-((sound->right.aux2_mix_gain>>1)&0x0e);
	ess_Mixer_Write(hw,ESS_AUX2_VOL, tmp);

	tmp=0;
	if(!sound->left.line_mix_mute)
			tmp+=224-((sound->left.line_mix_gain<<3)&0xe0);
	if(!sound->right.line_mix_mute)
			tmp+=14-((sound->right.line_mix_gain>>1)&0x0e);
	ess_Mixer_Write(hw,ESS_LINE_VOL, tmp);

	tmp=0;
	if(!sound->left.dac_mute)
			tmp+=224-((sound->left.dac_attn<<2)&0xe0);
	if(!sound->right.dac_mute)
			tmp+=14-((sound->right.dac_attn>>2)&0x0e);
	ess_Mixer_Write(hw,ESS_DAC_VOL, tmp);

	tmp = 0 ;
	switch(sound->left.adc_source)
	{
	 case line : tmp = 6;
	 			break;
	 
	 case aux1 : tmp = 2 ;
	 			break ;
	 
	 case mic : tmp = 4 ;
	 			break;
	 }
	 
	 if ( tmp )
	     ess_Mixer_Write (hw,  ESS_SOURCE_SELECT, tmp ) ;
	 			
	release_sem(hw->SetupLockSem);

	return B_OK;	
}


/*  
	you should have a structure to initialise the sound_setup 
	only for extended mode 
*/ 
status_t	ess_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{

	uint8 tmp, tmp2;
	status_t err;
	sb_hw *hw = card->hw ;

	err = acquire_sem_etc(hw->SetupLockSem, 1, B_CAN_INTERRUPT | B_TIMEOUT ,5000000);
	if(err != B_OK) return err;


	ss->left.adc_source = ss->right.adc_source = mic ; /* the source */ 
	ss->left.adc_gain=ss->right.adc_gain = 15; 		/* no controls for channel.adc_gain */

	ss->left.mic_gain_enable = ss->right.mic_gain_enable=0;
	
	tmp=tmp2=ess_Mixer_Read(hw,ESS_AUX1_VOL) ;
	ss->left.aux1_mix_mute=ss->right.aux1_mix_mute=0;	
	tmp = 30-((tmp>>3)&30);   
	tmp2 = 30-((tmp2<<1)&30);
	ss->left.aux1_mix_gain=tmp;
	ss->right.aux1_mix_gain=tmp2;
	
	tmp=tmp2=ess_Mixer_Read(hw,ESS_AUX2_VOL) ;
	ss->left.aux2_mix_mute=ss->right.aux2_mix_mute=0;	
	tmp = 30-((tmp>>3)&30);   
	tmp2 = 30-((tmp2<<1)&30);
	ss->left.aux2_mix_gain=tmp;
	ss->right.aux2_mix_gain=tmp2;		

		
	tmp=tmp2=ess_Mixer_Read(hw,ESS_LINE_VOL) ;		
	ss->left.line_mix_mute=ss->right.line_mix_mute=0;
	tmp = 30-((tmp>>3)&30);   
	tmp2 = 30-((tmp2<<1)&30);
	ss->left.line_mix_gain=tmp;
	ss->right.line_mix_gain=tmp2;		

	tmp=tmp2=ess_Mixer_Read(hw,ESS_DAC_VOL) ;		
	ss->left.dac_mute = ss->right.dac_mute = 0;
	tmp = 60-((tmp>>2)&60);   
	tmp2 = 60-((tmp2<<2)&60);
	ss->left.dac_attn=tmp;
	ss->right.dac_attn=tmp2;		


	ss->sample_rate = kHz_44_1;			/*	only support 44.1kHz*/
	ss->playback_format = linear_16bit_little_endian_stereo;  /*only support 16 bit stereo playback*/
	ss->capture_format = linear_16bit_little_endian_stereo;
	ss->dither_enable = 1;			/*no control for dither_enable*/
	ss->loop_attn = 64;				/*no control for loop_attn*/
	ss->loop_enable = 0;	 		/*no control for loop back*/
	ss->output_boost = 0;	 		/*no control for output boost*/
	ss->highpass_enable = 0; 		/*no control for high pass filter*/
	ss->mono_gain = 64;				/*no control for speaker gain*/
	ss->mono_mute = 0; 				/*un/mute speaker*/

	release_sem(hw->SetupLockSem);
	return B_OK;
}


