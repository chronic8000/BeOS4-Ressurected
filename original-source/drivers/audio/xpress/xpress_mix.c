#include "xpress.h"
#include "AC97defs.h"

/* xpress_ac97glue.c */
status_t xpress_cached_codec_write( void *, uchar, uint16, uint16 );
status_t xpress_cache_codec_values( void *, uchar );
uint16 	 xpress_codec_read(void *, uchar );
void	 lock_ac97(xpress_dev *, uchar);
void	 unlock_ac97(xpress_dev *, uchar);


void
write_initial_cached_controls( xpress_dev * pdev )
{
//	game_adc_info * padc = &(pdev->adc_info[0]);
//	game_dac_info * pdac = &(pdev->dac_info[0]);

	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_MASTER, 0x0000, 0xffff );		/* master   */
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_HEADPHONE, 0x8000, 0xffff );	/* headphone*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_MASTER_MONO, 0x8000, 0xffff );/* mono    	*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_PC_BEEP, 0x8000, 0xffff );	/* beep    	*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_PHONE, 0x8808, 0xffff );		/* phone   	*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_MIC, 0x8008, 0xffff );		/* mic     	*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_LINE, 0x8808, 0xffff );		/* line in 	*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_CD, 0x8808, 0xffff );			/* cd      	*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_VIDEO, 0x8808, 0xffff );		/* video   	*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_AUX, 0x8808, 0xffff );		/* aux     	*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_PCM, 0x0808, 0xffff );		/* pcm     	*/
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_REC_SEL, 0x0000, 0xffff );	/* rec select  */
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_REC_GAIN, 0x0000, 0xffff );	/* rec gain */
	xpress_cached_codec_write(&(pdev->xhost[0]), AC97_GENERAL_PURPOSE, 0x0000, 0xffff );/* gen purpose */
	xpress_cached_codec_write(&(pdev->xhost[0]), 0x2a, 0x1,    0xffff );			/* cvsr on  */
	xpress_cached_codec_write(&(pdev->xhost[0]), 0x2c, 0xAC44, 0xffff );			/* DAC		*/
	xpress_cached_codec_write(&(pdev->xhost[0]), 0x32, 0xAC44, 0xffff );			/* ADC		*/
}

void
read_initial_cached_controls( xpress_dev * pdev )
{
//	game_adc_info * padc = &(pdev->adc_info[0]);
//	game_dac_info * pdac = &(pdev->dac_info[0]);

	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_MASTER );		/* master   */
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_HEADPHONE );	/* headphone*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_MASTER_MONO );/* mono    	*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_PC_BEEP );	/* beep    	*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_PHONE );		/* phone   	*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_MIC );		/* mic     	*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_LINE );		/* line in 	*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_CD );			/* cd      	*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_VIDEO );		/* video   	*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_AUX );		/* aux     	*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_PCM );		/* pcm     	*/
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_REC_SEL );	/* rec select  */
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_REC_GAIN );	/* rec gain */
	xpress_cache_codec_values(&(pdev->xhost[0]), AC97_GENERAL_PURPOSE );/* gen purpose */
	xpress_cache_codec_values(&(pdev->xhost[0]), 0x2a );			/* cvsr on  */
	xpress_cache_codec_values(&(pdev->xhost[0]), 0x2c );			/* DAC		*/
	xpress_cache_codec_values(&(pdev->xhost[0]), 0x32 );			/* ADC		*/

}



