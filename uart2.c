/*
 * Using UART2 module
 *
 * Tested on:
 * Microstick + Microstick Plus Development Board
 * PIC24HJ64GP502
 *
 * File name: UART2.c
 * Author:    Jozsef KOPJAK
 * Info:      info@microstickplus.com
 * Web:       http://www.microstickplus.com
 *
 * Last modification: 10-04-2013
 *
 * bit banging uart:
 * http://www.8052.com/faqs.phtml?FAQ=124758
 * http://supp.iar.com/Support/?note=88469
 *  Async serial in AN510 and AN555
 * http://electronics.stackexchange.com/questions/69347/pic-interrupt-based-soft-uart-timing-trouble
 *
 */
//;*********************************************************************************************************
//;                       Software Implementation Of RS232 Communications Using PIC16CXX
//;                                              Half-Duplex
//;
//;  These routines are intended to be used with PIC16C6X/7X family. These routines can be
//;  used with processors in the 16C6X/7X family which do not have on board Hardware Async
//;  Serial Port.
//;  MX..
//;
//;  Description :
//;               Half Duplex RS-232 Mode Is implemented in Software.
//;               Both Reception & Transmission are Interrupt driven
//;               Only 1 peripheral (RTCC) used for both transmission & reception
//;               RTCC is used for both timing generation (for bit transmission & bit polling)
//;               and Start Bit Detection in reception mode.
//;               This is explained in more detail in the Interrupt Subroutine.
//;               Programmable Baud Rate (speed depnding on Input Clock Freq.), programmable
//;               #of bits, Parity enable/disable, odd/even parity is implemented.
//;                       Parity & Framing errors are detected on Reception
//;
//;                               RS-232 Parameters
//;
//;The RS-232 Parameters are defined as shown below:
//;
//;               SYS_FREQ          :       Input Clock Frequency of the processor
//;                                       (NOTE : RC Clock Mode Is Not Suggested due to wide variations)
//;               BAUD_RATE_UART2       :       Desired Baud Rate. Any valid value can be used.
//;                                       The highest Baud Rate achievable depends on Input Clock Freq.
//;                                       300 to 4800 Baud was tested using 4 Mhz Input Clock
//;                                       300 to 19200 Baud was tested using 10 Mhz Input Clock
//;                                       Higher rates can be obtained using higher Input Clock Frequencies.
//;                                       Once the BAUD_RATE_UART2 & SYS_FREQ are specified the program
//;                                       automatically selectes all the appropiate timings
//;               _DataBits       :       Can specify 1 to 8 Bits.
//;               _StopBits       :       Limited to 1 Stop Bit. Must set it to 1.
//;               _PARITY_ENABLE  :       Parity Enable Flag. Set it to TRUE or FALSE. If PARITY
//;                                       is used, then set it to TRUE, else FALSE. See "_ODD_PARITY" flag
//;                                       description below
//;               _ODD_PARITY     :       Set it to TRUE or FALSE. If TRUE, then ODD PARITY is used, else
//;                                       EVEN Parity Scheme is used.
//;                                       This Flag is ignored if _PARITY_ENABLE is set to FALSE.
//;
//;
//;  Usage :
//;               An example is given in the main program on how to Receive & Transmit Data
//;               In the example, the processor waits until a command is received. The command is interpreted
//;               as the A/D Channel Number of PIC16C71. Upon reception of a command, the desired A/D channel
//;               is selected and after A/D conversion, the 8 Bit A/D data is transmitted back to the Host.
//;
//;                       The RS-232 Control/Status Reg's bits are explained below :
//;
//;       "SerialStatus"          : RS-232 Status/Control Register
//;
//;       Bit 0   :       _txmtProgress   (1 if transmission in progress, 0 if transmission is complete)
//;                                       After a byte is transmitted by calling "PutChar" function, the
//;                                       user's code can poll this bit to check if transmission is complete.
//;                                       This bit is reset after the STOP bit has been transmitted
//;       Bit 1   :       _txmtEnable     Set this bit to 1 on initialization to enable transmission.
//;                                       This bit can be used to Abort a transmission while the transmitter
//;                                       is in progress (i.e when _txmtProgress = 1)
//;       Bit 2   :       _rcvProgress    Indicates that the receiver is in middle of reception.It is reset when
//;                                       a byte is received.
//;       Bit 3   :       _rcvOver        This bit indicates the completion of Reception of a Byte. The user's
//;                                       code can poll this bit after calling "GetChar" function. Once "GetChar"
//;                                       function is called, this bit is 1 and is set to 0 after reception of
//;                                       a complete byte (parity bit if enabled & stop bit)
//;       Bit 4   :       _ParityErr      A 1 indicates Parity Error on Reception (for both even & odd parity)
//;       Bit 5   :       _FrameErr       A 1 indicates Framing Error On Reception
//;
//;       Bit 6   :       _unused_        Unimplemented Bit
//;
//;       Bit 7   :       _parityBit      The 9 th bit of transmission or reception (status of PARITY bit
//;                                       if parity is enabled)
//;
//;       To Transmit A Byte Of Data :
//;                       1) Make sure _txmtProgress & _rcvOver bits are cleared
//;                       2) Load TxReg with data to be transmitted
//;                       3) CALL  PutChar function
//;
//;       To Receive A Byte Of Data :
//;                       1) Make sure _txmtProgress & _rcvOver bits are cleared
//;                       2) CALL GetChar function
//;                       3) The received Byte is in TxReg after _rcvOver bit is cleared
//;
//;
//;
//;       Program:          RS232.ASM
//;       Revision Date:
//;	                  May 17,1994 Scott Fink    (Rev 2)
//;	                        	Corrected 7 bit and parity operation, corrected stop bit generation, corrected
//;                       		receive prescaler settings.  Protected against inadvertant WDT reset.
//;                         1-16-97      Compatibility with MPASMWIN 1.40
//;
//****************************************************************************************************
//;

