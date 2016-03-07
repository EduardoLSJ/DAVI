/******************************************************************************/
/* System Level #define Macros                                                */
/******************************************************************************/
#ifndef _system_h
#define	_system_h

/* Device header file */
#if defined(__XC)
    #include <xc.h>         /* XC8 General Include File */
#endif

/*Os trechos de código de depuração são acionados automaticamente na simulação*/
#ifdef __DEBUG
#endif

/*comentar estas linhas, se estiver em produção*/
//#define depuracao
//#define depuracao_serial_bitbang
//#define depuracao_serial_usart
//#define depuracao_rn41

/*comentar esta linha se não quiser proteger contra alterações em produção*/
#define integridade_check

/* Microcontroller MIPs (FCY) */
#define SYS_FREQ        40000000L
#define FCY             SYS_FREQ/4
#define _XTAL_FREQ      40000000L

/******************************************************************************/
/* System Function Prototypes                                                 */
/******************************************************************************/

/* Custom oscillator configuration funtions, reset source evaluation
functions, and other non-peripheral microcontroller initialization functions
go here. */

void ConfigureOscillator(void); /* Handles clock switching/osc initialization */

#endif /* _system_h */