/* control_id's _must_ correspond to their ordinal.... */
game_mixer_control xxx_controls[X_NUM_CONTROLS] = {
	/* adc mixer controls */	
	{GAME_MAKE_CONTROL_ID(X_RECORD_GAIN), GAME_MAKE_MIXER_ID(0), GAME_MIXER_CONTROL_IS_LEVEL,  0, 							 GAME_NO_ID, 0, 0}, /* record gain */
	{GAME_MAKE_CONTROL_ID(X_RECORD_MUX),  GAME_MAKE_MIXER_ID(0), GAME_MIXER_CONTROL_IS_MUX,    GAME_MIXER_CONTROL_AUXILIARY, GAME_NO_ID, 0, 0}, /* record mux  */
	{GAME_MAKE_CONTROL_ID(X_MIC_BOOST),   GAME_MAKE_MIXER_ID(0), GAME_MIXER_CONTROL_IS_ENABLE, GAME_MIXER_CONTROL_AUXILIARY, GAME_NO_ID, 0, 0}, /* mic boost   */
	{GAME_MAKE_CONTROL_ID(X_MIC_MUX),     GAME_MAKE_MIXER_ID(0), GAME_MIXER_CONTROL_IS_MUX,    GAME_MIXER_CONTROL_AUXILIARY, GAME_NO_ID, 0, 0}, /* mic mux  */
	/* dac mixer controls */
	{GAME_MAKE_CONTROL_ID(X_MASTER),      GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, 0, 						   GAME_NO_ID, 0, 0}, /* master     */
	{GAME_MAKE_CONTROL_ID(X_HEADPHONE),   GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, GAME_MIXER_CONTROL_ADVANCED, GAME_NO_ID, 0, 0}, /* headphone  */
	{GAME_MAKE_CONTROL_ID(X_MASTER_MONO), GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, GAME_MIXER_CONTROL_ADVANCED, GAME_NO_ID, 0, 0}, /* master mono*/
	{GAME_MAKE_CONTROL_ID(X_BEEP),        GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, GAME_MIXER_CONTROL_ADVANCED, GAME_NO_ID, 0, 0}, /* pc beep    */
	{GAME_MAKE_CONTROL_ID(X_PCM_OUT),     GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, 0, 						   GAME_NO_ID, 0, 0}, /* pcm        */
	{GAME_MAKE_CONTROL_ID(X_PHONE),       GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, 0,                           GAME_NO_ID, 0, 0}, /* phone      */
	{GAME_MAKE_CONTROL_ID(X_MIC_OUT),     GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, 0,						   GAME_NO_ID, 0, 0}, /* mic        */
	{GAME_MAKE_CONTROL_ID(X_LINE_IN),     GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, 0,						   GAME_NO_ID, 0, 0}, /* line in    */
	{GAME_MAKE_CONTROL_ID(X_CD),	      GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, 0,                           GAME_NO_ID, 0, 0}, /* cd         */
	{GAME_MAKE_CONTROL_ID(X_VIDEO),       GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, GAME_MIXER_CONTROL_ADVANCED, GAME_NO_ID, 0, 0}, /* video      */
	{GAME_MAKE_CONTROL_ID(X_AUX),	      GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_LEVEL, GAME_MIXER_CONTROL_ADVANCED, GAME_NO_ID, 0, 0}, /* aux        */
	{GAME_MAKE_CONTROL_ID(X_3D_HW_BYPASS),GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_ENABLE,GAME_MIXER_CONTROL_AUXILIARY|GAME_MIXER_CONTROL_ADVANCED, GAME_NO_ID, 0, 0}, /* 3d h/w bypass */
	{GAME_MAKE_CONTROL_ID(X_3D_SOUND),    GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_ENABLE,GAME_MIXER_CONTROL_AUXILIARY|GAME_MIXER_CONTROL_ADVANCED, GAME_NO_ID, 0, 0}, /* 3d sound   */
	{GAME_MAKE_CONTROL_ID(X_LOOPBACK),    GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_ENABLE,GAME_MIXER_CONTROL_AUXILIARY|GAME_MIXER_CONTROL_ADVANCED, GAME_NO_ID, 0, 0}, /* loopback   */
	{GAME_MAKE_CONTROL_ID(X_MONO_MUX),    GAME_MAKE_MIXER_ID(1), GAME_MIXER_CONTROL_IS_MUX,   GAME_MIXER_CONTROL_AUXILIARY|GAME_MIXER_CONTROL_ADVANCED, GAME_NO_ID, 0, 0}  /* mono mux  */
	
};

