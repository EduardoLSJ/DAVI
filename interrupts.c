/******************************************************************************/
/*Files to Include                                                            */
/******************************************************************************/

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#endif

#if defined(__XC)
#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#include <compare.h>
#include <timers.h>
#include <math.h>
#include <stdlib.h>
#endif

#include "system.h"
#include "interrupts.h"
#include "user.h"
#include "machine_prot_393.h"
#include "machine_prot_davi.h"
#include "machine_main.h"

/******************************************************************************/
/* Interrupt Routines                                                         */
/******************************************************************************/
volatile unsigned char INT_LATENCY_VALUE; /**constante para correção do contador de tempo*/
/*localizado no ACCESS BANK. 3.4.7.5 em xc8 compiler*/
volatile uint8_t INT_LATENCY_RELOAD @ 0xD3; /**constante para correção do gerador de frequencia*/
volatile struct flags_struct_int flag_int = 0;
volatile uint32_t tempo_fracao;
volatile union Capture_ccp2 tempo_timer3_start = 0;
volatile union Capture_ccp2 tempo_timer3_end = 0;
volatile uint32_t odometro_out;
volatile uint32_t odometro_in;
uint32_t odometro_out_max;
volatile uint32_t odometro_fracao; /**totalizador do número de pulsos a cada fração*/
volatile uint8_t atraso1seg;
uint32_t tempo_debounce;

#if defined(__XC8)
void interrupt myIsr(void)
#else
#error "Invalid compiler selection for implemented ISR routines"
#endif
{
    /* Determine which flag generated the interrupt */
    /**
     * gerador de pulsos para a porta de saida PULSE_OUT
     * incrementa contador para informar a distância percorrida
     */
    if (PIR1bits.CCP1IF && PIE1bits.CCP1IE) {
        /**
         *  recarrega valor de TMR1 ao entrar na rotina de tratamento de interrupção
         * atualiza o valor para que a contagem de tempo seja precisa
         */
        asm("MOVLW 0x00"); //37 ciclos para chegar até aqui, após CCP1IF
        asm("MOVWF TMR1H"); //38
        asm("MOVFF _INT_LATENCY_RELOAD, TMR1L");
        asm("NOP"); //41
        if (CCP1) {
            CCP1CON = 0x0F & COM_LO_MATCH;
            //incrementa odometro no pulso de subida
            //incrementar contador de pulsos, para totalizar o odometro!
            //totalizado de 20 km, resolução de 0.1 m
            //kmax = 65535pulsos/km
            //em 20 km, teremos 13107000000 pulsos, ultrapassa 32bits.
            //dividir:= D_ensaio /4
            odometro_out++;
            if (odometro_out >= odometro_out_max) {
                /*informa que a distancia programada foi percorrida*/
                flag_int.distancia = true;
            }
        } else {
            CCP1CON = 0x0F & COM_HI_MATCH;
        }
        PIR1bits.CCP1IF = false;
    }

    /**
     *
     * interrupção com alta prioridade
     * interrupção com a subida do INPUT PIN
     * a leitura e realizada do byte ccp2L
     * neste momento, ccp2H é armazenado no buffer
     * com a resolução de 0.1us por bit, e possível contar até 6.5ms com CCP2
     * é necessário extender a contagem de tempo. adicionado 16 bits
     * logo a contagem será de até 3min 34s. 0x7FFF.FFFF, ainda com 0.1us de
     * resoluçao
     */
    if (PIE2bits.CCP2IE && PIR2bits.CCP2IF) {
        //salva o intervalo do registrador menos significativo
        tempo_timer3_end.low_int = ReadCapture2();

        //se houve overflow no contador TMR3, é mostrado no flag TMR3IF
        if (PIR2bits.TMR3IF) {
            //incrementa contador, corretamente.
            //se o overflow aconteceu antes de capturar CCP2:
            /*TODO avaliar se 2^7 é um valor adequado para dizer que houve interrupção em um intervalo próximo!*/
            if (!(tempo_timer3_end.flag_ccp_ovf)) {
                //é necessário corrigir nosso contador de tempo
                tempo_timer3_end.high_int += 1;
            }
            PIR2bits.TMR3IF = false;
        }
        //calcula o intervalo total entre bordas de subida
        tempo_debounce = labs(tempo_timer3_end.tempo_int32
                - tempo_timer3_start.tempo_int32);

        /**
         * debounce
         * testa se intervalo é maior que 350us
         * senão, mantém o valor inicial e não apresenta nova medida
         * e continua contando
         */
        if (tempo_debounce > debounce_edge_time) {
            tempo_fracao = tempo_debounce;
            flag_int.tempo_fracao = true;
            odometro_in++;
            //atualiza valor inicial
            tempo_timer3_start.tempo_int32 =
                    tempo_timer3_end.tempo_int32;
            /*atualiza a distância percorrida entre frações*/
            odometro_fracao = odometro_out;
            /*no levantamento de W, detectar bordas de subida e descida*/
            if (flag_int.levanta_w_mode) {
                /*verifica qual borda foi detectada*/
                if (CCP2CON & 0x01) {
                    CCP2CON = 0x0F & CAP_EVERY_FALL_EDGE;
                } else {
                    CCP2CON = 0x0F & CAP_EVERY_RISE_EDGE;
                }
            }
        }
        PIR2bits.CCP2IF = false;
    } else
        /**
         * habilita RX/TX do serial Protocol_393
         */
        if (INTCONbits.TMR0IF && INTCONbits.TMR0IE) {
        INTCONbits.T0IF = false;
        _U2Interrupt();
    }
}

void interrupt low_priority myLoIsr(void) {
    /**
     * habilita RX do Protocol_DAVI
     */
    if (PIR1bits.RCIF && PIE1bits.RCIE) {
        _U1RXInterrupt();
    }

    /**
     * habilita TX do Protocol_DAVI
     */
    if (PIR1bits.TX1IF && PIE1bits.TX1IE) {
        _U1TXInterrupt();
    }

    /**
     * interrupção de baixa prioridade
     * interrupção com o overflow do TIMER3 = 65535us
     * 65535*65536us = 7min 9s total
     * extende a contagem do CCP2
     */
    if (PIR2bits.TMR3IF && PIE2bits.TMR3IE) {
        tempo_timer3_end.high_int += 1;
        PIR2bits.TMR3IF = false;
    }

    /**
     * Contador de intervalos
     * intervalo de timeout para protocolos de comunicação
     * intervalo de 1 segundo para rotina de aceleração
     */
    if (PIR1bits.TMR2IF && PIE1bits.TMR2IE) {
        if (contador_timeout_davi) contador_timeout_davi--;
        if (contador_timeout_393) contador_timeout_393--;
        if (contador_timeout) contador_timeout--;
        if (contador_PC_timeout) contador_PC_timeout--;

        atraso1seg++;
#ifdef depuracao
        if (atraso1seg == 1) {//aguarda 4 ms
            atraso1seg = 0;
            flag_int.flag_1_segundo = true;
        }
#else
        if (atraso1seg == 249) {//aguarda 1 segundo
            atraso1seg = 0;
            flag_int.flag_1_segundo = true;
        }
#endif
        PIR1bits.TMR2IF = false;
    }
}
