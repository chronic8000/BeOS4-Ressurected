/* ========================================================================== */
#include "mydefs.h"
/* ========================================================================== */
/* = local function which caches the pci module handle                      = */
/* ========================================================================== */
static pci_module_info *get_pci_module()
{
	static pci_module_info *cache = NULL;
	
	if (cache == NULL)
	{
		if (B_OK != get_module(B_PCI_MODULE_NAME, (module_info **) &cache))
		{
			PRINTF("cannot get pci module\n");
			return NULL;
		}
	}

	return cache;	
}
/* ========================================================================== */
/* = search for a card with (vendor, device) on the PCI bus. If found, copy = */
/* = the information in the location designed by the pointer and return the = */
/* = pointer. If the card is not present, return NULL.                      = */
/* ========================================================================== */
pci_info *beia_tm_pci_find_device
	(
	uint32    vendor,
	uint32    device,
	pci_info *info_parameter
	)
{
	pci_module_info *pci = get_pci_module();
	pci_info 	info;
	int 		index;
				
	for
	(
		index = 0;
		B_OK == pci->get_nth_pci_info(index, &info);
		index++
	)
	{
		if ((info.vendor_id == vendor)&&(info.device_id == device))
		{
			*info_parameter = info;
			return info_parameter;
		}		
	}
	
	return NULL;
}
/* ========================================================================== */
/* = read a PCI configuration dword                                         = */
/* ========================================================================== */
void beia_tm_pci_read_config_dword
	(
	pci_info *info,
	uint8     offset,
	uint32   *value
	)
{
	pci_module_info *pci = get_pci_module();

    *value = pci->read_pci_config(info->bus,
                		          info->device,
                		          info->function,
                		          offset, 
                		          4);
}
/* ========================================================================== */
/* = write a PCI configuration dword                                        = */
/* ========================================================================== */
void beia_tm_pci_write_config_dword
	(
	pci_info *info,
	uint8     offset,
	uint32    value
	)
{
	pci_module_info *pci = get_pci_module();

    pci->write_pci_config(info->bus,
                		  info->device,
                		  info->function,
                		  offset, 
                		  4,
                		  value);
}
/* ========================================================================== */
