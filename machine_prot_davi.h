/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef _machine_prot_davi_h
#define _machine_prot_davi_h

#include <stdbool.h>
#include "state_machine.h"

/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#define max_state_tbl_davi_len sizeof(state_tbl_davi)/sizeof(state_tbl_davi[0])

#define max_lista_stx_davi_value_len sizeof(LISTA_STX_VALUE_DAVI)
#define max_lista_comm_davi_value_len sizeof(LISTA_COMMAND_LIST_DAVI)
#define max_lista_nbytes_davi_value_len sizeof(LISTA_N_BYTES_DAVI)
//será usado o valor maior para criar as estruturas de memória
#define BufferSizeDavi 255

//Caso o equipamento siga o RTM antigo, mantenha o define
//#define DAVI_old

/**
 EVENTOS AVALIADOS PELA MAQUINA DAVI*/
typedef enum EVENT_ID_PROTOCOL_DAVI {
    EVENT_ID_NONE_DAVI,
    EVENT_RECEBE_BYTE_DAVI,
    EVENT_TIMEOUT_DAVI,
    EVENT_ERRO_DAVI,
    EVENT_OCUPADO_DAVI,
    EVENT_N_ZERO_DAVI,
};

/**
 ESTADOS DA MAQUINA DAVI*/
typedef enum STATE_ID_PROTOCOL_DAVI {
    STATE_IDLE_DAVI,
    STATE_STX_DAVI,
    STATE_COMMAND_DAVI,
    STATE_FORMAT_DAVI,
    STATE_NUMBER_DAVI,
    STATE_PAYLOAD_DAVI,
    STATE_CRC_DAVI,
    STATE_TIMEOUT_DAVI,
};

enum STX_VALUE_DAVI {
    STX_MASTER_DAVI = '\xA2', //valor do cabeçalho do PC
    STX_INSTRUMENTO_DAVI, //A3 cabeçalho do instrumento
    STX_MASTER_MOTOVER_DAVI, //A4 cabeçalho do PC ao motover
    STX_ERROR_DAVI, //A5 cabeçalho de erro, todos dispositivos
    STX_MOTOVER_DAVI //A6 cabeçalho do motover resposta ao PC
};

/**
 * comandos disponíveis para o PC encaminhar para o DAVI
 * serão três ensaios disponíveis:
 * ensaio de quilométrico
 * ensaio horário
 * levantamento de W do veículo
 *
 * o comando de abortar encerra qualquer processo em andamento
 * o comando NOP mantém o PC ciente que o DAVI está com a tarefa em execução
 */
enum COMMAND_LIST_DAVI {//A4 , A5 e A6
    COMMAND_NOP_DAVI,
    COMMAND_VERSAO_DAVI,
    COMMAND_HASH_DAVI,
    COMMAND_QUILO_DAVI = '\x81',
    COMMAND_HORARIO_DAVI,
    COMMAND_ABORTAR_DAVI,
    COMMAND_LEVANTA_W_DAVI,
    COMMAND_CALIBRACAO_DAVI,
    COMMAND_AJUSTE_DAVI,
    COMMAND_NUMERO_SERIE
};

enum SIZE_BYTES_DAVI { //A4
    SIZE_NOP_DAVI = 0,
    SIZE_COMMAND_VERSAO_DAVI = 1, //  Identificador 1 byte
    SIZE_COMMAND_HASH_DAVI = 17, // end inicial 16 bytes, end final 16 bytes
    SIZE_PC_QUILO = 8,//distancia 4,velocidade 1,k 2,vt 1,
    SIZE_PC_HORARIO = 2, //N_frações 2bytes
    SIZE_PC_ABORTAR = 0,
    SIZE_PC_LEVANTA_W = 4, //distancia 2byte (valor Hexa em mm), k 2byte
    SIZE_COMMAND_CALIBRACAO_DAVI = 3, //velocidade 1byte, k 2byte
    SIZE_COMMAND_AJUSTE_DAVI = 7,
    SIZE_COMMAND_NUMERO_SERIE = 0
};

