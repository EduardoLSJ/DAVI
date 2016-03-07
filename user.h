
#ifndef _USER_H
#define _USER_H

#if defined(__XC)
#include <stdbool.h>       /* For true/false definition */
#include "system.h"
#include "md5.h"
#include <stdint.h>
#endif

/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
/*Endereço das constantes gravadas na ROM, HERDADOS DO BOOTLOADER*/
/**HASH_BOOT: Hash da área de BOOT 0x0000 a 0x07EF (16bytes)*/
#define _BOOT_ROM_HASH_LOCATION 0x07F0

/*endereço das constantes gravadas na ROM*/
/**Endereço de inicio do programa principal*/
#define _DAVI_APP_START 0x0800 
/**Endereço com a Versão de software do DAVI (2bytes)*/
#define _DAVI_VERSION_LOCATION 0x7FDA
/**Aponta para o Endereço inicial de COMM_HELPER (2bytes)*/
#define _COMM_ROM_INIT_LOCATION 0x7FDC //0x07C0 
/**Endereço com tamanho da função COMM_HELPER (2bytes)*/
#define _COMM_ROM_SIZE_LOCATION 0x7FDE //0x07D0 
/**HASH_COMM: Hash da área de comunicação de erro (16bytes)*/
#define _COMM_ROM_HASH_LOCATION 0x7FE0 
/**Endereço com início do HASH do programa principal*/
#define _DAVI_ROM_HASH_LOCATION 0x7FF0

/**Versão atual do DAVI*/
#define _DAVI_VERSION_VALUE {0x04CE}
/**Endereço de inicio da funcao _comm_helper.
 *  Usar MACRO ou determinar após compilaçã, no arquivo .map*/
#define _COMM_ROM_INIT_VALUE (uint16_t) 0x4FFE
/**Tamanho da função _comm_helper.
 *  Usar MACRO ou determinar após compilação, no arquivo .map*/
#define _COMM_ROM_SIZE_VALUE (uint16_t) 0x5128 - (uint16_t) 0x4FFE
/**Hash MD5 da memória FLASH comm_helper*/
#define _COMM_ROM_HASH_VALUE {0xFD,0x5A,0xAC,0x13,0x81,0x0F,0xBF,0x44,0xBC,0xC8,0x60,0x58,0x54,0xFB,0x15,0x7A}
//0xC0,0xA0,0x30,0xCB,0xFC,0x02,0x8F,0xBC,\
//            0x04,0xAC,0x1A,0x4D,0x9D,0x7C,0x8E,0xE0} //calcular após compilação
 /**Hash da memória ROM 0x800 a 0x7FEF*/
#define _DAVI_ROM_HASH_VALUE {0x58,0xD3,0x71,0x93,0x80,0xCB,0xA0,0xAD,0xEC,0x7D,0xBE,0x70,0xE0,0xB3,0x4F,0xA8} //calcular após compilação
/*Endereço das constantes gravadas na EEPROM*/
#define EE_AJUSTE_LATENCIA  0 /**endereço da constante para correção do gerador de frequencia*/
#define EE_AJUSTE_TEMPO 1 /**endereço da constante para correção do contador de tempo*/
#define EE_NUM_SERIE 2 
#define EE_HASH_DADOS 0x10
#define EE_FREE_INICIO 0x20
#define EE_HASH_FREE_EEPROM 0xF0

#define FALHA_INEXISTENTE ANSWER_NAOUSADO_DAVI //0x00
#define FALHA_HASH_EEPROM_FREE ANSWER_ERRO_HASH_EEPROM_FREE //0x11
#define FALHA_DADOS_EEPROM ANSWER_ERRO_DADOS_EEPROM //0x12
#define FALHA_APP_ROM ANSWER_ERRO_APP_ROM //0x13
#define FALHA_BOOT_ROM ANSWER_ERRO_BOOT_ROM //0x14
#define FALHA_COMM_HELPER ANSWER_ERRO_COMM_HELPER //0x15



// Definition of hardware version
#define RevA 1 //hardware do "Ready for PIC" + "easy BlueTooth"
#define RevB 2 //hardware do MOTOVER + HC-05
#define RevC 3 //hardware do DAVI + RN41

// Current revision is:
//#define MOTOVER  RevA
//#define MOTOVER  RevB
#define MOTOVER  RevC

//usart1 pins
//#define TurnOnUSART1() {TRISCbits.TRISC7 = 1; TRISCbits.TRISC6 = 1;}

