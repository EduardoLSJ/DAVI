/*
 * Using UART1 module
 *
 * Tested on:
 * Microstick + Microstick Plus Development Board
 * PIC24HJ64GP502
 *
 * File name: uart1.h
 * Author:    Jozsef KOPJAK
 * Info:      info@microstickplus.com
 * Web:       http://www.microstickplus.com
 *
 * Last modification: 10-04-2013
 */

#ifndef _UART1_H
#define _UART1_H

#include "typedefs.h"
#include "system.h"
#include <stdint.h>

/******************************************************************************/
/* Variables definition                                                       */
/******************************************************************************/
/* Speed of the UART1 module */
#define BAUD_RATE_UART1_1200	1200L
#define USART_BRG_1200 ((long) SYS_FREQ / (16 * (long)BAUD_RATE_UART1_1200)) - 1
#define BAUD_RATE_UART1_9600	9600L
#define USART_BRG_9600 ((long) SYS_FREQ / (16 * (long)BAUD_RATE_UART1_9600)) - 1

/*---------------------------------------------------------------------------
  MACRO: True if the serial ports input buffer contains received data
---------------------------------------------------------------------------*/
#define mIsU1RXDataAvailable() (PIR1bits.RC1IF)

extern char BufferMtRcv;

/*---------------------------------------------------------------------
  
 * @param USART_BRG
 * @param polarity
  Output parameters: -
-----------------------------------------------------------------------*/
/**
 * Function name: vInitU1
  Description: Initialization of UART1 module
  Input parameters:
 * @param USART_BRG seleciona a velocidade de comunicação
 * @param polarity - informar se a polaridade é invertida (RXDTP = 0x01)
 */
void vInitU1(uint16_t USART_BRG, uint8_t polarity_inverted);

/*---------------------------------------------------------------------
  Function name: serOutchar
  Description: Sends buffer characters to UART1 (use of blocking wait)
  Input parameters: Character to send
  Output parameters: -
-----------------------------------------------------------------------*/
int serOutchar(char c);

/*---------------------------------------------------------------------
  Function name: serInchar
  Description: Receives character from UART1 (use of blocking wait)
  Input parameters: -
  Output parameters: Received character
-----------------------------------------------------------------------*/
char serInchar(void);

/*---------------------------------------------------------------------
  Function name: serOutstring
  Description: Sends string from buf to UART1
  Input parameters: String to send
  Output parameters: -
-----------------------------------------------------------------------*/
void serOutstring(char *buf, int len);

/*---------------------------------------------------------------------
  Function name: vPutCharU1
  Description: Sends one character to UART1 (use of blocking wait)
  Input parameters: Character to send
  Output parameters: -
-----------------------------------------------------------------------*/
void vPutCharU1(CHAR cChar);

/*---------------------------------------------------------------------
  Function name: vPutStrU1
  Description: Sends string to UART1
  Input parameters: String to send
  Output parameters: -
-----------------------------------------------------------------------*/
void vPutStrU1(CHAR* pcStr);

/*---------------------------------------------------------------------
  Function name: cGetCharU1
  Description: Receives character from UART1 (use of blocking wait)
  Input parameters: -
  Output parameters: Received character
-----------------------------------------------------------------------*/
CHAR cGetCharU1(void);

#endif /* _UART1_H */
