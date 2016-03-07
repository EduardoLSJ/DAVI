/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/
#if defined(__XC)
#include <xc.h>        /* XC8 General Include File */
#endif

#if defined(__XC)
//#include <stdbool.h>       /* For true/false definition */
#endif

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "uart1.h"
#include "uart2.h"
#include "user.h"
#include "state_machine.h"
#include "interrupts.h"

#pragma warning disable 359
/******************************************************************************/
/* User Global Variable Declaration                                           */
/******************************************************************************/

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/
void main(void) {
    unsigned char Machine;

    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize I/O and Peripherals for application */
    InitApp();

    /*
     serão três máquinas de estado ativas por simultaneamente:
     maquina 0 - controle do processo de verificação MAIN
     maquina 1 - comunicação serial com PC - DAVI
     maquina 2 - comunicação serial com MT - 393
    
     as máquinas abaixo são mutamente exclusivas (MUTEX) com a máquina 0
     maquina 3 - ensaio de modo quilometrico
     maquina 4 - ensaio de levantamento de w
     maquina 5 - ensaio de modo horario
     maquina 6 - calibração do DAVI
     maquina 7 - ajuste de parametros do DAVI

     as fontes de interrupção serão:
     1 - interrupção de tempo do TMR0 - bit de dados serial PC
     2 - interrupção de tempo do TMR1 - evento de captura de tempo e pulsos
     3 - interrupção de tempo do TMR2 - monitora time-out serial (PC ou MT)
     4 - interrupção de tempo do TMR3 - base de tempo para medir pulsos
     5 - interrupção CCP1 - gera pulsos de saida
     6 - interrupção CCP2 - informa captura de pulso de entrada
     7 - interrupção USART_RX
     8 - interrupção USART TX

     4 - evento de mensagem PC completa e disponível
     5 - evento de mensagem MT completa e disponível
*/
    do {
        for (Machine = 0; Machine < MachineNumberMax; Machine++)
                state_machine_dispatcher(Machine);
        CLRWDT();
    } while (true);
}
