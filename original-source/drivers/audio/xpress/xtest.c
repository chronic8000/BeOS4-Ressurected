#include <stdio.h>
#include <OS.h>
#include "game_audio.h"


char dev_name[] = "/dev/audio/game/xpress/1";


int main(int nargs,char *args[])
{
	int 			driver_handle;
	int				rval, i, loop = 1;
	char			device[256];
	//
	// Open the driver
	//
	strcpy(device,dev_name);
				
	driver_handle = open(device,3);
	if (-1 == driver_handle)
	{
		printf("Failed to open %s\n",device);
		return -1;
	}
	printf("\nOpened %s\n",device);
	
while(loop++) {
	int32 x,y;
	printf("----------------0 Exit-----------------------%d\n",loop);
	printf("1  Make Noise           \t 2 Init Codec First\n");
	printf("3  Don't Do Anything    \t 4 Do Anything\n");
	printf("----------------90 Test Du Jour---------------\n");
	if (!scanf("%d",&x) || 0 == x) {
		break;
	}
	printf("\n");
	switch(x) {
	case 1:
		{
		rval = ioctl(driver_handle,_GAME_QUEUE_UNUSED_OPCODE_2_,NULL,0);
		}
		break;	

	case  2:
		{
		rval = ioctl(driver_handle,GAME_MIXER_UNUSED_OPCODE_1_,NULL,0);
		}
		break;

	case  3:
		printf("Enter channels to set as hex number\n");
		if (!scanf("%x",&y)) {
			break;
		}
		break;

	case  4:
		break;

	case  5:
		break;

	case 8:
		break;

	case 11:
		break;

	case 10:
	case 12:
	case 13:
		{
		}
		break;

	case 14:
		break;

	case 20:
		break;
	case 30:
		break;
	case 31:
		break;
	case 32:
		break;
	case 33:
		break;

	case 90:
		//
		// Soup
		//
		{
			printf("yello.\n");
		}
		break;

	default:
		printf("This function is not currently supported\n\n");
		break;
	}
}	
exit:	
	close(driver_handle);
	return 0;
}


