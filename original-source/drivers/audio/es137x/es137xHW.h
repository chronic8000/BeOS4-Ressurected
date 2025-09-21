
#ifndef _HARDWARE_H_
#define _HARDWARE_H_

/*****************************************************************************
* ENSONIQ ES1370 SPECIFICATIONS
*****************************************************************************/

/*****************************************************************************
* Interrupt/Chip Select(ICS)
*/
#define ICS_CONTROL_REGISTER        0x00
#define ICS_STATUS_REGISTER         0x04

#define B_ICS_CONTROL_ADC_STOP      0x80000000
#define B_ICS_CONTROL_XCTL1         0x40000000
#define B_ICS_CONTROL_NOTUSED_29    0x20000000
#define B_ICS_CONTROL_PCLKDIV       0x1FFF0000
#define B_ICS_CONTROL_MSFMTSEL      0x00008000
#define B_ICS_CONTROL_MSBB          0x00004000
#define B_ICS_CONTROL_WTSRSEL       0x00003000
#define B_ICS_CONTROL_DAC_SYNC      0x00000800
#define B_ICS_CONTROL_CCB_INTRM     0x00000400
#define B_ICS_CONTROL_M_CB          0x00000200
#define B_ICS_CONTROL_XCTL0         0x00000100
#define B_ICS_CONTROL_BREQ          0x00000080
#define B_ICS_CONTROL_DAC1_EN       0x00000040
#define B_ICS_CONTROL_DAC2_EN       0x00000020
#define B_ICS_CONTROL_ADC_EN        0x00000010
#define B_ICS_CONTROL_UART_EN       0x00000008
#define B_ICS_CONTROL_JYSTK_EN      0x00000004
#define B_ICS_CONTROL_CDC_EN        0x00000002
#define B_ICS_CONTROL_SERR_DISABLE  0x00000001

#define B_ICS_STATUS_INTR           0x80000000
#define B_ICS_STATUS_NOTUSED_3011   0x7FFFF800
#define B_ICS_STATUS_CSTAT          0x00000400
#define B_ICS_STATUS_CBUSY          0x00000200
#define B_ICS_STATUS_CWRIP          0x00000100
#define B_ICS_STATUS_NOTUSED_07     0x00000080
#define B_ICS_STATUS_VC             0x00000060
#define B_ICS_STATUS_MCCB           0x00000010
#define B_ICS_STATUS_UART           0x00000008
#define B_ICS_STATUS_DAC1           0x00000004
#define B_ICS_STATUS_DAC2           0x00000002
#define B_ICS_STATUS_ADC            0x00000001

#define MASTER_CLOCK_FREQUENCY      22579200
#define MIN_SAMPLING_RATE           5000
#define MAX_SAMPLING_RATE           48000
#define MIN_BITS_PER_SAMPLE         8
#define MAX_BITS_PER_SAMPLE         16
#define MIN_CHANNELS                1
#define MAX_CHANNELS                2

/*****************************************************************************
* UART
*/
#define UART_DATA_REGISTER              0x08
#define UART_STATUS_REGISTER            0x09
#define UART_CONTROL_REGISTER           0x09
#define UART_RESERVED_REGISTER          0x0A

#define B_UART_DATA_DATA                0x00FF

#define B_UART_STATUS_RXINT             0x80
#define B_UART_STATUS_NOTUSED_1411      0x78
#define B_UART_STATUS_TXINT             0x04
#define B_UART_STATUS_TXRDY             0x02
#define B_UART_STATUS_RXRDY             0x01

#define B_UART_CONTROL_RXINTEN          0x80
#define B_UART_CONTROL_TXINTEN          0x60
#define B_UART_CONTROL_TXRDYINTEN       0x20
#define B_UART_CONTROL_NOTUSED_1210     0x1C
#define B_UART_CONTROL_CNTRL            0x03

#define B_UART_RESERVERD_NOTUSED_0711   0xFE
#define B_UART_RESERVED_TESTMODE        0x01

/*****************************************************************************
* Host Interface - Memory Page
*/
#define MEMORY_PAGE_REGISTER            0x0C

#define B_MEMORY_PAGE_NOTUSED_3104      0xFFFFFFF0
#define B_MEMORY_PAGE_MEMORY_PAGE       0x00000003

/*****************************************************************************
* CODEC Write Register
*/
#define CODEC_WRITE_REGISTER            0x10

#define B_CODEC_WRITE_NOTUSED_3116      0xFFFF0000
#define B_CODEC_WRITE_ADDRESS           0x0000FF00
#define B_CODEC_WRITE_DATA              0x000000FF