game_get_mixer_level_info xxx_level_info[X_NUM_LEVELS] = {
	/* adc level info */
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_RECORD_GAIN), GAME_MAKE_MIXER_ID(0), "Record Gain", GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0xF, 0x0, 0.0, 22.5, "%.1f dB", GAME_LEVEL_AC97_RECORD, 0x2 },
	/* dac level info */
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_MASTER),     GAME_MAKE_MIXER_ID(1), "Master" ,    GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x0,  0.0, -46.5, "%.1f dB", GAME_LEVEL_AC97_MASTER,    0x2 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_HEADPHONE),  GAME_MAKE_MIXER_ID(1), "Headphone",  GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x0,  0.0, -46.5, "%.1f dB", GAME_LEVEL_AC97_ALTERNATE, 0x2 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_MASTER_MONO),GAME_MAKE_MIXER_ID(1), "Master Mono",GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x0,  0.0, -46.5, "%.1f dB", GAME_LEVEL_AC97_MONO,      0x1 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_BEEP),       GAME_MAKE_MIXER_ID(1), "PC BEEP",    GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x0F, 0x0,  0.0, -45.0, "%.1f dB", GAME_LEVEL_AC97_PCBEEP,    0x1 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_PCM_OUT),    GAME_MAKE_MIXER_ID(1), "Sounds",     GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x8, 12.0, -34.5, "%.1f dB", GAME_LEVEL_AC97_PCM,       0x2 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_PHONE),      GAME_MAKE_MIXER_ID(1), "Phone"  ,    GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x8, 12.0, -34.5, "%.1f dB", GAME_LEVEL_AC97_PHONE,     0x1 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_MIC_OUT),    GAME_MAKE_MIXER_ID(1), "Mic",        GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x8, 12.0, -34.5, "%.1f dB", GAME_LEVEL_AC97_MIC,       0x1 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_LINE_IN),    GAME_MAKE_MIXER_ID(1), "Line",       GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x8, 12.0, -34.5, "%.1f dB", GAME_LEVEL_AC97_LINE_IN,   0x2 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_CD),         GAME_MAKE_MIXER_ID(1), "CD",         GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x8, 12.0, -34.5, "%.1f dB", GAME_LEVEL_AC97_CD,        0x2 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_VIDEO),      GAME_MAKE_MIXER_ID(1), "Video",      GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x8, 12.0, -34.5, "%.1f dB", GAME_LEVEL_AC97_VIDEO,     0x2 },
	{sizeof(game_get_mixer_level_info), GAME_MAKE_CONTROL_ID(X_AUX),        GAME_MAKE_MIXER_ID(1), "Aux",        GAME_LEVEL_HAS_MUTE | GAME_LEVEL_VALUE_IN_DB, 0x0, 0x1F, 0x8, 12.0, -34.5, "%.1f dB", GAME_LEVEL_AC97_AUX,       0x2 }

};

game_get_mixer_mux_info xxx_mux_info[X_NUM_MUXES] = {
	{sizeof(game_get_mixer_mux_info), GAME_MAKE_CONTROL_ID(X_RECORD_MUX), GAME_MAKE_MIXER_ID(0), "Record Source", 0, 0x1, 0, 8, NULL, GAME_MUX_AC97_RECORD_SELECT},
	{sizeof(game_get_mixer_mux_info), GAME_MAKE_CONTROL_ID(X_MIC_MUX),    GAME_MAKE_MIXER_ID(0), "Mic Select",    0, 0x1, 0, 2, NULL, GAME_MUX_AC97_MIC_SELECT},
	{sizeof(game_get_mixer_mux_info), GAME_MAKE_CONTROL_ID(X_MONO_MUX),   GAME_MAKE_MIXER_ID(1), "Mono Output",   0, 0x1, 0, 2, NULL, GAME_MUX_AC97_MONO_SELECT}
};

game_mux_item xxx_mux_items[X_NUM_MUX_ITEMS] = {
	{ 0x00, GAME_MAKE_CONTROL_ID(X_RECORD_MUX), 0, "Mic", {0,0} },
	{ 0x01, GAME_MAKE_CONTROL_ID(X_RECORD_MUX), 0, "CD In", {0,0} },
	{ 0x02, GAME_MAKE_CONTROL_ID(X_RECORD_MUX), 0, "Video In", {0,0} },
	{ 0x03, GAME_MAKE_CONTROL_ID(X_RECORD_MUX), 0, "Aux In", {0,0} },
	{ 0x04, GAME_MAKE_CONTROL_ID(X_RECORD_MUX), 0, "Line In", {0,0} },
	{ 0x05, GAME_MAKE_CONTROL_ID(X_RECORD_MUX), 0, "Stereo Mix", {0,0} },
	{ 0x06, GAME_MAKE_CONTROL_ID(X_RECORD_MUX), 0, "Mono Mix", {0,0} },
	{ 0x07, GAME_MAKE_CONTROL_ID(X_RECORD_MUX), 0, "Phone", {0,0} },
	{ 0x00, GAME_MAKE_CONTROL_ID(X_MIC_MUX),    0, "Mic 1", {0,0} },
	{ 0x01, GAME_MAKE_CONTROL_ID(X_MIC_MUX),    0, "Mic 2", {0,0} },
	{ 0x00, GAME_MAKE_CONTROL_ID(X_MONO_MUX),   0, "Mix", {0,0} },
	{ 0x01, GAME_MAKE_CONTROL_ID(X_MONO_MUX),   0, "Mic", {0,0} },
};

