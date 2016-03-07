/*
 * Using UART1 module
 *
 * Tested on:
 * Microstick + Microstick Plus Development Board
 * PIC24HJ64GP502
 *
 * File name: uart1.c
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

/* Device header file */
#if defined(__XC)
    #include <xc.h>         /* XC8 General Include File */
#endif

#include "uart1.h"
#include "rb.h"
#include "system.h"
#include "user.h"

/******************************************************************************/
/* Variables definition                                                       */
/******************************************************************************/
#define       MAX_BUFLEN 64
static char   outbuf[MAX_BUFLEN];     /* memory for ring buffer #1 (TXD) */
static char   inbuf [MAX_BUFLEN];     /* memory for ring buffer #2 (RXD) */
static int    TXactive = 0;             /* transmission status flag (off) */

/* define o/p and i/p ring buffer control structures */
static RB_CREATE(out, char);            /* static struct { ... } out; */
static RB_CREATE(in,  char);            /* static struct { ... } in; */

char BufferMtRcv;
/******************************************************************************/
/* Interrupt Routines                                                         */
/******************************************************************************/
void _U1RXInterrupt(void) {
//    if (!RB_FULL(&in)) {
        *RB_PUSHSLOT(&in) = RCREG; /* store new data in the buffer */
        RB_PUSHADVANCE(&in); /* next write location */
//    } else {
//        *RB_PUSHSLOT(&in) = RCREG; /* store new data in the buffer */
//    }
}

void _U1TXInterrupt(void) {
    if (!RB_EMPTY(&out)) {
        TXREG = *RB_POPSLOT(&out); /* start transmission of next byte */
        RB_POPADVANCE(&out); /* remove the sent byte from buffer */
    } else {
        PIE1bits.TX1IE = 0; // desabilita transmission
        TXactive = 0; /* TX finished, interface inactive */
    }
}

/*---------------------------------------------------------------------
  Function name: vInitU1
  Description: Initialization of UART1 module
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void vInitU1(uint16_t USART_BRG, uint8_t polarity_inverted) {
    RB_INIT(&out, outbuf, MAX_BUFLEN); /* configura TX ring buffer */
    RB_INIT(&in, inbuf, MAX_BUFLEN); /* configura RX ring buffer */

    BufferMtRcv = 0; //informa que não há novo byte, evento

    //Transmit Status and Control (TXSTA)
    TXSTA1 = 0b00100010;

    //Receive Status and Control (RCSTA)
    RCSTA1 = 0b00010000;

    //Baud Rate Control (BAUDCON)
    BAUDCON = 0b00001000;
    if (polarity_inverted)
        BAUDCONbits.RXDTP = 1; //RX data is inverted

    SPBRG = USART_BRG;
    SPBRGH = USART_BRG >> 8;


#if ((MOTOVER == RevA) || (MOTOVER == RevB))
    TRISCbits.TRISC7 = 1; //RX
    TRISCbits.TRISC6 = 1; //TX
#elif (MOTOVER == RevC)
    RCSTA1bits.SPEN = 1;
    TRISCbits.TRISC7 = 1;
    TRISCbits.TRISC6 = 1;
#else
#error Revision number of Hardware not defined. Please define it in user.h file.
#endif

    /*libera interrupção pela porta serial*/
    PIR1bits.TX1IF = 0; 
    PIR1bits.RC1IF = 0; // Clear the Recieve Interrupt Flag
    PIE1bits.TX1IE = 1;
    PIE1bits.RC1IE = 1;// Enable Recieve Interrupts

    RCSTA1bits.SPEN = 1; // Enable UART1 module
}

/*---------------------------------------------------------------------
  Function name: serOutchar
  Description: Sends buffer characters to UART1 (use of blocking wait)
  Input parameters: Character to send
  Output parameters: -
-----------------------------------------------------------------------*/
int serOutchar(char c) {
#ifndef depuracao_serial_usart
  while(RB_FULL(&out)) CLRWDT();        /* wait until there's space */

  *RB_PUSHSLOT(&out) = c;               /* store data in the buffer */
  RB_PUSHADVANCE(&out);                 /* adjust write position */

    if (!TXactive) {
        PIE1bits.TX1IE = 0; /* desabilita transmission                                            ? only required once */
        TXREG1 = *RB_POPSLOT(&out); /* start transmission of next byte */
        RB_POPADVANCE(&out); /* remove the sent byte from buffer */
        TXactive = 1; /* indicate ongoing transmission */
        PIE1bits.TX1IE = 1; /* start transmission                                            ? only required once */
    }
#endif
    return 0;
}

/*---------------------------------------------------------------------
  Function name: serInchar
  Description: Receives character from UART1 (use of blocking wait)
  Input parameters: -
  Output parameters: Received character
-----------------------------------------------------------------------*/
void PCTrigger(void){CLRWDT(); };

char serInchar(void) {
    char c;
//    BufferMtRcv seja o indicador de byte recebido
    BufferMtRcv = 0;
#ifdef depuracao_serial_usart
    /*ponto de inserção da mensagem de teste*/
    NOP();
    c = ADRESL;
    BufferMtRcv = ADRESH;
#else
    if (RB_EMPTY(&in)) {
        return 0;
    };

    c = *RB_POPSLOT(&in); /* get character off the buffer */
    RB_POPADVANCE(&in); /* adjust write position */

    //BufferMtRcv seja o indicador de byte recebido
    BufferMtRcv = 1;
#endif
    return c;
}

/*---------------------------------------------------------------------
  Function name: serOutstring
  Description: Sends string from buf to UART1
  Input parameters: String to send
  Output parameters: -
-----------------------------------------------------------------------*/
void serOutstring(char *buf, int len) {
    char *ponteiro;

//    int ponteiro;
//
//    for(ponteiro = 0; ponteiro < len; ponteiro++)
//     serOutchar(buf[ponteiro]);

    ponteiro = buf;
    while (ponteiro < &buf[len]){
        serOutchar(*(ponteiro++));
}
}

/*---------------------------------------------------------------------
  Function name: vPutCharU1
  Description: Sends one character to UART1 (use of blocking wait)
  Input parameters: Character to send
  Output parameters: -
-----------------------------------------------------------------------*/
void vPutCharU1 ( CHAR cChar )
{
	while (!TXSTA1bits.TRMT1); // Waits when the output buffer is full
	TXREG = cChar;         // Puts the character to the buffer
}

/*---------------------------------------------------------------------
  Function name: vPutStrU1
  Description: Sends string to UART1
  Input parameters: String to send
  Output parameters: -
-----------------------------------------------------------------------*/
void vPutStrU1 ( CHAR* pcStr )
{
	while( *pcStr != 0 )
	{
		vPutCharU1(*pcStr++);
	}
}

/*---------------------------------------------------------------------
  Function name: cGetCharU1
  Description: Receives character from UART1 (use of blocking wait)
  Input parameters: -
  Output parameters: Received character
-----------------------------------------------------------------------*/
CHAR cGetCharU1( void )
{
    while (!PIR1bits.RC1IF); // Waits when the input buffer is empty
    return RCREG; // Returns with the received character
}