// PULSE_IN pin
#define PULSE_IN_LAT LATCbits.LATC1
#define PULSE_IN_PORT PORTCbits.CCP2
#define PULSE_IN_TRIS TRISCbits.TRISC1
#define TurnOnPulseIn() {PULSE_IN_LAT = 1; PULSE_IN_TRIS = 1;}

//PULSE_OUT pin
#define PULSE_OUT_LAT LATCbits.LATC2
#define PULSE_OUT_PORT PORTCbits.CCP1
#define PULSE_OUT_TRIS TRISCbits.TRISC2
#define TurnOnPulseOut() {PULSE_OUT_LAT = 0; PULSE_OUT_TRIS = 0;}
#define TurnOffPulseOut() {PULSE_OUT_LAT = 1; PULSE_OUT_TRIS = 0;}

// LEDs
#define LEDs_LAT    LATC
#define LEDs_TRIS   TRISC
#define LEDs_BTRIS  TRISB
#define LED0        LATCbits.LATC0
#define LED2        LATBbits.LATB2
#define LED3        LATBbits.LATB3
#define LED0_TRIS   TRISCbits.TRISC0
#define LED2_TRIS   TRISBbits.TRISB2
#define LED3_TRIS   TRISBbits.TRISB3
#define TurnOffLED0() {LED0 = 0;LED2 = 0;LED3 = 0; LED0_TRIS = 0;LED2_TRIS = 0;LED3_TRIS = 0;}
#define TurnOnLED0() {LED0 = 1; LED2 = 1;LED3 = 1;LED0_TRIS = 0;LED2_TRIS = 0;LED3_TRIS = 0;}

#define PR2_VALUE 250
#define TMR2_PoS 10
#define TMR2_PrS 16
#define TIMEOUT_SECONDS 5 /**timeout de 5 segundos, sem receber resposta do instrumento*/
#define TIMEOUT_MINUTES 4 /**valor do timeout em minutos, sem receber comandos do PC*/

#define TIMEOUT_VALUE TIMEOUT_SECONDS*(FCY/(long)(PR2_VALUE*TMR2_PoS))/TMR2_PrS
#define TIMEOUT_PC_VALUE TIMEOUT_MINUTES*60*(FCY/(long)(PR2_VALUE*TMR2_PoS))/TMR2_PrS

#define debounce_edge_time 3500 /**tempo de filtro para entrada de pulsos*/

#define ascendente true
#define descendente false

typedef struct {
    unsigned MensagemModo:1; /**fluxo de mensagens: transparente ou bloqueado*/
    unsigned ModoOpr:1; /**modo quilometrico ou Horario*/
    unsigned PulsoPend:1; /**indica pulso pendente*/
    unsigned DistPend:1; /**Indica que distancia foi percorrida*/
    unsigned Erro:1; /**indica erro durante ensaio*/
    unsigned NovaMensagem:1; /**indica nova mensagem a transmitir para o instrumento*/
    unsigned Parametro:1; /**indica novo parâmetro disponível (leitura do DI)*/
    unsigned ParametroFracao:1; /**indica que o parâmetro disponível corresponde à fração*/
}flags_struct_ensaio;
    
/******************************************************************************/
/* User Function Prototypes                                                   */
/******************************************************************************/
extern flags_struct_ensaio flags_ensaio;
extern const uint16_t DAVI_VERSAO_ATUAL;
/**erro encontrado durante verificação de integridade
*/
int8_t causa_erro_inicializacao;

/**
 * I/O and Peripheral Initialization
 */
void InitApp(void);         /* I/O and Peripheral Initialization */
/*
  Function:
    void IntegridadeGlobal(void);

  Summary:
    Verifica a integridade de todos os blocos de programa e armazenamento.

  Description:
    Esta rotina solicita a o cálculo de hash MD5 e caso haja erro, toma as ações
  possíveis para avisar ao usuário. No pior caso o sistema é bloqueado.
    As mensagens de comuicação estão a cargo do bloco de bootloader, através da 
 função Comm_Helper
    
  Precondition:
    Nenhuma.

  Parameters:
    Nenhum parâmetro é necessário.

  Returns:
    Nada.

  Example:
    <code>
    IntegridadeGlobal();
    </code>

  Remarks:
    Nenhum.
*/
void IntegridadeGlobal(void);