enum SIZE_BYTES_ANSWER_DAVI { //A6
    SIZE_ANSWER_NOP_DAVI = 0,
    SIZE_ANSWER_COMMAND_VERSAO_DAVI = 2, //    COMMAND_VERSAO,
    SIZE_ANSWER_COMMAND_HASH_DAVI = 16, //    COMMAND_HASH,
    SIZE_ANSWER_PC_QUILO = 18,//indicacao ini 4, indicacao fim 4, ind_fracao 4, D_FINAL 4,v_atual 1,vt 1
    SIZE_ANSWER_PC_HORARIO = 14,//tempo 4, ind_ini 4, ind_FIM 4, N frações calculada 2Bytes
    SIZE_ANSWER_PC_ABORTAR = 0,
    SIZE_ANSWER_PC_LEVANTA_W = 4,//n pulsos 4
    SIZE_ANSWER_COMMAND_CALIBRACAO_DAVI = 12, //tempo medido 4, numero pulsos no intervalo 4, intervalos 4/
    SIZE_ANSWER_COMMAND_AJUSTE_DAVI = 16, /*TODO determinar tamanho */
    SIZE_ANSWER_ERRO_DAVI = 2, //identificador do erro 2bytes
    SIZE_ANSWER_COMMAND_NUMERO_SERIE = 2, //número de série do DAVI
};

enum FORMAT_PAYLOAD_DAVI {
    FORMAT_HEX_DAVI,
    FORMAT_BCD_DAVI
};

enum ANSWER_ERRO_DAVI {
    ANSWER_NAOUSADO_DAVI,
    ANSWER_QUADRO_DAVI,
    ANSWER_CRC_DAVI,
    ANSWER_ATRASO_DAVI,
    ANSWER_COMPRIMENTO_DAVI,
    ANSWER_COMANDO_DAVI,
    ANSWER_VALOR_DAVI,
    ANSWER_OCUPADO_DAVI,
};

//estruturas para passagem de valores na transmissão de dados pela serial
union protocolDAVI {

    struct { //cabeçalho da mensagem
        char head; //8bits
        char command; //8bits
        char format; //8bits
        char length; //8bits
    } overhead;

    struct { //parametros transmitidos
        char unused[4];
        char parameter[BufferSizeDavi];
    } payload;

    struct { //parametro int
        char unused[4];
        int size_int[BufferSizeDavi / sizeof(int)];
    } valor_int;

    struct { //parametro LONG
        char unused[4];
        uint32_t size_32[BufferSizeDavi / sizeof(long)];
    } valor_long;

    char buffer[(BufferSizeDavi + 4) * sizeof (char) + sizeof (unsigned short) ];
};

union protocolDAVI_old {

    struct { //cabeçalho da mensagem
        char head; //8bits
        char command; //8bits
        char length; //8bits
    } overhead;

    struct { //parametros transmitidos
        char unused[3];
        char parameter[BufferSizeDavi];
    } payload;

    struct { //parametro int
        char unused[4];
        int size_int[BufferSizeDavi / sizeof(int)];
    } valor_int;

    struct { //parametro LONG
        char unused[4];
        uint32_t size_32[BufferSizeDavi / sizeof(long)];
    } valor;

    char buffer[(BufferSizeDavi + 4) * sizeof (char) + sizeof (unsigned short) ];
};

/**
 * estrutura da mensagem para transmissao de erros.
 */
union protocolDAVI_erro {

    struct { //cabeçalho da mensagem
        char head; //8bits
        char command; //8bits
        char format; //8bits
        char length; //8bits
    } overhead;

    struct { //parametros transmitidos
        char unused[4];
        char parameter[2];
    } payload;

    char buffer[(2 + 4) * sizeof (char) + sizeof (unsigned short) ];
};

#ifdef DAVI_old
union protocolDAVI_old mensagem_Davi; //recebe mensagem do PC
#else
union protocolDAVI mensagem_Davi; //recebe mensagem do PC
#endif
extern volatile int contador_timeout_davi;

// Comment a function and leverage automatic documentation with slash star star
/**
    <p><b>Function prototype:</b></p>
  
    <p><b>Summary:</b></p>

    <p><b>Description:</b></p>

    <p><b>Precondition:</b></p>

    <p><b>Parameters:</b></p>

    <p><b>Returns:</b></p>

    <p><b>Example:</b></p>
    <code>
 
    </code>

    <p><b>Remarks:</b></p>
 */
