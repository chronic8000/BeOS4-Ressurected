/******************************************************************************
 * Intel i8255x (82557, 58, 59) Ethernet Controller Driver
 * Copyright (c) 2001 Be Incorporated, All Rights Reserved.
 *
 * Written by: Chris Liscio <liscio@be.com>
 *
 * File:				i8255x_regs.h
 * Description:			8255x register listings
 *
 * Revision History:	March 29, 2001	- Initial Revision
 *****************************************************************************/

#ifndef I8255X_REGS_H
#define I8255X_REGS_H

/*
 * The CSR (Command/Status Registers) are defined below.  Intel recommends 
 * that the SCB_STATUS and SCB_COMMAND words are accessed using byte-wide
 * accesses to prevent walking over any important high-byte status.
 */
enum _8255x_CSR {
	CSR_SCB_STATUS		= 0x00,	// 16 bits - System Control Block Status
	CSR_SCB_COMMAND		= 0x02,	// 16 bits - System Control Block Command
	CSR_SCB_GEN_PTR		= 0x04,	// 32 bits - System Control Block General Pointer
	CSR_PORT			= 0x08,	// 32 bits - PORT Interface
	CSR_FLASH_CTRL		= 0x0C,	// 16 bits - Flash Control Register
	CSR_EEP_CTRL		= 0x0E,	// 16 bits - EEPROM Control Register
	CSR_MDI_CTRL		= 0x10,	// 32 bits - MDI Control Register
	CSR_FLOW_CTRL		= 0x19,	// 16 bits - Flow Control Register
	CSR_PMDR			= 0x21,	// 8  bits - Power Management Driver Register
};

/*
 * Note that the upper byte contains interrupt-time status and the lower
 * byte contains general operation status.  It is recommended that we
 * read the SCB_STATUS register using byte-wide accesses during non-
 * interrupt time so that we don't accidentally step over the important
 * interrupt status.
 */
enum _8255x_SCB_STATUS {
	STATUS_CX			= 0x8000,	// CU finished command with interrupt bit
	STATUS_FR			= 0x4000,	// RU finished receiving a frame
	STATUS_CNA			= 0x2000,	// CU has left the active state
	STATUS_RNR			= 0x1000,	// RU no longer in the ready state
	STATUS_MDI			= 0x0800,	// MDI read or write cycle complete
	STATUS_SWI			= 0x0400,	// Software generated interrupt
	STATUS_FCP			= 0x0100,	// Flow Control Pause
	STATUS_CUS_IDLE		= 0x0000,	// CU Status - Idle
	STATUS_CUS_SUSP		= 0x0040,	// CU Status - Suspended
	STATUS_CUS_ACTV		= 0x0080,	// CU Status - Active
	STATUS_RUS_IDLE		= 0x0000,	// RU Status - Idle
	STATUS_RUS_RDY		= 0x0010,	// RU Status - Ready
	STATUS_RUS_SUSP_NRBD= 0x0024,	// RU Status - Suspended w/no more RBDs
	STATUS_RUS_NRDY_NRBD= 0x0028,	// RU Status - No resources (no more RBDs)
	STATUS_RUS_RDY_NRBD	= 0x0030,	// RU Status - Ready with no RBDs present
};

enum _8255x_SCB_COMMAND {
	COMMAND_MASK_CX			= 0x8000,	// Mask the CX interrupt
	COMMAND_MASK_FR			= 0x4000,	// Mask the FR interrupt
	COMMAND_MASK_CNA		= 0x2000,	// Mask the CNR interrupt
	COMMAND_MASK_RNR		= 0x1000,	// Mask the RNR interrupt
	COMMAND_MASK_ER			= 0x0800,	// Mask the ER interrupt
	COMMAND_MASK_FCP		= 0x0400,	// Mask the FCP interrupt
	COMMAND_SI				= 0x0200,	// Generate a software interrupt
	COMMAND_M				= 0x0100,	// Mask all interrupts
/* CU Commands */
	COMMAND_CUC_NOP				= 0x0000,	// NOP - does not affect current state
	COMMAND_CUC_CU_START		= 0x0010,	// CU Start
	COMMAND_CUC_CU_RESUME		= 0x0020,	// CU Resume
	COMMAND_CUC_LD_DUMP_ADDR	= 0x0040,	// Load Dump Counters Address
	COMMAND_CUC_DUMP_STATS		= 0x0050,	// Dump Statistical Counters
	COMMAND_CUC_LOAD_CU_BASE	= 0x0060,	// Load CU Base
	COMMAND_CUC_DUMP_N_RESET	= 0x0070,	// Dump Counters and Reset to 0
	COMMAND_CUC_STATIC_RESUME	= 0x00A0,	// CU Static Resume
/* RU Commands */
	COMMAND_RUC_NOP				= 0x0000,	// NOP - no effect
	COMMAND_RUC_RU_START		= 0x0001,	// RU Start
	COMMAND_RUC_RU_RESUME		= 0x0002,	// RU Resume
	COMMAND_RUC_RU_RCV_DMA_REDIR= 0x0003,	// RCV DMA Redirect
	COMMAND_RUC_RU_ABORT		= 0x0004,	// RU Abort
	COMMAND_RUC_RU_LOAD_HDS		= 0x0005,	// Load Header Data Size
	COMMAND_RUC_LOAD_RU_BASE	= 0x0006,	// Load RU Base
	COMMAND_RUC_RBD_RESUME		= 0x0007,	// RBD Resume
};

#endif // I8255X_REGS_H
