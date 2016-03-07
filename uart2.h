/*
 * Using UART2 module
 *
 * Tested on:
 * Microstick + Microstick Plus Development Board
 * PIC24HJ64GP502
 *
 * File name: UART2.h
 * Author:    Jozsef KOPJAK
 * Info:      info@microstickplus.com
 * Web:       http://www.microstickplus.com
 *
 * Last modification: 10-04-2013
 */

#ifndef _UART2_H
#define _UART2_H

#if defined(__XC)
#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */
#include <string.h>
#endif

#include "typedefs.h"
#include "system.h"

/* Parameters of the UART2 module */
#define BAUD_RATE_UART2 9600L            // Baud Rate (bits per second) is 1200
#define _DataBits 8               // 8 bit data, can be 1 to 8
#define _StopBits 1               // 1 Stop Bit, 2 Stop Bits is not implemented

#define _PARITY_ENABLE  false           // NO Parity
#define _ODD_PARITY     false           // EVEN Parity, if Parity enabled
#define _USE_RTSCTS     true           // NO Hardware Handshaking is Used

#define _CyclesPerBit   (FCY/BAUD_RATE_UART2)
#define _tempCompute    (_CyclesPerBit >> 8)

// Register: SerialStatus
// bitfield definitions
typedef union {
        unsigned char SerialStatus;
    struct {
        unsigned _txmtProgress  :1;
        unsigned _txmtEnable    :1;
        unsigned _rcvProgress   :1;
        unsigned _rcvOver       :1;
        unsigned _ParityErr     :1;
        unsigned _FrameErr      :1;
        unsigned unused         :1;
        unsigned _parityBit     :1;
    };
} SerialStatusbits_t;

//;**************************************************************************************************
//;			Pin Assignements
//;**************************************************************************************************
#if (MOTOVER == RevA)
    #define USART2_RESET LATAbits.LATA1 //porta de Reset está diferente.
    #define USART2_RX PORTAbits.RA4
    #define USART2_TX LATAbits.LATA0
    #define USART2_CTS PORTAbits.RA2
    #define USART2_RTS LATAbits.LATA3
    #define USART2_RESET_TRIS TRISAbits.TRISA1
    #define USART2_RX_TRIS TRISAbits.TRISA4
    #define USART2_TX_TRIS TRISAbits.TRISA0
    #define USART2_CTS_TRIS TRISAbits.TRISA2
    #define USART2_RTS_TRIS TRISAbits.TRISA3
    #define USART2_ADCON 0x0F //a/d disabled
    #define USART2_CMCON 0X07 //disable all comparators
    #define RX_MASK 0x10		//; RX pin is connected to RA4, ie. bit 4
    #define	RX	RxTemp & 0x10
#elif (MOTOVER == RevB)
    #define USART2_RESET LATBbits.LATB4
    #define USART2_RX PORTAbits.RA4
    #define USART2_TX LATAbits.LATA0
    #define USART2_CTS PORTAbits.RA3 //trocado com RTS
    #define USART2_RTS LATAbits.LATA2
    #define USART2_RESET_TRIS TRISBbits.TRISB4
    #define USART2_RX_TRIS TRISAbits.TRISA4
    #define USART2_TX_TRIS TRISAbits.TRISA0
    #define USART2_CTS_TRIS TRISAbits.TRISA3 //trocado com RTS
    #define USART2_RTS_TRIS TRISAbits.TRISA2
    #define USART2_ADCON 0x0F //a/d disabled
    #define USART2_CMCON 0X07 //disable all comparators
    #define RX_MASK 0x10		//; RX pin is connected to RA4, ie. bit 4
    #define	RX	RxTemp & 0x10
#elif (MOTOVER == RevC)
    #define USART2_RESET LATBbits.LATB4
    #define USART2_RX PORTAbits.RA4
    #define USART2_TX LATAbits.LATA0
    #define USART2_CTS PORTAbits.RA2
    #define USART2_RTS LATAbits.LATA3
    #define USART2_RESET_TRIS TRISBbits.TRISB4
    #define USART2_RX_TRIS TRISAbits.TRISA4
    #define USART2_TX_TRIS TRISAbits.TRISA0
    #define USART2_CTS_TRIS TRISAbits.TRISA2
    #define USART2_RTS_TRIS TRISAbits.TRISA3
    #define USART2_ADCON 0x0F //a/d disabled
    #define USART2_CMCON 0X07 //disable all comparators
    #define RX_MASK 0x10		//; RX pin is connected to RA4, ie. bit 4
    #define	RX	RxTemp & 0x10
