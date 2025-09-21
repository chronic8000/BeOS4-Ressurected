
// usbraw.h
//
// Copyright 1999, Be Incorporated
//
// This is a private protocol between the usb_raw device driver and the USBKit.
// This protocol can and WILL change.  Please do not code to it.  This code is
// provided for illustration and feedback purposes only.
//

#ifndef _USB_RAW_H
#define _USB_RAW_H
#include <USB.h>

// Version 0.15
#define USB_RAW_DRIVER_VERSION 0x0015 

typedef enum 
{
	check_version = 0x1000,
	
	get_device_descriptor = 0x2000,
	get_configuration_descriptor,
	get_interface_descriptor,
	get_endpoint_descriptor,
	get_string_descriptor,
	get_other_descriptor,
			
	set_configuration = 0x3000,
	set_feature,
	clear_feature,
	get_status,
	get_descriptor,
	
	do_control = 0x4000,
	do_interrupt,
	do_bulk
} usb_raw_command_id;

typedef enum
{
	status_success = 0,
	
	status_failed,
	status_aborted,
	status_stalled,
	status_crc_error,
	status_timeout,
	
	status_invalid_configuration,
	status_invalid_interface,
	status_invalid_endpoint,
	status_invalid_string,
	status_not_enough_space
} usb_raw_command_status;

typedef union 
{
	struct {
		status_t status;
	} generic;
	
	struct {
		status_t status;
		usb_device_descriptor *descr;
	} device;
	struct {
		status_t status;
		usb_configuration_descriptor *descr;
		uint32 config_num;
	} config;
	struct {
		status_t status;
		usb_interface_descriptor *descr;
		uint32 config_num;
		uint32 interface_num;
	} interface;
	struct {
		status_t status;
		usb_endpoint_descriptor *descr;
		uint32 config_num;
		uint32 interface_num;
		uint32 endpoint_num;
	} endpoint;
	struct {
		status_t status;
		usb_string_descriptor *descr;
		uint32 string_num;
		size_t length;
	} string;
	struct {
		status_t status;
		int8 type;
		int8 index;
		int16 lang;
		void *data;
		size_t length;
	} descr;
	struct {
		status_t status;
		usb_descriptor *descr;
		uint32 config_num;
		uint32 interface_num;
		uint32 other_num;
		size_t length;
	} other;
	struct {
		status_t status;
		uint8 request_type;
		uint8 request;
		uint16 value;
		uint16 index;
		uint16 length;
		void *data;
	} control;
	struct {
		status_t status;
		uint32 interface;
		uint32 endpoint;
		void *data;
		size_t length;
	} interrupt;
	struct {
		status_t status;
		uint32 interface;
		uint32 endpoint;
		void *data;
		size_t length;
	} bulk;	
} usb_raw_command;

#endif