game_get_mixer_enable_info xxx_enable_info[X_NUM_ENABLES] = {
	{sizeof(game_get_mixer_enable_info), GAME_MAKE_CONTROL_ID(X_MIC_BOOST),   GAME_MAKE_MIXER_ID(0), "Mic Boost (20dB)",GAME_ENABLE_OFF, 0, GAME_ENABLE_AC97_MIC_BOOST,"Enabled", "Disabled"},
	{sizeof(game_get_mixer_enable_info), GAME_MAKE_CONTROL_ID(X_3D_HW_BYPASS),GAME_MAKE_MIXER_ID(1), "Bypass 3D H/W",   GAME_ENABLE_OFF, 0, GAME_ENABLE_AC97_GP_POP,   "Enabled", "Disabled"},
	{sizeof(game_get_mixer_enable_info), GAME_MAKE_CONTROL_ID(X_3D_SOUND),    GAME_MAKE_MIXER_ID(1), "3D Sound",        GAME_ENABLE_OFF, 0, GAME_ENABLE_AC97_GP_3D,    "Enabled", "Disabled"},
	{sizeof(game_get_mixer_enable_info), GAME_MAKE_CONTROL_ID(X_LOOPBACK),    GAME_MAKE_MIXER_ID(1), "ADC/DAC Loopback",GAME_ENABLE_OFF, 0, GAME_ENABLE_AC97_GP_LPBK,  "Enabled", "Disabled"}
};

inline int16 control_to_level_ordinal( uint16 control) {
	uint16 i;
	for (i = 0; i < X_NUM_LEVELS; i++) {
		if (control == xxx_level_info[i].control_id) {
			return i;
		}	
	}
	return -1;
}

inline int16 control_to_mux_ordinal( uint16 control) {
	uint16 i;
	for (i = 0; i < X_NUM_MUXES; i++) {
		if (control == xxx_mux_info[i].control_id) {
			return i;
		}	
	}
	return -1;
}

inline int16 control_mux_to_mux_item( uint16 control) {
	uint16 i;
	for (i = 0; i < X_NUM_MUX_ITEMS; i++) {
		if (control == xxx_mux_items[i].control) {
			return i;
		}	
	}
	return -1;
}


inline int16 control_to_enable_ordinal( uint16 control) {
	uint16 i;
	for (i = 0; i < X_NUM_ENABLES; i++) {
		if (control == xxx_enable_info[i].control_id) {
			return i;
		}	
	}
	return -1;
}