#else
    #error Revision number of Hardware not defined. Please define it in user.h file.
#endif


//;*****************************************************************************************
//;		Auto Generation Of Prescaler & Rtcc Values
//;   Computed during Assembly Time
//;*****************************************************************************************
//
//;  At first set Default values for RtccPrescale & RtccPreLoad
#if (_tempCompute >= 128)
    #define RtccPrescale	7
    #define RtccPreLoad	(_CyclesPerBit >> 8)
    #define UsePrescale	true
#elif (_tempCompute >= 64)
    #define RtccPrescale	6
    #define RtccPreLoad	(_CyclesPerBit >> 7)
    #define UsePrescale	true
#elif (_tempCompute >= 32)
    #define RtccPrescale	5
    #define RtccPreLoad	(_CyclesPerBit >> 6)
    #define UsePrescale	true
#elif (_tempCompute >= 16)
    #define RtccPrescale	4
    #define RtccPreLoad	(_CyclesPerBit >> 5)
    #define UsePrescale	true
#elif (_tempCompute >= 8)
    #define RtccPrescale	3
    #define RtccPreLoad	(_CyclesPerBit >> 4)
    #define UsePrescale	true
#elif (_tempCompute >= 4)
    #define RtccPrescale	2
    #define RtccPreLoad	(_CyclesPerBit >> 3)
    #define UsePrescale	true
#elif (_tempCompute >= 2)
    #define RtccPrescale	1
    #define RtccPreLoad	(_CyclesPerBit >> 2)
    #define UsePrescale	true
#elif (_tempCompute >= 1)
    #define RtccPrescale	0
    #define RtccPreLoad	(_CyclesPerBit >> 1)
    #define UsePrescale	true
#elif (_tempCompute == 0)
    #define RtccPrescale    0
    #define RtccPreLoad	_CyclesPerBit
    #define UsePrescale	false
#endif
#if( (RtccPrescale == 0) && (RtccPreLoad < 60))
    #error	"Warning : Baud Rate May Be Too High For This Input Clock"
#endif

//;
//; Compute RTCC & Prescaler Values For 1.5 Times the Baud Rate for Start Bit Detection
//;
#define _SBitCycles	(FCY/BAUD_RATE_UART2) + ((FCY/2)/BAUD_RATE_UART2)
#define _tempCompute1	(_SBitCycles >> 8)

#if   (_tempCompute1 >= 128)
    #define _BIT1_INIT          0xC0
    #define SBitPrescale	7
    #define SBitRtccLoad	(_SBitCycles >> 8)
#elif (_tempCompute1 >= 64)
    #define _BIT1_INIT          0xC0
    #define SBitPrescale	6
    #define SBitRtccLoad	(_SBitCycles >> 7)
#elif (_tempCompute1 >= 32)
    #define _BIT1_INIT          0xC0
    #define SBitPrescale	5
    #define SBitRtccLoad	(_SBitCycles >> 6)
#elif (_tempCompute1 >= 16)
    #define _BIT1_INIT          0xC0
    #define SBitPrescale	4
    #define SBitRtccLoad	(_SBitCycles >> 5)
#elif (_tempCompute1 >= 8)
    #define _BIT1_INIT          0xC0
    #define SBitPrescale	3
    #define SBitRtccLoad	(_SBitCycles >> 4)
#elif (_tempCompute1 >= 4)
    #define _BIT1_INIT          0xC0
    #define SBitPrescale	2
    #define SBitRtccLoad	(_SBitCycles >> 3)
#elif (_tempCompute1 >= 2)
    #define _BIT1_INIT          0xC0
    #define SBitPrescale	1
    #define SBitRtccLoad	(_SBitCycles >> 2)
#elif (_tempCompute1 >= 1)
    #define _BIT1_INIT          0xC0
    #define SBitPrescale	0
    #define SBitRtccLoad	(_SBitCycles >> 1)
#elif (_tempCompute1 == 0)
    #define _BIT1_INIT          0xC8
    #define SBitPrescale	0
    #define SBitRtccLoad	_SBitCycles
#endif

////;
////;*****************************************************************************************
//;
#define	_Cycle_Offset1	54	//;account for interrupt latency, call time

