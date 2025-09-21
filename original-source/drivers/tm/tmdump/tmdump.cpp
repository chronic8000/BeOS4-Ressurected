#include <stdio.h>
#include "tmmanapi.h"

extern "C" void Init_TMMan_API_Lib();

int  main()
{
	Init_TMMan_API_Lib();

	//display number of TM 
	{
		UInt32 n;
		n = tmmanDSPGetNum ();
		printf("%ld TriMedia within the system\n", n);
		printf("-----------------------------------------------------\n");
	}


	UInt32		DSPHandle;

	//open DSP
	{
		TMStatus	ret;
		ret = tmmanDSPOpen (0, &DSPHandle); 
		if (0 != ret)
		{
			printf("tmmanDSPOpen failed\n");
		}
	}

	//get info
	tmmanDSPInfo DSPInfo;
	{
		TMStatus ret;
		ret = tmmanDSPGetInfo 
			( 
			DSPHandle, 
			&DSPInfo 
			);
		if (0 != ret)
		{
			printf("tmmanDSPGetInfo failed\n");
		}
	}

	//display info
	{
		printf("SDRAM is 0x%08lx\n", DSPInfo.SDRAM.MappedAddress);
		printf("MMIO  is 0x%08lx\n", DSPInfo.MMIO.MappedAddress);
		printf("-----------------------------------------------------\n");
	}


	if (0)
	{
		//clear
		int *ptr = 0; 
		int *ptr2 = &ptr[DSPInfo.SDRAM.MappedAddress/4];
		int i;
		for (i=0;i<8*1024*1024/4;i++)
		{
			ptr2[i] = 0;
		}
	}
	else
	{
		//dump
		int *ptr = 0; 
		int *ptr2 = &ptr[DSPInfo.SDRAM.MappedAddress/4];
		{
			int i;
			for (i=0;i<130000;i++)
			{
				printf("%08X: %08X\n", (int) &ptr[i], ptr2[i]);
			}
			printf("-----------------------------------------------------\n");
		}
	}
	
	return 0;
}
