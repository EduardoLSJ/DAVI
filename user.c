/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
    #include <xc.h>         /* XC8 General Include File */
#endif

#if defined(__XC)
#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#include <capture.h>
#include <timers.h>
#endif

#include "system.h"
#include "user.h"
#include "uart1.h"
#include "uart2.h"
#include <usart.h>
#include "rn41.h"
#include "state_machine.h"
#include "interrupts.h"
#include <math.h>
#include "md5.h"
#include "md5_private.h"

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/
flags_struct_ensaio flags_ensaio = false;

// Definition of software version
/* to indicate the compiler?s version number multiplied
by 1000, e.g., v1.00 will be represented by 1000*/
const uint16_t DAVI_VERSAO_ATUAL __at(_DAVI_VERSION_LOCATION) = _DAVI_VERSION_VALUE;
const uint16_t COMM_ROM_INIT __at(_COMM_ROM_INIT_LOCATION) = _COMM_ROM_INIT_VALUE;//_comm_helper determinar após compilação
const uint16_t COMM_ROM_SIZE __at(_COMM_ROM_SIZE_LOCATION) = _COMM_ROM_SIZE_VALUE;//_end_of_comm_helper - _comm_helper determinar após compilação
const uint8_t COMM_ROM_HASH[] __at(_COMM_ROM_HASH_LOCATION) = _COMM_ROM_HASH_VALUE;//calcular após compilação
const uint8_t DAVI_ROM_HASH[] __at( _DAVI_ROM_HASH_LOCATION) = _DAVI_ROM_HASH_VALUE;


/*Tabela EEPROM com as constantes de programação e consistência.*/
/*o espaço livre ou não utilizado é preenchido com*/
/* valor arbitrário encontrado em https://www.random.org/integers/*/

/*AJUSTE_LATENCIA, AJUSTE_TEMPO, NUM_SERIE*/
asm("\tpsect eeprom_data,class=EEDATA");
asm("\torg\t" ___mkstr(EE_AJUSTE_LATENCIA));
__EEPROM_DATA(43, 10, 01, 23, 251, 107, 147, 177);
__EEPROM_DATA(0x61,0x0A,0x82,0x7A,0x65,0xD0,0xBC,0xD6);

 /*HASH_DADOS: Hash da área de dados*/
asm("\tpsect eeprom_data,class=EEDATA");
asm("\torg\t" ___mkstr(EE_HASH_DADOS));
__EEPROM_DATA(0x6F,0x83,0xF7,0xB6,0xDD,0xA7,0x7F,0x0A);	
__EEPROM_DATA(0x97,0x20,0xAB,0x54,0x32,0xF2,0x5A,0x29);

/*
  The compiler?s HEXMATE utility (see Section 8.3 ?HEXMATE?) has the capability to fill
unused locations and this operation can be requested using a command-line driver
option; see Section 4.8.31 ?--FILL: Fill Unused Program Memory?.
  */
/*HASH_EEPROM: Hash de toda a memória EEPROM*/
asm("\tpsect eeprom_data,class=EEDATA");
asm("\torg\t" ___mkstr(EE_HASH_FREE_EEPROM));
__EEPROM_DATA(0x86,0x9B,0x18,0xC7,0x14,0xAC,0x0D,0x56);
__EEPROM_DATA(0x1B,0x93,0x08,0x48,0x41,0xD6,0x0D,0x72);

void InitApp(void)
{
    /* inicialização dos bits de reset */
    RCONbits.SBOREN = false;
    RCONbits.POR = true;
    RCONbits.nBOR = true;

    /*habilita prioridade de interrupção */
    RCONbits.IPEN = true; 
    /* fontes de interrupção com baixa prioridade */
    IPR1bits.RC1IP = false;
    IPR1bits.TX1IP = false;
    IPR1bits.TMR2IP = false;
    IPR1bits.TMR1IP = false;
    IPR2bits.TMR3IP = false;

    /* Setup analog functionality and port direction */

    /* Verifica condição de saúde */
#ifdef integridade_check
    IntegridadeGlobal();
#else
    causa_erro_inicializacao = FALHA_INEXISTENTE;
#endif

    /*State machine initializer*/
    state_machine_initializer();

    /* SERIAL PORT*/
//    vInitU1(USART_BRG_1200, false); //ainda não habilitar interrupção
    vInitU2();

    /* PULSE_IN */
    TurnOnPulseIn();
    ConfigPulseIn(false);

    /* PULSE_OUT */
    ConfigPulseOutOff();

    /* PULSE source counter*/
    InitPulseCounter();

    /*Timeout counter*/
    InitTimeoutCounter();

    /* Enable interrupts */
    PIR1 = 0;
    PIR2 = 0;
    INTCONbits.PEIE = 1;

    ei();

#ifndef depuracao_rn41
    //configure RN41 module on serial2
#if ((MOTOVER == RevA) || (MOTOVER == RevC))
    Rn41Init();
#elif (MOTOVER == RevB)
    HC05Init();
    /*TODO criar rotina de inicialização para HC05*/
#else
#error Revision number of Hardware not defined. Please define it in user.h file.
#endif
#endif

    di();
    vInitU1(USART_BRG_1200, false);
    
    ei();
}