/*****************************************************************************
* Serial Interface
*/
#define SERIAL_INTF_CONTROL_REGISTER            0x20
#define DAC1_CHANNEL_SAMPLE_COUNT_REGISTER      0x24
#define DAC2_CHANNEL_SAMPLE_COUNT_REGISTER      0x28
#define ADC_CHANNEL_SAMPLE_COUNT_REGISTER       0x2C

#define B_SERIAL_INTF_CONTROL_NOTUSED_3122      0xFFC00000
#define B_SERIAL_INTF_CONTROL_P2_END_INC        0x00380000
#define B_SERIAL_INTF_CONTROL_P2_ST_INC         0x00070000
#define B_SERIAL_INTF_CONTROL_R1_LOOP_SEL       0x00008000
#define B_SERIAL_INTF_CONTROL_P2_LOOP_SEL       0x00004000
#define B_SERIAL_INTF_CONTROL_P1_LOOP_SEL       0x00002000
#define B_SERIAL_INTF_CONTROL_P2_PAUSE          0x00001000
#define B_SERIAL_INTF_CONTROL_P1_PAUSE          0x00000800
#define B_SERIAL_INTF_CONTROL_R1_INT_EN         0x00000400
#define B_SERIAL_INTF_CONTROL_P2_INTR_EN        0x00000200
#define B_SERIAL_INTF_CONTROL_P1_INTR_EN        0x00000100
#define B_SERIAL_INTF_CONTROL_P1_SCT_RLD        0x00000080
#define B_SERIAL_INTF_CONTROL_P2_DAC_SEN        0x00000040
#define B_SERIAL_INTF_CONTROL_R1_S_EB           0x00000020
#define B_SERIAL_INTF_CONTROL_R1_S_MB           0x00000010
#define B_SERIAL_INTF_CONTROL_P2_S_EB           0x00000008
#define B_SERIAL_INTF_CONTROL_P2_S_MB           0x00000004
#define B_SERIAL_INTF_CONTROL_P1_S_EB           0x00000002
#define B_SERIAL_INTF_CONTROL_P1_S_MB           0x00000001

#define B_STEREO        0x01
#define B_SIXTEEN_BIT   0x02

#define B_DAC1_CHANNEL_SAMPLE_COUNT_CURR_SAMP_CT    0xFFFF0000
#define B_DAC1_CHANNEL_SAMPLE_COUNT_SAMP_CT         0x0000FFFF

#define B_DAC2_CHANNEL_SAMPLE_COUNT_CURR_SAMP_CT    0xFFFF0000
#define B_DAC2_CHANNEL_SAMPLE_COUNT_SAMP_CT         0x0000FFFF

#define B_ADC_CHANNEL_SAMPLE_COUNT_CURR_SAMP_CT     0xFFFF0000
#define B_ADC_CHANNEL_SAMPLE_COUNT_SAMP_CT          0x0000FFFF

/*****************************************************************************
* Host Interface - Memory
*/
#define DAC1_FRAME_REGISTER_1       0x30
#define DAC1_FRAME_REGISTER_2       0x34
#define DAC2_FRAME_REGISTER_1       0x38
#define DAC2_FRAME_REGISTER_2       0x3C
#define ADC_FRAME_REGISTER_1        0x30
#define ADC_FRAME_REGISTER_2        0x34
#define UART_FIFO_REGISTER          0x30

#define B_DAC1_FRAME_PCI_ADDRESS    0xFFFFFFFF
#define B_DAC1_FRAME_CURRENT_COUNT  0xFFFF0000
#define B_DAC1_FRAME_BUFFER_SIZE    0x0000FFFF

#define B_DAC2_FRAME_PCI_ADDRESS    0xFFFFFFFF
#define B_DAC2_FRAME_CURRENT_COUNT  0xFFFF0000
#define B_DAC2_FRAME_BUFFER_SIZE    0x0000FFFF

#define B_ADC_FRAME_PCI_ADDRESS     0xFFFFFFFF
#define B_ADC_FRAME_CURRENT_COUNT   0xFFFF0000
#define B_ADC_FRAME_BUFFER_SIZE     0x0000FFFF

#define B_UART_FIFO_NOTUSED_3109    0xFFFFFE00
#define B_UART_FIFO_BYTE_VALID      0x00000100
#define B_UART_FIFO_BYTE            0x000000FF


#endif // _HARDWARE_H_
