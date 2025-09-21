#include <SupportDefs.h>
#include <ByteOrder.h>
#include <string.h>
#include <KernelExport.h>

#include <lendian_bitfield.h>
#include <ide_device_info.h>

/*#ifndef IDE_INFO_TEST
#define ide_dprintf dprintf
#else
#define ide_dprintf printf
#endif*/

static
void swapbuf16(uint16 *buf, int count)
{
	uint16 *bp=buf, *ep=buf+count;
	while(bp<ep) {
		uint16 tmp = *bp;
		*bp = tmp >> 8 | tmp << 8;
		bp++;
	}
}

/* prepare_ide_device_info:
** convert ide_device_info buffer from raw device format
** and native host format
*/

void prepare_ide_device_info(ide_device_info *di)
{
#if B_HOST_IS_BENDIAN
	swapbuf16((uint16*)di,256);
	di->current_capacity_in_sectors =
		di->current_capacity_in_sectors >> 16 |
		di->current_capacity_in_sectors << 16;
	di->user_addressable_sectors =
		di->user_addressable_sectors >> 16 |
		di->user_addressable_sectors << 16;
#elif B_HOST_IS_LENDIAN
	swapbuf16((uint16*)di->serial_number,10);
	swapbuf16((uint16*)di->firmware_revision,4);
	swapbuf16((uint16*)di->model_number,20);
#else
#error "Unknown host endianness"
#endif

	if(!di->field_validity.word_54_to_58_valid) {
		di->current_sectors_per_track = di->default_sectors_per_track;
		di->current_cylinders         = di->default_cylinders;
		di->current_heads             = di->default_heads;
	}
}

static
void printstring(void (*ide_dprintf)(), char *format, char *string, int count)
{
	const int maxlen = 40;
	char buf[maxlen+1];
	if(count>maxlen) count=maxlen;
	strncpy(buf, string, count);
	buf[count] = '\0';
	ide_dprintf(format, buf);
}