/* Device header file */
#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#endif

#include "system.h"
#include "uart2.h"
#include "rb.h"
#include "user.h"

/******************************************************************************/
/* Variables definition                                                       */
/******************************************************************************/
#define       MAX_BUFLEN2 64
static char outbuf2[MAX_BUFLEN2]; /* memory for ring buffer #1 (TXD) */
static char inbuf2 [MAX_BUFLEN2]; /* memory for ring buffer #2 (RXD) */

///* define o/p and i/p ring buffer control structures */
static RB_CREATE(out2, char); /* static struct { ... } out; */
static RB_CREATE(in2, char); /* static struct { ... } in; */

static volatile char TxReg; //			; Transmit Data Holding/Shift Reg
static volatile char RxReg; // Rcv Data Holding Reg
static volatile char RxTemp;
static volatile char BitCount;
static volatile char ExtraBitCount; // Parity & Stop Bit Count

char BufferPcRcv;

/******************************************************************************/
/* Interrupt Routines                                                         */
/******************************************************************************/
//interrupt handling
//Transmit and Receive routines share the same TMR0
//when in RX mode, all TX routines have less priority
void _U2Interrupt() {
//    INTCONbits.T0IF = 0;
    if (SerialStatusbits._txmtProgress)
        _TxmtNextBit(); // Txmt Next Bit
    else if (SerialStatusbits._rcvProgress)
        _RcvNextBit(); // Receive Next Bit
    else
        _SBitDetected(); // Must be start Bit
}

/*---------------------------------------------------------------------
  Function name: vInitU2
  Description: Initialization of UART2 module
  Input parameters: none
  Output parameters: none
-----------------------------------------------------------------------*/
void vInitU2() {

    RB_INIT(&out2, outbuf2, MAX_BUFLEN2-1);//15 /* configura TX ring buffer */
    RB_INIT(&in2, inbuf2, MAX_BUFLEN2-1); //15/* configura RX ring buffer */

    SerialStatusbits.SerialStatus = 0;
    BufferPcRcv = 0; //informa que não há novo byte, evento

    ADCON1 = USART2_ADCON; //disable analog inputs, change it to ttl
    CMCON = USART2_CMCON; //configure coparators for digital input
    USART2_TX = 1; //make sure TX Pin is high on powerup, use RB Port Pullup
    USART2_TX_TRIS = 0; //set TX Pin As Output Pin, by modifying TRIS
    USART2_RX_TRIS = 1; //set RX Pin As Input for reception
    USART2_RESET = 1;
    USART2_RESET_TRIS = 0;
#if (_USE_RTSCTS)
    USART2_RTS = 1;
    USART2_RTS_TRIS = 0; //RTS is output signal, controlled by PIC16Cxx
    USART2_CTS_TRIS = 1; //CTS is Input signal, controlled by the host
#endif
    _RcvEnable(); //enable int TMR0, and RX flags
}