#if (UsePrescale)//Re Load RTCC init value + INT Latency Offset
                // Note that Prescaler is cleared when RTCC is written
    #define LOAD_RTCC_ZERO(K, Prescale)\
        TMR0L = -K + (_Cycle_Offset1 >> (Prescale+1));
    #define LOAD_RTCC_ONE(K, Prescale)\
        TMR0L = -K + (_Cycle_Offset1 >> (Prescale+1));
#else
    #define LOAD_RTCC_ZERO(K, Prescale)\
        TMR0L = -K + _Cycle_Offset1;
    #define LOAD_RTCC_ONE(K, Prescale)\
        TMR0L = -K + (_Cycle_Offset1 >> (Prescale+1));
#endif

#if (_PARITY_ENABLE)
    #define LOAD_BITCOUNT(BitCount, ExtraBitCount)\
    {BitCount = _DataBits+1;\
    ExtraBitCount = 2;\
    }
#else
    #define LOAD_BITCOUNT(BitCount, ExtraBitCount)\
    {BitCount = _DataBits+1;\
    ExtraBitCount = 1;\
    }
#endif

#define _OPTION_SBIT	0xF8		// Increment on Ext Clock (falling edge), for START Bit Detect

#if (UsePrescale)
    #define _OPTION_INIT	0xC0	// Prescaler is used depending on Input Clock & Baud Rate
                                     //8bits, tmr0=on
#else
    #define _OPTION_INIT	0xCF
#endif

volatile SerialStatusbits_t SerialStatusbits;
extern char BufferPcRcv;

/*---------------------------------------------------------------------
  Function name: vInitU2
  Description: Initialization of UART2 module
  Input parameters: none
  Output parameters: none
-----------------------------------------------------------------------*/
void vInitU2(void);

/*---------------------------------------------------------------------
  Function name: ser2Outchar
  Description: Sends buffer characters to UART2 (use of blocking wait)
  Input parameters: Character to send
  Output parameters: -

 * send character to the buffer
 * if txmt is not in progress, start another tx session
-----------------------------------------------------------------------*/
int ser2Outchar(char c);

/*---------------------------------------------------------------------
  Function name: ser2Outstring
  Description: Sends string from buf to UART2
  Input parameters: String to send
  Output parameters: -
-----------------------------------------------------------------------*/
void ser2Outstring(char *buf, int len);

/*---------------------------------------------------------------------
  Function name: _TxmtStartBit
  Description: put start bit over TX line, and enable TMR0 to next bits.
 entered from interrupt service routine and ser2Outchar
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void _TxmtStartBit(void);

/*---------------------------------------------------------------------
  Function name: _TxmtNextBit
  Description: put output word bit over TX line, and enable TMR0 to next bits.
 * could start another frame, if buffer_out is not empty, or change to RX mode.
 entered from interrupt service routine
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void _TxmtNextBit(void);

/*---------------------------------------------------------------------
  Function name: GenParity
  Description: generate parity
  Input parameters: byte under test (not destroyed)
  Output parameters: _parityBit flag
-----------------------------------------------------------------------*/
void GenParity(char byte);

/*---------------------------------------------------------------------
  Function name: ser2Inchar
  Description: Receives character from UART2 (use of blocking wait)
 *  when a transmission request is detected, the incoming bytes are blocked
  Input parameters: -
  Output parameters: Received character
-----------------------------------------------------------------------*/
char ser2Inchar(void);

/*---------------------------------------------------------------------
  Function name: _RcvEnable
  Description: enable data reception. The incoming start bit is hardware(tmr0)
 * monitored
  Input parameters: byte under test (not destroyed)
  Output parameters: _parityBit flag
-----------------------------------------------------------------------*/
void _RcvEnable(void);

/*---------------------------------------------------------------------
  Function name: _SBitDetected
  Description: start bit detection. The incoming start bit is hardware(tmr0)
 * monitored
 * entered from interrupt service routine
 * _rcvProgress flag is set when a start bit is detected. Tmr0 is set to 1 1/2 bit
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void _SBitDetected(void);

/*---------------------------------------------------------------------
  Function name: _SBitDetected
  Description: data bit handling
 * entered from interrupt service routine
 * _rcvProgress flag is clear after frame reception.
 * new reception is restarted if necessary (no TX request)
 * parity treatment is accomplished if requested.
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void _RcvNextBit(void);

#endif /* _UART2_H */

