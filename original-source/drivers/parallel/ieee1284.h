#define wait_ack_low(_x)	wait_for_ieee1284_status((_x), S_ACK, 0)
#define wait_ack_high(_x)	wait_for_ieee1284_status((_x), S_ACK, S_ACK)

status_t negotiate_ieee1284(par_port_info *d, int ext_byte);
status_t terminate_ieee1284(par_port_info *d);
status_t reset_ieee1284(par_port_info *d);
bool wait_for_ieee1284_status(par_port_info *d, const uint8 mask, const uint8 val);