void print_ide_device_info(void (*ide_dprintf)(), ide_device_info *di)
{
	bool is_ata = IDE_PROTOCOL_IS_ATA(di);
	bool is_atapi = IDE_PROTOCOL_IS_ATAPI(di);

	ide_dprintf( "\nIDE device info:\n");
	printstring(ide_dprintf, "  Model number:           %s\n",
	            di->model_number, 40);
	printstring(ide_dprintf, "  Firmware revision:      %s\n",
	            di->firmware_revision, 8);
	printstring(ide_dprintf, "  Serial number:          %s\n",
	            di->serial_number, 20);
	ide_dprintf( "\n");

	{
		char *protocol_type[4] = {"ATA", "ATA", "ATAPI", "???"};
		ide_dprintf( "  Protocol type:          %s",
		             protocol_type[di->general_configuration.protocol_type]);
	}
	if(is_atapi)
	{
		int command_packet_size[4] = {12,16,0,0};
		ide_dprintf(" (%d byte command packets)",
			command_packet_size[di->general_configuration.command_packet_size]);
	}
	ide_dprintf("\n");

	if(is_atapi)
	{
		char *device_type[32] = {
			"Direct-access device",
			"Sequential-access device",
			"Printer device",
			"Processor device",
			"Write-once device",
			"CD-ROM device",
			"Scanner device",
			"Optical memory device",
			"Medium changer device",
			"Communications device",
			"???","???",
			"Array controller device",
			"???","???","???",
			"???","???","???","???","???","???","???","???",
			"???","???","???","???","???","???","???",
			"Unknown or no device type"
		};
		ide_dprintf( "  Device type:            %s\n",
		             device_type[di->general_configuration.device_type]);
		ide_dprintf( "                          %s media\n",
		             di->general_configuration.removable_media ?
		             "Removable" : "Non-removable" );
	}
	else {
		ide_dprintf( "  Disk type:              %s media\n",
		             di->general_configuration.removable_media ?
		             "Removable" : "Non-removable" );
	}
	if(is_atapi)
	{
		char *CMD_DRQ_type[4] = {
			"Microprocessor DRQ -- poll for 3ms",
			"Interupt DRQ -- wait 10ms for interrupt",
			"Accelerated DRQ -- poll for 50us",
			"reserved value"
		};
		ide_dprintf( "  CMD_DRQ_type:           %s\n",
		             CMD_DRQ_type[di->general_configuration.CMD_DRQ_type]);
	}
	
	if(is_ata) {
		ide_dprintf( "\n"
		             "  Logical CHS:            "
		             "%d cylinders, %d heads, %d sectors/track\n",
		             di->default_cylinders, di->default_heads,
		             di->default_sectors_per_track );
		if(di->field_validity.word_54_to_58_valid) {
			ide_dprintf( "  Current logical CHS:    "
			             "%d cylinders, %d heads, %d sectors/track\n",
			             di->current_cylinders, di->current_heads,
			             di->current_sectors_per_track );
			ide_dprintf( "  Current capacity:       %u sectors\n",
			             di->current_capacity_in_sectors);
		}
		if(di->capabilities.lba_supported) {
			ide_dprintf( "  LBA mode capacity:      %u sectors\n",
			             di->user_addressable_sectors);
		}

		ide_dprintf( "\n"
		             "  Max sectors/interrupt:  %d\n",
			di->multiple_sector_command_support.maximum_sectors_per_interupt);
		if(di->multiple_sector_command_setting.setting_valid) {
			ide_dprintf("  Curr.sectors/interrupt: %d\n",
			  di->multiple_sector_command_setting.current_sectors_per_interupt);
		}
	}

	//ide_dprintf("  capabilities 0x%x\n", *((uint16*)&di->capabilities));
	ide_dprintf( "\n"
	             "  Device capabilities:\n");
	ide_dprintf( "    DMA %ssupported\n",
	             di->capabilities.dma_supported ? "" : "NOT ");
	ide_dprintf( "    LBA %ssupported\n",
	             di->capabilities.lba_supported ? "" : "NOT ");
	ide_dprintf( "    can%s disable IORDY\n",
	             di->capabilities.iordy_can_disable ? "" : "not");
	ide_dprintf( "    %ssupport IORDY\n",
	             di->capabilities.iordy_supported ? "" : "may ");
	if(is_atapi) {
		ide_dprintf( "    %s reset device\n",
		             di->capabilities.software_reset_required ?
		             "software reset required to" :
		             "DEVICE_RESET command will");
		ide_dprintf( "    overlap operation %ssupported\n",
		             di->capabilities.overlap_operation_supported ?
		             "" : "NOT ");
		ide_dprintf( "    command queuing %ssupported\n",
		             di->capabilities.command_queuing_supported ? "" : "NOT ");
		ide_dprintf( "    interleaved dma %ssupported\n",
		             di->capabilities.interleaved_dma_supported ? "" : "NOT ");
	}
	ide_dprintf( "\n" );
	if(di->capabilities_50.capabilities_valid == 1) {
		ide_dprintf( "  Standby timer value is %svalid\n",
		             di->capabilities_50.standby_timer_value_valid ?
		             "" : "NOT ");
	}

	ide_dprintf( "  PIO data transfer mode number: %d\n",
	             di->pio_cycle_timing.pio_data_transfer_mode_number);
	
	
	if(di->field_validity.word_64_to_70_valid) {
		ide_dprintf( "  Enhanced PIO:\n" );
		ide_dprintf( "    Supported modes:  " );
		if(di->enhanced_pio_mode.supported_modes & 1) ide_dprintf("3 ");
		if(di->enhanced_pio_mode.supported_modes & 2) ide_dprintf("4 ");
		ide_dprintf( "\n" );
	}
	if(di->capabilities.dma_supported) {
		ide_dprintf( "  Multiword DMA:\n" );
		ide_dprintf( "    Supported modes:  " );
		if(di->multiword_dma_mode.supported_modes & 1) ide_dprintf("0 ");
		if(di->multiword_dma_mode.supported_modes & 2) ide_dprintf("1 ");
		if(di->multiword_dma_mode.supported_modes & 4) ide_dprintf("2 ");
		ide_dprintf( "\n"
		             "    Selected mode:    " );
		if(di->multiword_dma_mode.selected_mode & 1) ide_dprintf("0 ");
		if(di->multiword_dma_mode.selected_mode & 2) ide_dprintf("1 ");
		if(di->multiword_dma_mode.selected_mode & 4) ide_dprintf("2 ");
		ide_dprintf( "\n");
		
		if(di->field_validity.word_88_valid) {
			ide_dprintf( "  Ultra DMA:\n" );
			ide_dprintf( "    Supported modes:  " );
			if(di->ultra_dma_mode.supported_modes & 1) ide_dprintf("0 ");
			if(di->ultra_dma_mode.supported_modes & 2) ide_dprintf("1 ");
			if(di->ultra_dma_mode.supported_modes & 4) ide_dprintf("2 ");
			if(di->ultra_dma_mode.supported_modes & 8) ide_dprintf("3 ");
			if(di->ultra_dma_mode.supported_modes & 0x10) ide_dprintf("4 ");
			if(di->ultra_dma_mode.supported_modes & 0x20) ide_dprintf("5 ");
			if(di->ultra_dma_mode.supported_modes & 0x40) ide_dprintf("6 ");
			ide_dprintf( "\n"
			             "    Selected mode:    " );
			if(di->ultra_dma_mode.selected_mode & 1) ide_dprintf("0 ");
			if(di->ultra_dma_mode.selected_mode & 2) ide_dprintf("1 ");
			if(di->ultra_dma_mode.selected_mode & 4) ide_dprintf("2 ");
			if(di->ultra_dma_mode.selected_mode & 8) ide_dprintf("3 ");
			if(di->ultra_dma_mode.selected_mode & 0x10) ide_dprintf("4 ");
			if(di->ultra_dma_mode.selected_mode & 0x20) ide_dprintf("5 ");
			if(di->ultra_dma_mode.selected_mode & 0x40) ide_dprintf("6 ");
			ide_dprintf( "\n" );
		}
	}

	if(di->field_validity.word_64_to_70_valid) {
		ide_dprintf( "\n"
		             "  Minimum Multiword DMA cycle time:               %d\n",
		             di->minimum_multi_word_dma_cycle_time );
		ide_dprintf( "  Recommended Multiword DMA cycle time:           %d\n",
		             di->recomended_multi_word_dma_cycle_time );
		ide_dprintf( "  Minimum PIO cycle time without flow control:    %d\n",
		             di->minimum_pio_cycle_time_without_flow_control );
		ide_dprintf( "  Minimum PIO cycle time with IORDY flow control: %d\n",
		             di->minimum_pio_cycle_time_with_iordy_flow_control );
	}
	
	if(is_atapi && di->capabilities.overlap_operation_supported) {
		ide_dprintf( "  Typical release time after command received:    %d\n",
		              di->release_time_after_command_received );
		ide_dprintf( "  Typical release time after service:             %d\n",
		             di->release_time_after_service );
	}
	if( (di->major_version_atapi != 0x0000) &&
	    (di->major_version_atapi != 0xffff) ) {
		ide_dprintf( "  ATAPI major version: 0x%04x\n", di->major_version_atapi );
	}
	if( (di->minor_version_atapi != 0x0000) &&
	    (di->minor_version_atapi != 0xffff) ) {
		ide_dprintf( "  ATAPI minor version: 0x%04x\n", di->minor_version_atapi );
	}
	if( (di->major_version != 0x0000) &&
	    (di->major_version != 0xffff) ) {
		ide_dprintf( "  ATA major version: 0x%04x\n", di->major_version );
	}
	if( (di->minor_version != 0x0000) &&
	    (di->minor_version != 0xffff) ) {
		ide_dprintf( "  ATA minor version: 0x%04x\n", di->minor_version );
	}

	if(di->last_lun_word.last_lun > 0) {
		ide_dprintf("  Last LUN Identifier: %d\n", di->last_lun_word.last_lun);
	}

	if(di->command_set_feature_supported.valid == 1) {
		ide_dprintf( "\n"
		             "  Command sets/features supported:\n"
		             "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		             di->command_set_feature_supported.smart ? "    SMART feature set\n" : "",
		             di->command_set_feature_supported.security_mode ? "    Security Mode feature set\n" : "",
		             di->command_set_feature_supported.removable_media ? "    Removable Media feature set\n" : "",
		             di->command_set_feature_supported.power_management ? "    Power Management feature set\n" : "",
		             di->command_set_feature_supported.packet_command ? "    PACKET Command feature set (ATAPI)\n" : "",
		             di->command_set_feature_supported.write_cache ? "    write cache\n" : "",
		             di->command_set_feature_supported.look_ahead ? "    look-ahead\n" : "",
		             di->command_set_feature_supported.release_interrupt ? "    release interrupt\n" : "",
		             di->command_set_feature_supported.service_interrupt ? "    SERVICE interrupt\n" : "",
		             di->command_set_feature_supported.device_reset_command ? "    DEVICE RESET command\n" : "",
		             di->command_set_feature_supported.host_protected_area ? "    Host Protected Area feature set\n" : "",
		             di->command_set_feature_supported.write_buffer_command ? "    WRITE BUFFER command\n" : "",
		             di->command_set_feature_supported.read_buffer_command ? "    READ BUFFER command\n" : "",
		             di->command_set_feature_supported.nop_command ? "    NOP command\n" : "",
		             di->command_set_feature_supported.download_microcode_command ? "    DOWNLOAD MICROCODE command\n" : "",
		             di->command_set_feature_supported.read_write_dma_queued ? "    READ DMA QUEUED and WRITE DMA QUEUED command\n" : "",
		             di->command_set_feature_supported.cfa ? "    CFA feature set\n" : "",
		             di->command_set_feature_supported.advanced_power_management ? "    Advanced Power Management feature set\n" : "",
		             di->command_set_feature_supported.removable_media_status_notification ? "    Removable Media Status feature set\n" : ""
		);
	}
	if((di->command_set_feature_default & 0xc000) == 0x4000) {
		ide_dprintf( "\n"
		             "  Command sets/features enabled:\n"
		             "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		             di->command_set_feature_enabled.smart ? "    SMART feature set\n" : "",
		             di->command_set_feature_enabled.security_mode ? "    Security Mode feature set\n" : "",
		             di->command_set_feature_enabled.removable_media ? "    Removable Media feature set\n" : "",
		             di->command_set_feature_enabled.power_management ? "    Power Management feature set\n" : "",
		             di->command_set_feature_enabled.packet_command ? "    PACKET Command feature set (ATAPI)\n" : "",
		             di->command_set_feature_enabled.write_cache ? "    write cache\n" : "",
		             di->command_set_feature_enabled.look_ahead ? "    look-ahead\n" : "",
		             di->command_set_feature_enabled.release_interrupt ? "    release interrupt\n" : "",
		             di->command_set_feature_enabled.service_interrupt ? "    SERVICE interrupt\n" : "",
		             di->command_set_feature_enabled.device_reset_command ? "    DEVICE RESET command\n" : "",
		             di->command_set_feature_enabled.host_protected_area ? "    Host Protected Area feature set\n" : "",
		             di->command_set_feature_enabled.write_buffer_command ? "    WRITE BUFFER command\n" : "",
		             di->command_set_feature_enabled.read_buffer_command ? "    READ BUFFER command\n" : "",
		             di->command_set_feature_enabled.nop_command ? "    NOP command\n" : "",
		             di->command_set_feature_enabled.download_microcode_command ? "    DOWNLOAD MICROCODE command\n" : "",
		             di->command_set_feature_enabled.read_write_dma_queued ? "    READ DMA QUEUED and WRITE DMA QUEUED command\n" : "",
		             di->command_set_feature_enabled.cfa ? "    CFA feature set\n" : "",
		             di->command_set_feature_enabled.advanced_power_management ? "    Advanced Power Management feature set\n" : "",
		             di->command_set_feature_enabled.removable_media_status_notification ? "    Removable Media Status feature set\n" : ""
		);
	}
	if(di->removable_media.media_status_notification_supported == 1) {
		ide_dprintf( "\n"
		             "  Removable Media Status Notification supported\n");
		ide_dprintf( "  Software Write Protect %ssupported\n",
		             di->removable_media.device_write_protect ? "" : "NOT ");
	}
	ide_dprintf("\n");
}