void IntegridadeGlobal() {
//    MD5_CONTEXT contextMd5;
    uint8_t digest[16];
    const uint16_t *ROM_ini, *ROM_len;
    uint8_t EE_ini, EE_fim;
    uint8_t i;
    /*ponteiro para os blocos de memoria das funcoes com_helper e bootloader*/
    const uint8_t *rom_byte;
    
    /*bloquear o uso do instrumento em caso de falha*/
    /*separar resultado de integridade de blocos da menor para a maior criticidade:*/
    causa_erro_inicializacao = FALHA_INEXISTENTE;
    
    /*1. EEPROM total - erro não crítico. Pode ser falha na faixa livre*/
    /*copia o valor hash gravado na EEPROM*/
    for (i = 0; i < 16; i++) {
        digest[i] = Read_b_eep(EE_HASH_FREE_EEPROM + i);
    }
    EE_ini = EE_FREE_INICIO;
    EE_fim = EE_HASH_FREE_EEPROM;
    /*checa a integridade por comparação de hash*/
    i = IntegridadeEEPROM(EE_ini, EE_fim, digest);
    if (!i) {
        /*falhou, se prepara para gerar relatório*/
        causa_erro_inicializacao = FALHA_HASH_EEPROM_FREE;
    }
    
    /*2. EEPROM dados - erro não crítico. Solicitar ajuste*/
    /*copia o valor hash dos dados EEPROM*/
    for (i = 0; i < 16; i++) {
        digest[i] = Read_b_eep(EE_HASH_DADOS + i);
    }
    EE_ini = 0;
    EE_fim = EE_HASH_DADOS;
    /*checa a integridade por comparação de hash*/
    i = IntegridadeEEPROM(EE_ini, EE_fim, digest);
    if (!i) {
        /*falhou, se prepara para gerar relatório*/
        causa_erro_inicializacao = FALHA_DADOS_EEPROM;
    }
    
    /*3. programa principal - bloqueia ação e transmite erro em COMM*/
    /*copia o valor hash da ROM*/
    for (i = 0; i < 16; i++) {
        /*le o hash do programa principal*/
        digest[i] = DAVI_ROM_HASH[i];
    }
    /*checa a integridade por comparação de hash*/
    i = IntegridadeRom(_DAVI_APP_START, _DAVI_ROM_HASH_LOCATION - _DAVI_APP_START, digest);
    if (!i) {
        /*falhou, se prepara para gerar relatório*/
        causa_erro_inicializacao = FALHA_APP_ROM;
    }
        
    /*4. bootloader - crítico. transmite erro em COMM.*/
    /*copia o valor hash da FLASH no bootloader*/
    rom_byte = (const uint8_t *)_BOOT_ROM_HASH_LOCATION;
    for (i = 0; i < 16; i++) {
        /*le o hash do BOOTLOADER*/
        digest[i] = *rom_byte;
        rom_byte++;
    }
    /*checa a integridade por comparação de hash*/
    i = IntegridadeRom(0, _BOOT_ROM_HASH_LOCATION, digest);
    if (!i) {
        /*falhou, se prepara para gerar relatório*/
        causa_erro_inicializacao = FALHA_BOOT_ROM;
    }
    
    /*TODO transportar a função COMM_HELPER do bootloader para o DAVI*/
    /*5. comunicação serial - não há como transmitir em caso de erro*/
    /*copia o valor hash da função COMM_HELPER*/
    rom_byte = (const uint8_t *) _COMM_ROM_HASH_LOCATION;
    for (i = 0; i < 16; i++) {
        /*le o hash da função COMM_HELPER*/
        digest[i] = *rom_byte;
        rom_byte++;
    }
    /*TODO capturar os dois bytes com o tamanho da função em ROM*/
    ROM_ini = (const uint16_t*) _COMM_ROM_INIT_LOCATION;
    ROM_len = (const uint16_t*) _COMM_ROM_SIZE_LOCATION;
    /*checa a integridade por comparação de hash*/
    i = IntegridadeRom(*ROM_ini, *ROM_len, digest);
    if (!i) {
        /*falhou, não há como gerar relatório. Pare a máquina por segurança.*/
        causa_erro_inicializacao = FALHA_COMM_HELPER;
    }
 
    switch (causa_erro_inicializacao) {
        case FALHA_INEXISTENTE:
        case FALHA_DADOS_EEPROM:
        case FALHA_HASH_EEPROM_FREE:
        case FALHA_BOOT_ROM:
            /*sair sem erro.*/
            /*falha somente nos dados. Permite correção por Ajuste*/
            /*falha na eeprom livre, com numeros aleatorios, informa usuário*/
            /*falha no BOOT, informa usuário*/
            break;
        case FALHA_COMM_HELPER:
            /*falhou, não há como gerar relatório. 
             * Pare a máquina por segurança.*/
#ifdef integridade_check            
            while (true);
#endif
            break;
        default:
            /*COMM_HELPER funcionando, informe a causa da falha 
             * por mensagem fixa até reload*/
            /*falha no programa principal*/
#ifdef integridade_check
            comm_helper(causa_erro_inicializacao);
            while(true);
#endif
            break;
    };
}
    
