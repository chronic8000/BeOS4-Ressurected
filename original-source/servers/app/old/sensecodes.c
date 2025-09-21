enum {
	B_640x480  = B_8_BIT_640x480  | B_16_BIT_640x480  | B_32_BIT_640x480,
	B_832x624  = 0, // BeOS doesn't support 832x624
	B_1024x768 = B_8_BIT_1024x768 | B_16_BIT_1024x768 | B_32_BIT_1024x768,
	B_1152x870 = 0  // BeOS doesn't support 1152x870
};


float MapCableSenseToRefresh( ulong raw, ulong ext, ulong space )
{
	static const struct {
	  ulong space;
	  float refresh;
	} map[] = {
		B_640x480,  66.667,
		B_832x624,  74.550,
		B_1024x768, 74.927,
		B_1152x870, 75.062
	};
	
	// special case non-Apple timings
	if( raw == 7 && ext == 0x17 ) {
		return 60.0;
	}

	for( i=0; i<sizeof(map)/sizeof(map[0]); i++ ) {
		if( map[i].space & space ) {
			return map[i].refresh;
		}
	}

	return 0.0;
}


ulong MapCableSenseToScreenSpaces( ulong raw, ulong ext )
{
	static const struct {
	  ulong raw;
	  ulong ext;
	  ulong space;
	} map[] = {
		0, 0x00, 0,		// We don't support Apple 21" timings
		1, 0x14, 0,		// We don't support mono portrait displays
		2, 0x21, 0,		// We don't support 12" displays
		3, 0x31, 0,		// We don't support Apple 21" timings
		3, 0x34, 0,		// We don't support mono 21" displays
		3, 0x35, 0,		// We don't support mono 21" displays
		4, 0x0A, 0,		// We don't support NTSC display
		5, 0x1E, 0,		// We don't support portrait displays
		6, 0x03, B_640x480 | B_832x624,
	    6, 0x0B, B_640x480 | B_832x624 | B_1024x768,
		6, 0x23, B_640x480 | B_832x624 | B_1024x768 | B_1152x870,
		7, 0x00, 0,		// We don't support PAL
		7, 0x14, 0,		// We don't support NTSC
		7, 0x17, B_640x480, // VGA, NOT 67Hz!
		7, 0x2D, B_832x624,
		7, 0x30, 0,		// We don't support PAL
		7, 0x3A, B_1024x768,
		7, 0x3F, 0		// no display connected
	};

	for( i=0; i<sizeof(map)/sizeof(map[0]); i++ ) {
		if( map[i].raw == raw && map[i].ext == ext ) {
			return map[i].space;
		}
	}

	return 0;
}