status_t
get_selected_dma_mode(ide_device_info *di, uint32 *modep)
{
	int dmamodesselected = 0;
	uint32	drive_dma_mode = 0;
	
	if(di->multiword_dma_mode.selected_mode & 0x1) {
		dmamodesselected++;
		drive_dma_mode = 0x00;
	}
	if(di->multiword_dma_mode.selected_mode & 0x2) {
		dmamodesselected++;
		drive_dma_mode = 0x01;
	}
	if(di->multiword_dma_mode.selected_mode & 0x4) {
		dmamodesselected++;
		drive_dma_mode = 0x02;
	}
	if(di->field_validity.word_88_valid) {
		if(di->ultra_dma_mode.selected_mode & 0x1) {
			dmamodesselected++;
			drive_dma_mode = 0x10;
		}
		if(di->ultra_dma_mode.selected_mode & 0x2) {
			dmamodesselected++;
			drive_dma_mode = 0x11;
		}
		if(di->ultra_dma_mode.selected_mode & 0x4) {
			dmamodesselected++;
			drive_dma_mode = 0x12;
		}
		if(di->ultra_dma_mode.selected_mode & 0x8) {
			dmamodesselected++;
			drive_dma_mode = 0x13;
		}
		if(di->ultra_dma_mode.selected_mode & 0x10) {
			dmamodesselected++;
			drive_dma_mode = 0x14;
		}
		if(di->ultra_dma_mode.selected_mode & 0x20) {
			dmamodesselected++;
			drive_dma_mode = 0x15;
		}
		if(di->ultra_dma_mode.selected_mode & 0x40) {
			dmamodesselected++;
			drive_dma_mode = 0x16;
		}
	}
	if(dmamodesselected != 1)
		return B_ERROR;
	*modep = drive_dma_mode;
	return B_NO_ERROR;
}

