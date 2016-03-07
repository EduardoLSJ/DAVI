/*
 * interruptions
 *
 * Tested on:
 * pic18f
 *
 * File name: interrupts.h
 * Author:    Eduardo Lopes
 * Info:      elopes_74@hotmail.com
 *
 * Last modification: 03-11-2013
 */

#ifndef _interrupts_h
#define _interrupts_h

#include "machine_main.h"

/* used to hold the 32-bit captured value */
union Capture_ccp2 {
    int32_t tempo_int32; /*TODO remover teste signed long tempo_int32; //armazena o tempo total*/
    struct {
        uint16_t low_int;
        int16_t high_int; /*TODO remover teste signed int high_int;*/
    };
    struct {
        char ccpr_L; /**holds the 16-bit low captured value*/
        char ccpr_H; /**holds the 16-bit high captured value*/
    };
    struct {
        char;
        unsigned :7;
        unsigned flag_ccp_ovf :1;
        char;
        unsigned :7;
        unsigned flag_tempo_ovf :1;
    };
};

struct flags_struct_int{
    unsigned tempo_fracao:1; /*informa que um pulso indicativo de fração ocorreu*/
    unsigned distancia:1; /*informa que a distancia programada foi percorrida*/
    unsigned levanta_w_mode:1; /*no modo levanta W detecta as duas bordas do pulso*/
    unsigned flag_1_segundo:1; /*intervalo de 1 segundo*/
};

extern volatile struct flags_struct_int flag_int;
extern volatile uint8_t INT_LATENCY_VALUE;
extern volatile uint8_t INT_LATENCY_RELOAD;
extern volatile uint32_t tempo_fracao;
extern volatile uint32_t odometro_out;
extern volatile uint32_t odometro_in;
extern uint32_t odometro_out_max; /*numero de pulsos a percorrer no ensaio quilometrico*/
extern volatile uint32_t odometro_fracao;

/*---------------------------------------------------------------------
  Function name: _U1RXInterrupt
  Description: RECEBE caracter da usart1
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void _U1RXInterrupt(void);

/*---------------------------------------------------------------------
  Function name: _U1TXInterrupt
  Description: TRANSMITE caracter para usart1
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
void _U1TXInterrupt(void);

/*---------------------------------------------------------------------
  Function name: _U2Interrupt
  Description: RECEBE caracter da usart2
  Input parameters: -
  Output parameters: -
-----------------------------------------------------------------------*/
/*********************************************************************************************************
                               Interrupt Service Routine

 Only RTCC Inturrupt Is used. RTCC Inturrupt is used as timing for Serial Port Receive & Transmit
 Since RS-232 is implemented only as a Half Duplex System, The RTCC is shared by both Receive &
 Transmit Modules.
       Transmission :
                       RTCC is setup for Internal Clock increments and interrupt is generated when
                       RTCC overflows. Prescaler is assigned, depending on The INPUT CLOCK & the
                       desired BAUD RATE.
       Reception :
                       When put in receive mode, RTCC is setup for external clock mode (FALLING EDGE)
                       and preloaded with 0xFF. When a Falling Edge is detected on RTCC Pin, RTCC
                       rolls over and an Interrupt is generated (thus Start Bit Detect). Once the start
                       bit is detected, RTCC is changed to INTERNAL CLOCK mode and RTCC is preloaded
                       with a certain value for regular timing interrupts to Poll RTCC Pin (i.e RX pin).

********************************************************************************************************/
void _U2Interrupt(void);

void interrupt myIsr(void);
void interrupt low_priority myLoIsr(void);

#endif /* _interrupts_h */