int8_t IntegridadeRom(uint16_t ini, uint16_t len, uint8_t* result){
    MD5_CONTEXT contextMd5; // Context for MD5
    uint8_t digest[16];
    uint8_t i;
    bool confere;

    /*inicia o calculo de hash*/
    MD5_Initialize(&contextMd5);
    /*calculo de hash de toda a memoria de programa*/
    MD5_ROMDataAdd(&contextMd5, (const uint8_t *) ini, (uint32_t) len);
    /*encerra calculo de hash*/
    MD5_Calculate(&contextMd5, digest);

    /*compara o valor hash com valor fornecido e copia para o resultado*/
    confere = true;
    for (i = 0; i < 16; i++) {
        if (result[i] != digest[i])
            confere = false;
        //copiar digest em result
        result[i] = digest[i];
    }
    if(confere)
        return true;
    return false;
}

int8_t IntegridadeEEPROM(uint8_t ini, uint8_t fim, uint8_t* result){
    MD5_CONTEXT contextMd5; // Context for MD5
    uint8_t digest[16];
    uint8_t i, len;
    bool confere;

    /*carrega o valor inicial de memoria*/
    MD5_Initialize(&contextMd5);
    /*solicita calculo de hash da memoria EEPROM*/
    /*copia em blocos parciais*/
    len = 0;
    for (i = ini; i < fim; i++) {
        digest[len++] = Read_b_eep(i);
        if ((len == 16) || (i == fim - 1)) {
            MD5_DataAdd(&contextMd5, (uint8_t *) digest, len);
            len = 0; /*recomeço*/
        }
    }
    
    /*encerra calculo de hash*/
    MD5_Calculate(&contextMd5, digest);
    /*compara o valor hash com valor fornecido e copia para o resultado*/
    confere = true;
    for (i = 0; i < 16; i++) {
        if (result[i] != digest[i])
            confere = false;
        //copiar digest em result
        result[i] = digest[i];
    }
    if (confere)
        return true;
    return false;
}