status_t
get_control_value(	open_xpress * card,
					game_mixer_control_value * pv )
{
	xpress_dev * pdev  = card->device;
	status_t err = B_OK;

	if (!GAME_IS_CONTROL( 	pv->control_id )|| 
		GAME_CONTROL_ORDINAL(pv->control_id) >= (X_NUM_CONTROLS)) {
		pv->control_id = GAME_NO_ID;
		err = B_BAD_VALUE;
	}
	else {
		uint16 ac_type;
		pv->type = xxx_controls[GAME_CONTROL_ORDINAL(pv->control_id)].type;
		
		if (pv->type == GAME_MIXER_CONTROL_IS_LEVEL) {
			ac_type = xxx_level_info[control_to_level_ordinal(pv->control_id)].type;
			switch (ac_type) {
				case GAME_LEVEL_AC97_RECORD:
				case GAME_LEVEL_AC97_MASTER:
				case GAME_LEVEL_AC97_ALTERNATE:
				case GAME_LEVEL_AC97_PCM:
				case GAME_LEVEL_AC97_LINE_IN:
				case GAME_LEVEL_AC97_CD:
				case GAME_LEVEL_AC97_VIDEO:
				case GAME_LEVEL_AC97_AUX:
					lock_ac97(pdev, pdev->xhost[0].index);
						pv->u.level.values[0] = (pdev->ccc[ac_type] & 0x1F00) >> 8;
						pv->u.level.values[1] = (pdev->ccc[ac_type] & 0x001F);
						pv->u.level.flags = pdev->ccc[ac_type] & 0x8000 ? GAME_LEVEL_IS_MUTED : 0;
					unlock_ac97(pdev, pdev->xhost[0].index);
					break;
				case GAME_LEVEL_AC97_MONO:
				case GAME_LEVEL_AC97_PHONE:
				case GAME_LEVEL_AC97_MIC:
					lock_ac97(pdev, pdev->xhost[0].index);
						pv->u.level.values[0] = (pdev->ccc[ac_type] & 0x001F);
						pv->u.level.flags = pdev->ccc[ac_type] & 0x8000 ? GAME_LEVEL_IS_MUTED : 0;
					unlock_ac97(pdev, pdev->xhost[0].index);
					break;
				case GAME_LEVEL_AC97_PCBEEP:
					lock_ac97(pdev, pdev->xhost[0].index);
						pv->u.level.values[0] = (pdev->ccc[ac_type] & 0x001F) >> 1;
						pv->u.level.flags = pdev->ccc[ac_type] & 0x8000 ? GAME_LEVEL_IS_MUTED : 0;
					unlock_ac97(pdev, pdev->xhost[0].index);
					break;
	
				default:
					KASSERT((0));
					pv->control_id = GAME_NO_ID;
					err = B_BAD_VALUE;
					break;
			}
		}	
		else if (pv->type == GAME_MIXER_CONTROL_IS_MUX) {
			ac_type = xxx_mux_info[control_to_mux_ordinal(pv->control_id)].type;
			switch (ac_type) {
				case GAME_MUX_AC97_RECORD_SELECT:
					lock_ac97(pdev, pdev->xhost[0].index);
						pv->u.mux.mask = pdev->ccc[ac_type];
					unlock_ac97(pdev, pdev->xhost[0].index);
					break;
				case GAME_MUX_AC97_MIC_SELECT:
				case GAME_MUX_AC97_MONO_SELECT:
					lock_ac97(pdev, pdev->xhost[0].index);
						pv->u.mux.mask = (1 << ((ac_type & 0xFF00)>>8)) & (pdev->ccc[(ac_type & 0xFF)]);
					unlock_ac97(pdev, pdev->xhost[0].index);
					break;	
				default:
					KASSERT((0));
					pv->control_id = GAME_NO_ID;
					err = B_BAD_VALUE;
					break;
			}
		}	
		else if (pv->type == GAME_MIXER_CONTROL_IS_ENABLE) {
			uint16 bit_mask = 1 << ((xxx_enable_info[control_to_enable_ordinal(pv->control_id)].type & 0xFF00)>>8);
			ac_type = xxx_enable_info[control_to_enable_ordinal(pv->control_id)].type & 0x00FF;

			lock_ac97(pdev, pdev->xhost[0].index);
				pv->u.enable.enabled = pdev->ccc[ac_type] & bit_mask;
			unlock_ac97(pdev, pdev->xhost[0].index);
		}	
		else {
			pv->control_id = GAME_NO_ID;
			err = B_BAD_VALUE;
		}
	}
	return err;
}