bool le_mensagem_davi(bool clear);
void receive_idle_smf_davi(enum STATE_ID_PROTOCOL_DAVI state);
void receive_stx_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI new_event);
void receive_command_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI new_event);
void receive_format_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI new_event);
void receive_number_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI new_event);
void receive_payload_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI new_event);
void receive_crc_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI new_event);
void receive_message_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI new_event);
void receive_timeout_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI new_event);
void receive_erro_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI new_event);

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
const struct STATE_DESCRIPTION
state_tbl_davi[] = {
    //eventos lineares na maquina de estados
        {
            STATE_ID_IDLE,
            STATE_STX_DAVI,
            EVENT_RECEBE_BYTE_DAVI,
            receive_stx_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_ID_IDLE,
            STATE_ID_IDLE,
            EVENT_OCUPADO_DAVI,
            receive_erro_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_STX_DAVI,
            STATE_COMMAND_DAVI,
            EVENT_RECEBE_BYTE_DAVI,
            receive_command_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_COMMAND_DAVI,
            STATE_FORMAT_DAVI,
            EVENT_RECEBE_BYTE_DAVI,
            receive_format_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_FORMAT_DAVI,
            STATE_NUMBER_DAVI,
            EVENT_RECEBE_BYTE_DAVI,
            receive_number_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_NUMBER_DAVI,
            STATE_PAYLOAD_DAVI,
            EVENT_RECEBE_BYTE_DAVI,
            receive_payload_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_PAYLOAD_DAVI,
            STATE_CRC_DAVI,
            EVENT_N_ZERO_DAVI, //E4, final do payload
            receive_crc_stf_davi,
            receive_idle_smf_davi
        },
        //evento de salto para payload zero
        {
            STATE_NUMBER_DAVI,
            STATE_CRC_DAVI,
            EVENT_N_ZERO_DAVI, //E4, numero de bytes = 0
            receive_crc_stf_davi,
            receive_idle_smf_davi
        },
        //evento de espera do payload bytes
        {
            STATE_PAYLOAD_DAVI,
            STATE_PAYLOAD_DAVI,
            EVENT_RECEBE_BYTE_DAVI,
            receive_payload_stf_davi,
            receive_idle_smf_davi
        },
        //evento espera de CRC bytes
        {
            STATE_CRC_DAVI,
            STATE_CRC_DAVI,
            EVENT_RECEBE_BYTE_DAVI,
            receive_crc_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_CRC_DAVI,
            STATE_ID_IDLE,
            EVENT_N_ZERO_DAVI, //E4, final do CRC valido
            receive_message_stf_davi,
            receive_idle_smf_davi
        },
        //evento de timeout
        {
            STATE_STX_DAVI,
            STATE_ID_IDLE,//STATE_TIMEOUT_DAVI,
            EVENT_TIMEOUT_DAVI,
            receive_erro_stf_davi,//receive_timeout_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_COMMAND_DAVI,
            STATE_ID_IDLE,//STATE_TIMEOUT_DAVI,
            EVENT_TIMEOUT_DAVI,
            receive_erro_stf_davi,//receive_timeout_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_FORMAT_DAVI,
            STATE_ID_IDLE,//STATE_TIMEOUT_DAVI,
            EVENT_TIMEOUT_DAVI,
            receive_erro_stf_davi,//receive_timeout_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_NUMBER_DAVI,
            STATE_ID_IDLE,//STATE_TIMEOUT_DAVI,
            EVENT_TIMEOUT_DAVI,
            receive_erro_stf_davi,//receive_timeout_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_PAYLOAD_DAVI,
            STATE_ID_IDLE,//STATE_TIMEOUT_DAVI,
            EVENT_TIMEOUT_DAVI,
            receive_erro_stf_davi,//receive_timeout_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_CRC_DAVI,
            STATE_ID_IDLE,//STATE_TIMEOUT_DAVI,
            EVENT_TIMEOUT_DAVI,
            receive_erro_stf_davi,//receive_timeout_stf_davi,
            receive_idle_smf_davi
        },
        //evento de erro
        {
            STATE_STX_DAVI,
            STATE_ID_IDLE,
            EVENT_ERRO_DAVI,
            receive_erro_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_COMMAND_DAVI,
            STATE_ID_IDLE,
            EVENT_ERRO_DAVI,
            receive_erro_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_FORMAT_DAVI,
            STATE_ID_IDLE,
            EVENT_ERRO_DAVI,
            receive_erro_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_NUMBER_DAVI,
            STATE_ID_IDLE,
            EVENT_ERRO_DAVI,
            receive_erro_stf_davi,
            receive_idle_smf_davi
        },
        {
            STATE_CRC_DAVI,
            STATE_ID_IDLE,
            EVENT_ERRO_DAVI,
            receive_erro_stf_davi,
            receive_idle_smf_davi
        }
};
#endif	/* _machine_prot_davi_h */