/*---------------------------------------------------------------------
  Function name: ser2Outchar
  Description: Sends buffer characters to UART2 (use of blocking wait)
  Input parameters: Character to send
  Output parameters: -

 * send character to the buffer
 * if txmt is not in progress, start another tx session
-----------------------------------------------------------------------*/
int ser2Outchar(char c) {
#ifndef depuracao_serial_bitbang
    while (RB_FULL(&out2)) CLRWDT(); // wait until there's space

    SerialStatusbits._txmtEnable = 1; // enable transmission
    *RB_PUSHSLOT(&out2) = c; /* store data in the buffer */
    RB_PUSHADVANCE(&out2); /* adjust write position */

    if (!SerialStatusbits._txmtProgress) {
        while (SerialStatusbits._rcvOver) { //wait until reception end
            USART2_RTS = 1; //send wait to host
            _delaywdt(_CyclesPerBit >> 1); //wait for 2 bits
            if (!SerialStatusbits._rcvProgress)//if not in rx mode
                SerialStatusbits._rcvOver = 0; //prepare to tx
        }
        TxReg = *RB_POPSLOT(&out2); /* start transmission of next byte */
        RB_POPADVANCE(&out2); /* remove the sent byte from buffer */
        _TxmtStartBit();
        INTCONbits.T0IE = 1; // Enable RTCC Overflow INT
    }
#endif
    return 0;
}

/*---------------------------------------------------------------------
  Function name: ser2Outstring
  Description: Sends string from buf to UART2
  Input parameters: String to send
  Output parameters: -
-----------------------------------------------------------------------*/
void ser2Outstring(char *buf, int len) {
    char *ponteiro;
    //ponteiro modificável, para uma variável que não deve ser modificada

//    int comprimento;
//    for (comprimento = 0; comprimento < len; comprimento++){
//        ser2Outchar(buf[comprimento]);
//    }

//    ocorrido a falha de acesso a RAM em blocos diferentes.
//            atualização de XC8 v1.20 para v1.33b
//            o ponteiro é atualizado com mudança de BSR
    ponteiro = buf;
    while (ponteiro < &buf[len]){
        ser2Outchar(*(ponteiro++));
    }
}

/*---------------------------------------------------------------------
  Function name: _TxmtStartBit
  Description: put start bit over TX line, and enable TMR0 to next bits.
 entered from interrupt service routine and ser2Outchar
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void _TxmtStartBit() {
    SerialStatusbits._txmtProgress = 1;
    LOAD_BITCOUNT(BitCount, ExtraBitCount); // Macro to load bit count
    BitCount--;
#if(_DataBits == 7)
    TxReg |= 0x80;
#endif
#if(_PARITY_ENABLE)
    GenParity(TxReg); // If Parity is used, then Generate Parity Bit
#endif
    TMR0L = 0;
    CLRWDT();
    T0CON = 0xC7; //incremento com RA4, na subida
    TMR0L = 0;
    CLRWDT();
    T0CON = 0xCF; //para a contagem no tmr0, passa para wdt
    CLRWDT();
    T0CON = _OPTION_INIT | RtccPrescale; //reinicia contagem
    USART2_TX = 0; //Send Start Bit
    TMR0L = -RtccPreLoad; // Prepare for Timing Interrupt
    INTCONbits.T0IF = 0;
}

/*---------------------------------------------------------------------
  Function name: _TxmtNextBit
  Description: put output word bit over TX line, and enable TMR0 to next bits.
 * could start another frame, if buffer_out is not empty, or change to RX mode.
 entered from interrupt service routine
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void _TxmtNextBit() {
    LOAD_RTCC_ZERO(RtccPreLoad, RtccPrescale); //Macro to reload RTCC
    if (BitCount) {//;done with data xmission?
        BitCount--; //;no, send another
        //_NextTxmtBit:
        if (TxReg & 0x01)
            USART2_TX = 1;
        else
            USART2_TX = 0;
        TxReg >>= 1;
        if (!SerialStatusbits._txmtEnable) {
            INTCONbits.T0IE = 0; //; disable further interrupts, Transmission Aborted
            SerialStatusbits._txmtProgress = 0; //;indicates end of xmission
            USART2_TX = 1;
            _RcvEnable(); //enable reception
        }
    } else {//;yes, do parity or stop bit
#if(_PARITY_ENABLE)
        //_ParityOrStop:
        if (ExtraBitCount & 0x02) {//;ready for parity bit?
            //_SendParity:
            ExtraBitCount--; //;subtract parity from count
            if (SerialStatusbits._parityBit)
                USART2_TX = 1;
            else
                USART2_TX = 0;
        } else {
            if (ExtraBitCount) {//;check if sending stop bit
                ExtraBitCount--;
                //_StopBit:
                USART2_TX = 1; //; STOP Bit is High
            } else {
                //DoneTxmt
                USART2_TX = 1; //;STOP Bit is High
                if (!RB_EMPTY(&out2)) { //verify buffer
                    TxReg = *RB_POPSLOT(&out2); /* start transmission of next byte */
                    RB_POPADVANCE(&out2); /* remove the sent byte from buffer */
                    _TxmtStartBit();
                } else {
                    SerialStatusbits._txmtProgress = 0; //;indicates end of xmission
                    SerialStatusbits._txmtEnable = 0;
                    _RcvEnable(); //enable reception
                }
            }
        }