#ifdef IDE_INFO_TEST

main()
{
	ide_device_info di;
	ide_dprintf("sizeof(device_config): %d\n", sizeof(di));
#if 0
#define printoff(off, var) \
	printf("word %d (byte %d) is at offset %d\n", \
	off, off*2, (char*)&dc.var-(char*)&dc)

	printoff(7, reserved_7_to_9);
	printoff(48, reserved_48);
	printoff(52, reserverd_52);
	printoff(55, current_logical_heads);
	printoff(57, current_capacity_in_sectors);
	printoff(60, user_addressable_sectors);
	printoff(70, reserved_70);
	printoff(88, ultra_dma_mode);
	printoff(92, reserved_92_to_125);
	printoff(160, reserved_160_to_255);
#endif
	memset(&di, 0xff, 512);
	{
		char *p = (char*)&di;
		*p++ = 0x80;
		//*p++ = 0x05; // ATA
		*p++ = 0x85; // ATAPI
		p+=18;
		*p++ = '2';
		*p++ = '1';
		*p++ = ' ';
		*p++ = '3';
		p+=22;
		*p++ = '.';
		*p++ = '1';
		*p++ = '1';
		*p++ = '0';
		p+=4;
		*p++ = 'y';
		*p++ = 'M';
		*p++ = 'D';
		*p++ = ' ';
		*p++ = 's';
		*p++ = 'i';
		*p++ = ' ';
		*p++ = 'k';
		p+=32+4;
		*p++ = 0xff;
		//*p++ = 0x6; // lba,iordy
	}
	swap_ide_device_info(&di);
	di.capabilities_50.capabilities_valid=1;
	print_ide_device_info(&di);
}

#endif
