// util.h

status_t SendVector( PMONKEYINFO pMI, DWORD dwCommand);
status_t VMONK_Read_DSP( PMONKEYINFO pMI, DWORD *pdwData);
status_t VMONK_Write_DSP( PMONKEYINFO pMI, DWORD dwData );
void reset_card(PMONKEYINFO pMI);
void reset_buffer_exchange_state(PMONKEYINFO pMI);
DWORD get_physical_address(void *pLogical,DWORD dwSize);
void setup_input_gain(PMONKEYINFO pMI);
int32 decibel_to_linear(int8 dB);
int8 linear_to_decibel(float lin);
PMA_CLIENT add_ma_client(PMONKEYINFO pMI,uint32 flags);
void remove_ma_client(PMONKEYINFO pMI,PMA_CLIENT pClient);
PMIDI_CLIENT add_midi_client(PMONKEYINFO pMI);
void remove_midi_client(PMONKEYINFO pMI,PMIDI_CLIENT pClient);
size_t write_midi_bytes(PMONKEYINFO pMI, uint8 const *buffer,size_t bytes_requested);