void comm_helper(uint8_t erro_detectado) {
    static uint8_t falha;
    uint32_t contador;
    union CRC_table{
        uint16_t crc16;
        struct {//crc em bytes
            uint8_t low;
            uint8_t high;
        }crc8;
    };

    union CRC_table crc[] = {0x9884, 0x1881, 0x188B, 0x988E, 0x189F,\
                             0x989A, 0x9890, 0x1895, 0x18B7, 0x98B2,\
                             0x98B8, 0x18BD, 0x98AC, 0x18A9, 0x18A3,\
                             0x98A6, 0x18E7, 0x98E2, 0x98E8, 0x18ED,\
                             0x98FC, 0x18F9, 0x18F3, 0x98F6, 0x98D4};
   
    falha = erro_detectado & 0x1F;
    if(falha >= 25) falha = 0;
    /*inicia a interface serial*/
    CloseUSART();
    TXSTA = 0;
    RCSTA = 0;
    BAUDCON = 0;

    TXSTAbits.TXEN = 1;
    TXSTAbits.SYNC = 0;
    TXSTAbits.BRGH = 0;

    RCSTAbits.CREN = 1; //enable receiver

    BAUDCONbits.BRG16 = 1;
    SPBRG = USART_BRG_1200;
    SPBRGH = USART_BRG_1200 >> 8;

    TRISCbits.RC6 = 1; //output
    TRISCbits.RC7 = 1; //input
    /*bloqueia interrupção pela porta serial*/
    PIR1bits.TX1IF = 0;
    PIR1bits.RC1IF = 0; // Clear the Recieve Interrupt Flag
    PIE1bits.TX1IE = 0;
    PIE1bits.RC1IE = 0; // Enable Recieve Interrupts

    RCSTAbits.SPEN = 1;

    /*mantenha-se alerta*/
    while (true) {
        /*transmite a mensagem completa, com identificação do erro*/
        TXREG = '\xA5'; /*cabeçalho*/
        while (TXSTAbits.TRMT == 0); //wait empty
        TXREG = '\x00'; /*comando*/
        while (TXSTAbits.TRMT == 0); //wait empty
        TXREG = '\x00'; /*comando*/
        while (TXSTAbits.TRMT == 0); //wait empty
        TXREG = '\x02'; /*numero de bytes*/
        while (TXSTAbits.TRMT == 0); //wait empty
        TXREG = '\x00'; /*comando*/
        while (TXSTAbits.TRMT == 0); //wait empty
        TXREG = falha; /*codigo do erro*/
        while (TXSTAbits.TRMT == 0); //wait empty
        /*monta a mensagem de erro (CRC)*/
        TXREG = crc[falha].crc8.high; /*CRC*/
        while (TXSTAbits.TRMT == 0); //wait empty
        TXREG = crc[falha].crc8.low; /*CRC*/
        while (TXSTAbits.TRMT == 0); //wait empty

        /*repete a cada 1/2 segundo*/
        for (contador = 0; contador <= 200000; contador++) {
            NOP();
        }
    }
}

void Delay_ms(uint32_t atraso) {
    uint32_t contador;
    for (contador = 1; contador < atraso; contador++){
        __delay_ms(1);
        CLRWDT();
    }
}

void ConfigPulseIn(bool enable) {
    //initialize CCP2 capture mode
    //capture mode every rising edge
    //the CCPR2H:CCPR2L register pair effectively becomes
    //a period register for Timer3.
    if (enable) {
#if (MOTOVER == RevA)
        //para reabilitar, apenas use o CCP2IE
        if (!CCP2CON) {
            CCP2CON = 0x0F & CAP_EVERY_FALL_EDGE;
            PIR2bits.CCP2IF = 0; // Clear the interrupt flag
        }
#elif (MOTOVER == RevB || MOTOVER == RevC)
        //para reabilitar, apenas use o CCP2IE
        if (!CCP2CON) {
            CCP2CON = 0x0F & CAP_EVERY_RISE_EDGE;
            PIR2bits.CCP2IF = 0; // Clear the interrupt flag
        }
#endif
        PIE2bits.CCP2IE = 1; // Enable the interrupt
    } else { //desliga CCP2
        PIE2bits.CCP2IE = 0;
    }
    flag_int.levanta_w_mode = false; /*sempre habilitar apenas borda de subida*/
}

void ConfigPulseOutOff(void){
    /*desliga CCP1*/
        CCP1CON = 0x00;
        PIE1bits.CCP1IE = false;
        TurnOffPulseOut();
}

void InitPulseCounter() {
    unsigned char config_timer3;
    //inicializa fonte de clock para CCP
    config_timer3 = TIMER_INT_ON & T3_16BIT_RW
            & T3_SOURCE_INT & T3_PS_1_1
            & T3_SYNC_EXT_ON & T1_CCP1_T3_CCP2;
    OpenTimer3(config_timer3);
    //    configura o clock para cada módulo correspondente
    SetTmrCCPSrc(T1_CCP1_T3_CCP2);
    PIE2bits.TMR3IE = true;
}

void InitTimeoutCounter(){
    /* configurar TIMER2 para gerar eventos de tick/timeout*/
    //interruption every 4ms

    T2CON = 0x4F;
    PR2 = PR2_VALUE;
    PIE1bits.TMR2IE = 1;
}