#else
        if (ExtraBitCount) {//;check if sending stop bit
            ExtraBitCount--;
            //_StopBit:
            USART2_TX = 1; //; STOP Bit is High
        } else {
            //DoneTxmt
            USART2_TX = 1; //;STOP Bit is High
            if (!RB_EMPTY(&out2)) { //verify buffer
                TxReg = *RB_POPSLOT(&out2); /* start transmission of next byte */
                RB_POPADVANCE(&out2); /* remove the sent byte from buffer */
                _TxmtStartBit();
            } else {
                SerialStatusbits._txmtProgress = 0; //;indicates end of xmission
                SerialStatusbits._txmtEnable = 0;
                _RcvEnable(); //enable reception
            }
        }
#endif
    }
}

/*---------------------------------------------------------------------
  Function name: GenParity
  Description: generate parity
  Input parameters: byte under test (not destroyed)
  Output parameters: _parityBit flag
-----------------------------------------------------------------------*/
void GenParity(char byte) {
#if _PARITY_ENABLE
    char temp1;
    for (temp1 = _DataBits; temp1 > 0; temp1--) {
        if (byte & 0x01)
            byte = (byte >> 1) ^ 0x00; //;parity calculated by XORing all data bits
        else
            byte = (byte >> 1) ^ 0x01; //; Parity bit is in Bit 0 of temp1
    }
#if (_ODD_PARITY)
    SerialStatusbits._parityBit = 1;
    if (byte & 0x01)
        SerialStatusbits._parityBit = 0;
#else
    SerialStatusbits._parityBit = 0;
    if (byte & 0x01)
        SerialStatusbits._parityBit = 1;
#endif
#endif
}

/*---------------------------------------------------------------------
  Function name: ser2Inchar
  Description: Receives character from UART2 (use of blocking wait)
 *  when a transmission request is detected, the incoming bytes are blocked
  Input parameters: -
  Output parameters: Received character
-----------------------------------------------------------------------*/
void PCTrigger1() {
    CLRWDT();
};

char ser2Inchar() {
    char c;
    BufferPcRcv = 0;
#ifdef depuracao_serial_bitbang
    /*ponto de inserção da mensagem de teste*/
    NOP();
    c = ADRESL;
    BufferPcRcv = ADRESH;
#else
    if (!SerialStatusbits._rcvOver & !SerialStatusbits._txmtEnable)//enable RX, after recovery
        _RcvEnable();
    if (RB_EMPTY(&in2)) {
        return 0;
        //        PCTrigger1();
    };

    c = *RB_POPSLOT(&in2); /* get character off the buffer */
    RB_POPADVANCE(&in2); /* adjust write position */
    //BufferPcRcv indicador de byte recebido
        BufferPcRcv = 1;
#endif
    return c;
}

/*---------------------------------------------------------------------
  Function name: _RcvEnable
  Description: enable data reception. The incoming start bit is hardware(tmr0)
 * monitored
  Input parameters: byte under test (not destroyed)
  Output parameters: _parityBit flag
-----------------------------------------------------------------------*/
void _RcvEnable() {
#if (_USE_RTSCTS)
    USART2_RTS = 0; // ; ready to accept data from host
#endif
    SerialStatusbits._rcvOver = 1; //; Enable Reception, this bit gets reset on Byte Rcv Complete

    LOAD_BITCOUNT(BitCount, ExtraBitCount);
    RxReg = 0;
    SerialStatusbits._FrameErr = 0;
    SerialStatusbits._ParityErr = 0; //; Init Parity & Framing Errors
    TMR0L = 0;
    CLRWDT();
    T0CON = 0xC7;
    TMR0L = 0;
    T0CON = 0xCF;
    CLRWDT();
    T0CON = _OPTION_SBIT; //; Inc On Ext Clk Falling Edge
    TMR0L = 0xFF; //; A Start Bit will roll over RTCC & Gen INT
    INTCONbits.T0IF = 0;
    INTCONbits.T0IE = 1;
}