status_t
set_control_value(	open_xpress * card,
					game_mixer_control_value * pv )
{
	xpress_dev * pdev  = card->device;
	status_t err = B_OK;

	if (!GAME_IS_CONTROL( 	pv->control_id )|| 
		GAME_CONTROL_ORDINAL(pv->control_id) >= X_NUM_CONTROLS ) {
		pv->control_id = GAME_NO_ID;
		err = B_BAD_VALUE;
	}
	else {
		uint16 ac_type;
		uint16 reg_value = 0;
		pv->type = xxx_controls[GAME_CONTROL_ORDINAL(pv->control_id)].type;

		if (pv->type == GAME_MIXER_CONTROL_IS_LEVEL) {
			ac_type = xxx_level_info[control_to_level_ordinal(pv->control_id)].type;
			switch (ac_type) {
					case GAME_LEVEL_AC97_RECORD:
					case GAME_LEVEL_AC97_MASTER:
					case GAME_LEVEL_AC97_ALTERNATE:
					case GAME_LEVEL_AC97_PCM:
					case GAME_LEVEL_AC97_LINE_IN:
					case GAME_LEVEL_AC97_CD:
					case GAME_LEVEL_AC97_VIDEO:
					case GAME_LEVEL_AC97_AUX:
						reg_value = ((pv->u.level.values[0] & 0x1F) << 8) | (pv->u.level.values[1] & 0x1F);
						if (pv->u.level.flags & GAME_LEVEL_IS_MUTED) {
							reg_value |= 0x8000;
						}
						if (reg_value != pdev->ccc[ac_type]){					
							xpress_cached_codec_write(&(pdev->xhost[0]), ac_type, reg_value, 0xffff );
						}
						break;
					case GAME_LEVEL_AC97_MONO:
					case GAME_LEVEL_AC97_MIC:
					case GAME_LEVEL_AC97_PHONE:
						reg_value = (pv->u.level.values[0] & 0x1F);
						/* this picks up the 20 db bit for MIC    */
						/* should not affect PHONE, BEEP or MMONO */ 
						reg_value |=  (0x40 & pdev->ccc[ac_type]); 
						if (pv->u.level.flags & GAME_LEVEL_IS_MUTED) {
							reg_value |= 0x8000;
						}
						if (reg_value != pdev->ccc[ac_type]){					
							xpress_cached_codec_write(&(pdev->xhost[0]), ac_type, reg_value, 0xffff );
						}
						break;
					case GAME_LEVEL_AC97_PCBEEP:
						reg_value = (pv->u.level.values[0] & 0x0F) << 1;
						if (pv->u.level.flags & GAME_LEVEL_IS_MUTED) {
							reg_value |= 0x8000;
						}
						if (reg_value != pdev->ccc[ac_type]){					
							xpress_cached_codec_write(&(pdev->xhost[0]), ac_type, reg_value, 0xffff );
						}
						break;

		
					default:
						KASSERT((0));
						pv->control_id = GAME_NO_ID;
						err = B_BAD_VALUE;
						break;
			}			
		}
		else if (pv->type == GAME_MIXER_CONTROL_IS_MUX) {
			ac_type = xxx_mux_info[control_to_mux_ordinal(pv->control_id)].type;
			switch (ac_type) {
					case GAME_MUX_AC97_RECORD_SELECT:
						reg_value = pv->u.mux.mask;
						if (reg_value != pdev->ccc[ac_type]){					
							xpress_cached_codec_write(&(pdev->xhost[0]), ac_type, reg_value, 0xffff );
						}
						break;
					case GAME_MUX_AC97_MIC_SELECT:
					case GAME_MUX_AC97_MONO_SELECT:
						{
							uint16 bit_mask = 1 << ((xxx_mux_info[control_to_mux_ordinal(pv->control_id)].type & 0xFF00)>>8);
							ac_type = xxx_mux_info[control_to_mux_ordinal(pv->control_id)].type & 0x00FF;
							reg_value = pv->u.mux.mask ? (bit_mask | pdev->ccc[ac_type]) : (pdev->ccc[ac_type] & ~bit_mask);
							if (reg_value != pdev->ccc[ac_type]){					
								xpress_cached_codec_write(&(pdev->xhost[0]), ac_type, reg_value, 0xffff );
							}	
						}
						break;		
					default:
						KASSERT((0));
						pv->control_id = GAME_NO_ID;
						err = B_BAD_VALUE;
						break;
			}			
		}
		else if (pv->type == GAME_MIXER_CONTROL_IS_ENABLE) {		
			uint16 bit_mask = 1 << ((xxx_enable_info[control_to_enable_ordinal(pv->control_id)].type & 0xFF00)>>8);
			ac_type = xxx_enable_info[control_to_enable_ordinal(pv->control_id)].type & 0x00FF;
			reg_value = pv->u.enable.enabled ? (bit_mask | pdev->ccc[ac_type]) : (pdev->ccc[ac_type] & ~bit_mask);
			if (reg_value != pdev->ccc[ac_type]){					
				xpress_cached_codec_write(&(pdev->xhost[0]), ac_type, reg_value, 0xffff );
			}	
		}
		else {
			pv->control_id = GAME_NO_ID;
			err = B_BAD_VALUE;
		}
	}
	return err;
}