/*
  Function:
    int8_t IntegridadeRom(MD5_CONTEXT* context, uint16_t ini, uint16_t len, uint8_t* result);

  Summary:
    Calcula hash MD5 do intervalo de memória FLASH informado, 
  compara o resultado com o hash informado e caso coincidam, retorna true.

  Description:
    Esta rotina armazena o valor result para comparação com o Hash calculado do
 * intervalo solicitado. Em caso de coincidência é retornado o valor true.
 * Result é destruído para receber o valor hash calculado.
    
  Precondition:
    Os parâmetros precisam ser inicializados. Em caso de conteúdo result vazio a
 * função retorna false, que deve ser desconsiderada.

  Parameters:
    @param context - contexto de trabalho do MD5
    @param ini - endereço inicial do bloco de memória
    @param len - comprimento do bloco de memória
    @param result - hash de referência.
  
  Returns:
    @param result - retornará com o resultado do Hash calculado (16 bytes).
    @return - informa true/false se o hash calculado coincidir com o informado

  Example:
    <code>
    uint8_t result[16] = {0,1,2,3,4,5,6,7,8\
                        9,10,11,12,13,14,15};
    uint16_t ini = 0x0000;
    uint16_t len = 0x8000;

    MD5_CONTEXT context;

    if( IntegridadeRom(&context, ini, len, result)){
            be_happy();
    else
           worry_about();
    }
    </code>

  Remarks:
    None.
*/
//int8_t IntegridadeRom(MD5_CONTEXT* context, uint16_t ini, uint16_t len, uint8_t* result);
int8_t IntegridadeRom(uint16_t ini, uint16_t len, uint8_t* result);

/*
  Function:
    int8_t IntegridadeEEPROM(MD5_CONTEXT* context, uint16_t ini, uint16_t fim, uint8_t* result);

  Summary:
    Calcula hash MD5 do intervalo de memória EEPROM informado, 
  compara o resultado com o hash informado e caso coincidam, retorna true.

  Description:
    Esta rotina armazena o valor result para comparação com o Hash calculado do
 * intervalo solicitado. Em caso de coincidência é retornado o valor true.
 * Result é destruído para receber o valor hash calculado.
    
  Precondition:
    Os parâmetros precisam ser inicializados. Em caso de conteúdo result vazio a
 * função retorna false, que deve ser desconsiderada.

  Parameters:
    @param context - contexto de trabalho do MD5
    @param ini - endereço inicial do bloco de memória
    @param fim - endereço final do bloco de memória
    @param result - hash de referência.

  Returns:
    @param result - retornará com o resultado do Hash calculado (16 bytes).
    @return - informa true/false se o hash calculado coincidir com o informado

  Example:
    <code>
    uint8_t result[16] = {0,1,2,3,4,5,6,7,8\
                        9,10,11,12,13,14,15};
    uint8_t ini = 0x00;
    uint8_t fim = 0xFF;

    MD5_CONTEXT context;

    if( IntegridadeEEPROM(&context, ini, fim, result)){
            be_happy();
    else
           worry_about();
    }
    </code>

  Remarks:
    None.
*/
//int8_t IntegridadeEEPROM(MD5_CONTEXT* context, uint8_t ini, uint8_t fim, uint8_t* result);
int8_t IntegridadeEEPROM(uint8_t ini, uint8_t fim, uint8_t* result);

//extern void comm_helper(uint8_t) __at(_COMM_ROM_INIT_LOCATION);
/**
 * Auxiliar na transmissão de código de falhas
 * 
 * Configura a USART em software(autobaudrate)
 * recebe o valor do erro detectado pelo programa principal
 * recebe qualquer mensagem do PC
 * a cada mensagem recebida, transmite a mensagem de erro codificada
 * 
 * @param erro_detectado - código do erro detectado pelo programa principal
 */
void comm_helper(uint8_t erro_detectado);
void Delay_ms(uint32_t atraso);
void ConfigPulseIn(bool enable);
void ConfigPulseOutOff(void);
void InitPulseCounter(void);
void InitTimeoutCounter(void);
uint32_t endian_big_to_little(uint32_t data);
unsigned short endian_shortint (unsigned short shortInt);
void gera_velocidade(char *vel, uint16_t *k);
uint32_t Converter_BCD_to_HEX(uint32_t BCD);
uint32_t Converter_HEX_to_BCD(uint32_t HEX);
uint32_t divl10(uint32_t n);

#endif /* _USER_H */