/*---------------------------------------------------------------------
  Function name: _SBitDetected
  Description: start bit detection. The incoming start bit is hardware(tmr0)
 * monitored
 * entered from interrupt service routine
 * _rcvProgress flag is set when a start bit is detected. Tmr0 is set to 1 1/2 bit
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void _SBitDetected() {
    if (!USART2_RX) {//; Make sure Start Bit Interrupt is not a Glitch
        SerialStatusbits._rcvProgress = 1;
        TMR0L = 0;
        CLRWDT();
        T0CON = 0xC7;
        TMR0L = 0;
        T0CON = 0xCF;
        CLRWDT();
        T0CON = (_BIT1_INIT | SBitPrescale); //; Switch Back to INT Clock
        LOAD_RTCC_ONE(SBitRtccLoad, SBitPrescale);
    } else
        //_FalseStartBit:
        TMR0L = 0xFF; //; reload RTCC with 0xFF for start bit detection
}

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
void _RcvNextBit() {
    CLRWDT();
    T0CON = 0xC7;
    TMR0L = 0;
    CLRWDT();
    T0CON = 0xC7;
    TMR0L = 0;
    T0CON = 0xCF;
    CLRWDT();
    T0CON = _OPTION_INIT | RtccPrescale; //; Switch Back to INT Clock
    RxTemp = PORTA; //; read RX pin immediately into WREG
    LOAD_RTCC_ZERO(RtccPreLoad, RtccPrescale); //Macro to reload RTCC
    if ((PORTA ^ RxTemp) & RX_MASK)//; mask for only RX PIN (RA4)
        //_SampleAgain:
        RxTemp = PORTA; //; 2 out of 3 majority sampling done
    //_PinSampled
    //; both samples are same state
    if (BitCount)
        BitCount--;
    if (BitCount == 0) {
        //_RcvP_Or_S:
#if (_PARITY_ENABLE)
        ExtraBitCount--;
        if (!ExtraBitCount) {
            //_RcvStopBit:
            SerialStatusbits._FrameErr = 1; //; may be framing Error or Glitch
            if (RX){
                SerialStatusbits._FrameErr = 0;
                GenParity(RxReg); //; Generate Parity, for Parity check
                SerialStatusbits._ParityErr ^= SerialStatusbits._parityBit;
                if(!SerialStatusbits._ParityErr){
#if(_DataBits == 7)
                    RxReg >>= 1;
                    RxReg &= 0x7F;
#endif
                    *RB_PUSHSLOT(&in2) = RxReg; /* store new data in the buffer */
                    RB_PUSHADVANCE(&in2); /* next write location */
                }
            }
            RxReg = 0;
            SerialStatusbits._rcvProgress = 0;
            SerialStatusbits._rcvOver = 0; //; Byte Received, Can RCV/TXMT an other Byte
            INTCONbits.T0IE = 0; // disable RTCC Overflow INT
            if (!SerialStatusbits._txmtEnable & !RB_FULL(&in2))//não recebe se buffer cheio, ou há sol de tx
                _RcvEnable();
        } else {
            //_RcvParity:
            SerialStatusbits._ParityErr = 0; //; Temporarily store PARITY Bit in _ParityErr
            if (RX) //; Sample again to avoid any glitches
                SerialStatusbits._ParityErr = 1;
        }
#else
        //_RcvStopBit:
        SerialStatusbits._FrameErr = 1; //; may be framing Error or Glitch
        if (RX) {
            SerialStatusbits._FrameErr = 0; //no framing error, load buffer
#if(_DataBits == 7)
            RxReg >>= 1;
            RxReg &= 0x7F;
#endif
                *RB_PUSHSLOT(&in2) = RxReg; /* store new data in the buffer */
                RB_PUSHADVANCE(&in2); /* next write location */
        }
        RxReg = 0;
        SerialStatusbits._rcvProgress = 0;
        SerialStatusbits._rcvOver = 0; //; Byte Received, Can RCV/TXMT an other Byte
        INTCONbits.T0IE = 0; // disable RTCC Overflow INT
        if (!SerialStatusbits._txmtEnable & !RB_FULL(&in2))//não recebe se buffer cheio, ou há sol de tx
            _RcvEnable();
#endif
    } else {
        //_NextRcvBit
        RxReg >>= 1; //; prepare bit for shift
        if (RX)
            RxReg |= 0x80; //; shift in received data
        if(BitCount == 1)//trava recepção no ultimo bit(latencia), antes de testar liberação
            USART2_RTS = 1;
    }
}
