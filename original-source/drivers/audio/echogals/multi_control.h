// multi_control.h

status_t multiGetDescription(PMONKEYINFO pMI,multi_description *pMD);
status_t multiBufferExchange(PMONKEYINFO pMI, multi_buffer_info *pMBI);
status_t multiGetEnabledChannels(PMONKEYINFO pMI,multi_channel_enable *pMCE);
status_t multiSetEnabledChannels(PMONKEYINFO pMI,multi_channel_enable *pMCE);
status_t multiGetGlobalFormat(PMONKEYINFO pMI,multi_format_info * pMFI);
status_t multiSetGlobalFormat(PMONKEYINFO pMI,multi_format_info * pMFI);
status_t multiGetBuffers(PMONKEYINFO pMI,multi_buffer_list * pMBL);
status_t multiBufferForceStop(PMONKEYINFO pMI);
status_t multiListExtensions(PMONKEYINFO pMI,multi_extension_list * pMEL);
status_t echoGetAvailableSync(PMONKEYINFO pMI, DWORD *pAvailable);
