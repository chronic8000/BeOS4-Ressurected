// timings.c

// Contains all timings known to the AppServer.  The first
// timing in the array (timings[0]) is the default timing.

#include <timings.h>


const nameptrd_display_timing timings[] = {
	
	"VESA 640x480 @ 60Hz",
	25.175,		// PCLK
	800, 640, 	// H total, addressable
	648, 144,	// H blank start, width
	656, 96,	// H sync start, width
	525, 480,	// V total, addressable
	488, 29,	// V blank start, width
	490, 2,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	
	// no need for 640 x 350
	// no need for 640 x 400
	// no need for 720 x 400

	"VESA 640x480 @ 72Hz",
	31.500,		// PCLK
	832, 640, 	// H total, addressable
	648, 176,	// H blank start, width
	664, 40,	// H sync start, width
	520, 480,	// V total, addressable
	488, 24,	// V blank start, width
	489, 3,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	"VESA 640x480 @ 75Hz",
	31.500,		// PCLK
	840, 640, 	// H total, addressable
	640, 200,	// H blank start, width
	656, 64,	// H sync start, width
	500, 480,	// V total, addressable
	480, 20,	// V blank start, width
	481, 3,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	"VESA 640x480 @ 85Hz",
	36.000,		// PCLK
	832, 640, 	// H total, addressable
	640, 192,	// H blank start, width
	696, 56,	// H sync start, width
	509, 480,	// V total, addressable
	480, 29,	// V blank start, width
	481, 3,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	// no need for 800x600 @ 56

	"VESA 800x600 @ 60Hz",
	40.000,		// PCLK
	1056, 800, 	// H total, addressable
	800, 256,	// H blank start, width
	840, 128,	// H sync start, width
	628, 600,	// V total, addressable
	600, 28,	// V blank start, width
	601, 4,		// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused
	
	"VESA 800x600 @ 72Hz",
	50.000,		// PCLK
	1040, 800, 	// H total, addressable
	800, 240,	// H blank start, width
	856, 120,	// H sync start, width
	666, 600,	// V total, addressable
	600, 66,	// V blank start, width
	637, 6,		// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused
	
	"VESA 800x600 @ 75Hz",
	49.500,		// PCLK
	1056, 800, 	// H total, addressable
	800, 256,	// H blank start, width
	816, 80,	// H sync start, width
	625, 600,	// V total, addressable
	600, 25,	// V blank start, width
	601, 3,		// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused
	
	"VESA 800x600 @ 85Hz",
	56.250,		// PCLK
	1048, 800, 	// H total, addressable
	800, 248,	// H blank start, width
	832, 64,	// H sync start, width
	631, 600,	// V total, addressable
	600, 31,	// V blank start, width
	601, 3,		// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused
	
	// No need for 1024x768 @ 43Hz

	"VESA 1024x768 @ 60Hz",
	65.000,		// PCLK
	1344, 1024,	// H total, addressable
	1024, 320,	// H blank start, width
	1048, 136,	// H sync start, width
	806, 768,	// V total, addressable
	768, 38,	// V blank start, width
	771, 6,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	"VESA 1024x768 @ 70Hz",
	75.000,		// PCLK
	1328, 1024,	// H total, addressable
	1024, 304,	// H blank start, width
	1048, 136,	// H sync start, width
	806, 768,	// V total, addressable
	768, 38,	// V blank start, width
	771, 6,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	"VESA 1024x768 @ 75Hz",
	78.750,		// PCLK
	1312, 1024, 	// H total, addressable
	1024, 288,	// H blank start, width
	1040, 96,	// H sync start, width
	800, 768,	// V total, addressable
	768, 32,	// V blank start, width
	769, 3,		// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused
	
	"VESA 1024x768 @ 85Hz",
	94.500,		// PCLK
	1376, 1024,	// H total, addressable
	1024, 352,	// H blank start, width
	1072, 96,	// H sync start, width
	808, 768,	// V total, addressable
	768, 40,	// V blank start, width
	769, 3,		// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused
	
	"VESA 1152x864 @ 75Hz",
	108.000,		// PCLK
	1600, 1152,	// H total, addressable
	1152, 448,	// H blank start, width
	1216, 128,	// H sync start, width
	900, 864,	// V total, addressable
	864, 36,	// V blank start, width
	865, 3,		// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused
	
	"VESA 1280x960 @ 60Hz",
	108.000,		// PCLK
	1800, 1280,	// H total, addressable
	1280, 520,	// H blank start, width
	1376, 112,	// H sync start, width
	1000, 960,	// V total, addressable
	960, 40,	// V blank start, width
	961, 3,		// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	"VESA 1280x960 @ 85Hz",
	148.500,	// PCLK
	1728, 1280,	// H total, addressable
	1280, 448,	// H blank start, width
	1344, 160,	// H sync start, width
	1011, 960,	// V total, addressable
	960, 51,	// V blank start, width
	961, 3,		// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	"VESA 1280x1024 @ 60Hz",
	108.000,	// PCLK
	1688, 1280,	// H total, addressable
	1280, 408,	// H blank start, width
	1328, 112,	// H sync start, width
	1066, 1024,	// V total, addressable
	1024, 42,	// V blank start, width
	1025, 3,	// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	"VESA 1280x1024 @ 75Hz",
	135.000,	// PCLK
	1688, 1280,	// H total, addressable
	1280, 408,	// H blank start, width
	1296, 144,	// H sync start, width
	1066, 1024,	// V total, addressable
	1024, 42,	// V blank start, width
	1025, 3,	// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	"VESA 1280x1024 @ 85Hz",
	157.500,	// PCLK
	1728, 1280,	// H total, addressable
	1280, 448,	// H blank start, width
	1344, 160,	// H sync start, width
	1072, 1024,	// V total, addressable
	1024, 48,	// V blank start, width
	1025, 3,	// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	"VESA 1600x1200 @ 60Hz",
	162.000,	// PCLK
	2160, 1600,	// H total, addressable
	1600, 560,	// H blank start, width
	1664, 192,	// H sync start, width
	1250, 1200,	// V total, addressable
	1200, 50,	// V blank start, width
	1201, 3,	// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	"VESA 1600x1200 @ 65Hz",
	175.500,	// PCLK
	2160, 1600,	// H total, addressable
	1600, 560,	// H blank start, width
	1664, 192,	// H sync start, width
	1250, 1200,	// V total, addressable
	1200, 50,	// V blank start, width
	1201, 3,	// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	"VESA 1600x1200 @ 70Hz",
	162.000,	// PCLK
	2160, 1600,	// H total, addressable
	1600, 560,	// H blank start, width
	1664, 192,	// H sync start, width
	1250, 1200,	// V total, addressable
	1200, 50,	// V blank start, width
	1201, 3,	// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	"VESA 1600x1200 @ 75Hz",
	202.500,	// PCLK
	2160, 1600,	// H total, addressable
	1600, 560,	// H blank start, width
	1664, 192,	// H sync start, width
	1250, 1200,	// V total, addressable
	1200, 50,	// V blank start, width
	1201, 3,	// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	"VESA 1600x1200 @ 85Hz",
	229.500,	// PCLK
	2160, 1600,	// H total, addressable
	1600, 560,	// H blank start, width
	1664, 192,	// H sync start, width
	1250, 1200,	// V total, addressable
	1200, 50,	// V blank start, width
	1201, 3,	// V sync start, width
	B_POSITIVE_HSYNC | B_POSITIVE_VSYNC,
	0, 0,		// unused

	
	"Apple 512x384 @ 60Hz",
	15.6672,   	// PCLK
	640, 512, 	// H total, addressable
	512, 128,	// H blank start, width
	544, 32,	// H sync start, width
	407, 384,	// V total, addressable
	384, 23,	// V blank start, width
	387, 3,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	"Apple 13\" (640x480 @ 67Hz)",
	30.240,		// PCLK
	864, 640, 	// H total, addressable
	640, 224,	// H blank start, width
	704, 64,	// H sync start, width
	525, 480,	// V total, addressable
	480, 45,	// V blank start, width
	483, 3,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	"Apple Portrait (640x870 @ 75Hz)",
	57.2832,		// PCLK
	832, 640, 	// H total, addressable
	640, 192,	// H blank start, width
	720, 80,	// H sync start, width
	918, 870,	// V total, addressable
	870, 48,	// V blank start, width
	873, 3,		// V sync start, width
	0,			// flags
	0, 0,		// unused

	"Apple 16\" (832x624 @ 75Hz)",
	57.2832,		// PCLK
	1152, 832, 	// H total, addressable
	832, 320,	// H blank start, width
	896, 64,	// H sync start, width
	667, 624,	// V total, addressable
	624, 43,	// V blank start, width
	627, 3,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	"Apple 19\" (1024x768 @ 75Hz)",
	80.000,		// PCLK
	1328, 1024, // H total, addressable
	1024, 304,	// H blank start, width
	1120, 96,	// H sync start, width
	804, 768,	// V total, addressable
	768, 36,	// V blank start, width
	771, 3,		// V sync start, width
	0,			// flags
	0, 0,		// unused
	
	"Apple 21\" (1152x870 @ 75Hz)",
	100.000,  	// PCLK
	1456, 1152,	// H total, addressable
	1152, 304,	// H blank start, width
	1280, 128,	// H sync start, width
	915, 870,	// V total, addressable
	870, 45,	// V blank start, width
	873, 3,		// V sync start, width
	0,			// flags
	0, 0		// unused	
	
};
