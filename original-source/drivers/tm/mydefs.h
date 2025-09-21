/* ========================================================================== */
#ifndef __INCLUDE_mydefs_h
#define __INCLUDE_mydefs_h 1
/* ========================================================================== */
#include <KernelExport.h>
#include <PCI.h>
/* ========================================================================== */
#define PRINTF			dprintf("tm_00168: "); dprintf
/* ========================================================================== */
/* = implementation of functions to access the pci bus information with     = */
/* = similar semantics that their linux counterparts (goal = minimizing     = */
/* = modification of driver source files).                                  = */
/* ========================================================================== */
/* = search for a card with (vendor, device) on the PCI bus. If found, copy = */
/* = the information in the location designed by the pointer and return the = */
/* = pointer. If the card is not present, return NULL.                      = */
/* ========================================================================== */
pci_info *beia_tm_pci_find_device
	(
	uint32    vendor,
	uint32    device,
	pci_info *info
	);
/* ========================================================================== */
/* = read a PCI configuration dword                                         = */
/* ========================================================================== */
void beia_tm_pci_read_config_dword
	(
	pci_info *info,
	uint8     offset,
	uint32   *value
	);
/* ========================================================================== */
/* = write a PCI configuration dword                                        = */
/* ========================================================================== */
void beia_tm_pci_write_config_dword
	(
	pci_info *info,
	uint8     offset,
	uint32    value
	);
/* ========================================================================== */
#endif
/* ========================================================================== */