/*================================================================
=  calculo do periodo
==================================================================
pulsos/seg = velocidade(km/h) * w(pulsos/volta) * k(volta/km) * 1/3600(h/seg)
w = 8
k = 1000
periodo = 1/ pulsos/seg
================================================================*/
//recalcula valor de CCP para velocidade desejada
void gera_velocidade(char *vel, uint16_t *k) {
    float rascunho;
    float entrada;
    int ponteiro;
    char T1CON_temp;
    unsigned short CCPR1_temp;

    if (*vel == 0) {
        /*desliga CCP1*/
        ConfigPulseOutOff();
    } else {
        entrada = (float) *vel;
        //religa CCP
        rascunho = ((18E9 / (float) *k) / entrada);

        ponteiro = 0;
        INT_LATENCY_RELOAD = Read_b_eep(EE_AJUSTE_LATENCIA);//INT_LATENCY_VALUE;
        while (rascunho >= 65535) {
            rascunho /= 2;
            //corrige entrada de interrupção
            INT_LATENCY_RELOAD >>= 1;
            ponteiro++;
        }
        switch (ponteiro) {//valor com timer1 desligado
            case 0:
                T1CON_temp = 0x31 & T1_PS_1_1;
                break;
            case 1:
                T1CON_temp = 0x31 & T1_PS_1_2;
                break;
            case 2:
                T1CON_temp = 0x31 & T1_PS_1_4;
                break;
            case 3:
                T1CON_temp = 0x31 & T1_PS_1_8;
                break;
            default:
                T1CON_temp = 0x31 & T1_PS_1_1;
                /*desliga CCP1, Veocidade mínima ultrapassada*/
                /*TODO sinalizar erro nesta condição*/
                ConfigPulseOutOff();
                break;
        }
        CCPR1_temp = (unsigned short) floor(rascunho);

        PIE1bits.CCP1IE = false;
        //verifica se saída lógica estava desligada
        if (!CCP1CON) {
            CCP1CON = 0x0F & COM_HI_MATCH;
        }
        //usar temporários diminui a latência no ajuste
        CCPR1 = CCPR1_temp;
        T1CON = T1CON_temp;
        //PIR1bits.CCP1IF = false;
        PIE1bits.CCP1IE = true;
    }
}

/*
 * swap.c --
 *
 *	Library routine for manipulating byte order.
 *
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
uint32_t endian_big_to_little(uint32_t data) {
    union {
	uint32_t l;
	unsigned char  c[4];
    } in, out;

    in.l = data;
    out.c[0] = in.c[3];
    out.c[1] = in.c[2];
    out.c[2] = in.c[1];
    out.c[3] = in.c[0];
    return out.l;
}

unsigned short endian_shortint (unsigned short shortInt)
{
    union swab {
	unsigned short s;
	unsigned char  c[2];
    } in, out;

    in.s = shortInt;
    out.c[0] = in.c[1];
    out.c[1] = in.c[0];
    return out.s;
}

/**
 * rotina para converter um valor BCD em hexadecimal correspondente
 * o valor de saída é convertido com o conteúdo em Little-endian
 *
 * @param data: valor em BCD, big-endian
 * @return long little-endian
 */
uint32_t Converter_BCD_to_HEX(uint32_t BCD){
    union {
	uint32_t l;
	unsigned char  c[4];
    } in;
    uint32_t out;
    char loop_count;

    in.l = BCD;
    out = 0;

    for (loop_count = 8; loop_count > 0; loop_count--) {
        out *= 10;
        out += (in.c[3] & 0xF0) >> 4;
        in.l <<= 4;
    }
    return out;
}

uint32_t Converter_HEX_to_BCD(uint32_t HEX) {
     union {
        uint32_t l;
        unsigned char c[4];
    } out;
    uint32_t temp;

    signed char loop_count;

    out.l = 0;

    for (loop_count = 8; HEX > 0; loop_count--) {
        temp = HEX / 10;
        out.l >>= 4;
        out.c[3] += (char) (HEX - 10 * temp) << 4;
        HEX = temp;
    }
    //preenche os bytes mais significativos com zero
    for (; loop_count > 0; loop_count--)
        out.l >>= 4;
    
    return out.l;
}

/*consultado de http://www.hackersdelight.org/divcMore.pdf*/
uint32_t divl10(uint32_t n) {
   uint32_t q, r;
   q = (n >> 1) + (n >> 2);
   q = q + (q >> 4);
   q = q + (q >> 8);
   q = q + (q >> 16);
   q = q >> 3;
   r = n - q*10;
   return q + ((r + 6) >> 4);
}
