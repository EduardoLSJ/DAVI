/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef _machine_prot_393_h
#define _machine_prot_393_h

#include "state_machine.h"

/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#define max_state_tbl_protocol393_len sizeof(state_tbl_393)/sizeof(state_tbl_393[0])

#define max_lista_stx_393_value_len sizeof(LISTA_STX_VALUE_393)
#define max_lista_comm_393_value_len sizeof(LISTA_COMMAND_LIST_PROTOCOL_393)
#define max_lista_nbytes_393_value_len sizeof(LISTA_N_BYTES_PROTOCOL_393)
/*item 3.1.6.10 do Anexo B do RTM*/
#define MO_QUILOMETRICO false
#define MO_HORARIO true
//será usado o valor maior para criar as estruturas de memória
#define BufferSizeProtocol393 255
//Caso o equipamento siga o RTM antigo, mantenha o define
//#define RTM_old

/**
 EVENTOS AVALIADOS PELA MAQUINA 393*/
typedef enum EVENT_ID_PROTOCOL_393 {
    EVENT_ID_NONE_393,
    EVENT_RECEBE_BYTE_393,
    EVENT_TIMEOUT_393,
    EVENT_ERRO_393,
    EVENT_OCUPADO_393,
    EVENT_N_ZERO_393 //payload itens = 0
};

/**
 ESTADOS DA MAQUINA DAVI*/
typedef enum STATE_ID_PROTOCOL_393 {
    STATE_ID_IDLE_393,
    STATE_STX_393,
    STATE_COMMAND_393,
    STATE_FORMAT_393,
    STATE_NUMBER_393,
    STATE_PAYLOAD_393,
    STATE_CRC_393,
    STATE_TIMEOUT_393,
};

enum STX_VALUE_PROTOCOL_393 {
    STX_MASTER_393 = '\xA2', //valor do cabeçalho do PC
    STX_INSTRUMENTO_393, //A3 cabeçalho do instrumento
    STX_MASTER_MOTOVER_393, //A4 cabeçalho do PC ao motover
    STX_ERROR_393, //A5 cabeçalho de erro, todos dispositivos
    STX_MOTOVER_393 //A6 cabeçalho do motover resposta ao PC
};

enum COMMAND_LIST_PROTOCOL_393 {
    COMMAND_NOP_393,
    COMMAND_VERSAO_393,
    COMMAND_HASH_393,
    COMMAND_DI_393, //Leitura do Dispositivo Indicador
    COMMAND_K_393, //Leitura da constante K
    COMMAND_TI_393, //Leitura da tarifa inicial
    COMMAND_TH_393, //Leitura da tarifa horária
    COMMAND_TQ_393, //Leitura da Tarifa Quilometrica
    COMMAND_PosC_393, //Leitura da Posicao de comando
    COMMAND_MO_393, //Leitura do modo de operacao
    COMMAND_RST_393 = '\x80'//reset do dispositivo
};
enum SIZE_BYTES_PROTOCOL_393 { //A2
    SIZE_COMMAND_NOP_393 = 0, //COMMAND_NOP,
    SIZE_COMMAND_VERSAO_393 = 1, //COMMAND_VERSAO,
    SIZE_COMMAND_HASH_393 = 17, //COMMAND_HASH,
    SIZE_COMMAND_DI_393 = 0, //Leitura do Dispositivo Indicador
    SIZE_COMMAND_K_393 = 0, //Leitura da constante K
    SIZE_COMMAND_TI_393 = 0, //Leitura da tarifa inicial
    SIZE_COMMAND_TH_393 = 0, //Leitura da tarifa horária
    SIZE_COMMAND_TQ_393 = 0, //Leitura da Tarifa Quilometrica
    SIZE_COMMAND_PosC_393 = 0, //Leitura da Posicao de comando
    SIZE_COMMAND_MO_393 = 0, //Leitura do modo de operacao
    SIZE_COMMAND_RST_393 = 0, //reset do dispositivo
};

enum SIZE_BYTES_ANSWER_PROTOCOL_393 { //A3
    SIZE_ANSWER_COMMAND_NOP_393 = 0, //    COMMAND_NOP,
    SIZE_ANSWER_COMMAND_VERSAO_393 = 2, //    COMMAND_VERSAO,
    SIZE_ANSWER_COMMAND_HASH_393 = 16, //    COMMAND_HASH,
    SIZE_ANSWER_COMMAND_DI_393 = 4, //    COMMAND_DI, //Leitura do Dispositivo Indicador
    SIZE_ANSWER_COMMAND_K_393 = 4, //    CCOMMAND_K, //Leitura da constante K
    SIZE_ANSWER_COMMAND_TI_393 = 4, //    COMMAND_TI, //Leitura da tarifa inicial
    SIZE_ANSWER_COMMAND_TH_393 = 4, //    COMMAND_TH, //Leitura da tarifa horária
    SIZE_ANSWER_COMMAND_TQ_393 = 4, //    COMMAND_TQ, //Leitura da Tarifa Quilometrica
    SIZE_ANSWER_COMMAND_PosC_393 = 1, //    COMMAND_PosC, //Leitura da Posicao de comando
    SIZE_ANSWER_COMMAND_MO_393 = 1, //    COMMAND_MO, //Leitura do modo de operacao
    SIZE_ANSWER_COMMAND_RST_393 = 0, //    COMMAND_RST//reset do dispositivo
};

enum FORMAT_PAYLOAD_PROTOCOL_393 {
    FORMAT_HEX_393,
    FORMAT_BCD_393
};

enum ANSWER_ERRO_PROTOCOL_393 {
    ANSWER_NAOUSADO_393,
    ANSWER_QUADRO_393,
    ANSWER_CRC_393,
    ANSWER_ATRASO_393,
    ANSWER_COMPRIMENTO_393,
    ANSWER_COMANDO_393,
    ANSWER_VALOR_393,
    ANSWER_OCUPADO_393
};

enum PosC_393 {
    PosC_LIVRE,
    PosC_OCUPADO,
    PosC_A_PAGAR = 0x80,
    PosC_MODO_VERIFICACAO
};

//estruturas para passagem de valores na transmissão de dados pela serial
union protocolRTM393 {

    struct { //cabeçalho da mensagem
        char head; //8bits
        char command; //8bits
        char format; //8bits
        char length; //8bits
    } overhead;

    struct { //parametros transmitidos
        char unused[4];
        char parameter[BufferSizeProtocol393];
    } payload;

    struct { //parametro int
        char unused[4];
        int size_int[BufferSizeProtocol393 / sizeof(int)];
    } valor_int;

    struct { //parametro LONG
        char unused[4];
        uint32_t size_32[BufferSizeProtocol393 / sizeof(long)];
    } valor;

    char buffer[(BufferSizeProtocol393 + 4) * sizeof (char) + sizeof (unsigned short) ];
};

union protocolRTM393_old {

    struct { //cabeçalho da mensagem
        char head; //8bits
        char command; //8bits
        char length; //8bits
    } overhead;

    struct { //parametros transmitidos
        char unused[3];
        char parameter[BufferSizeProtocol393];
    } payload;

    struct { //parametro int
        char unused[4];
        int size_int[BufferSizeProtocol393 / sizeof(int)];
    } valor_int;

    struct { //parametro LONG
        char unused[4];
        uint32_t size_32[BufferSizeProtocol393 / sizeof(long)];
    } valor;

    char buffer[(BufferSizeProtocol393 + 4) * sizeof (char) + sizeof (unsigned short) ];
};

/**
 * estrutura da mensagem para transmissao de erros.
 */
union protocol393_erro {

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

/******************************************************************************/
/* Variables definition                                                       */
/******************************************************************************/
extern int mensagem_393_index; //numero de bytes recebidos
extern volatile int contador_timeout_393;
#ifdef RTM393_old
//union protocolRTM393_old mensagem_PC; //recebe mensagem do PC
union protocolRTM393_old mensagem_393; //recebe mensagem do instrumento
#else
//union protocolRTM393 mensagem_PC; //recebe mensagem do PC
union protocolRTM393 mensagem_393; //recebe mensagem do instrumento
#endif

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
bool le_mensagem_393(bool clear);
void receive_idle_smf_393(enum STATE_ID_PROTOCOL_393 state);
void receive_stx_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event);
void receive_command_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event);
void receive_format_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event);
void receive_number_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event);
void receive_payload_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event);
void receive_crc_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event);
void receive_message_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event);
void receive_timeout_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event);
void receive_erro_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event);

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
const struct STATE_DESCRIPTION
state_tbl_393[] = {
    /*eventos lineares na maquina de estados */
        {
            STATE_ID_IDLE_393,
            STATE_STX_393,
            EVENT_RECEBE_BYTE_393,
            receive_stx_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_ID_IDLE_393,
            STATE_ID_IDLE_393,
            EVENT_OCUPADO_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_STX_393,
            STATE_COMMAND_393,
            EVENT_RECEBE_BYTE_393,
            receive_command_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_COMMAND_393,
            STATE_FORMAT_393,
            EVENT_RECEBE_BYTE_393,
            receive_format_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_FORMAT_393,
            STATE_NUMBER_393,
            EVENT_RECEBE_BYTE_393,
            receive_number_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_NUMBER_393,
            STATE_PAYLOAD_393,
            EVENT_RECEBE_BYTE_393,
            receive_payload_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_PAYLOAD_393,
            STATE_CRC_393,
            EVENT_N_ZERO_393, //E4, final do payload
            receive_crc_stf_393,
            receive_idle_smf_393
        },
        /*evento de salto para payload zero*/
        {
            STATE_NUMBER_393,
            STATE_CRC_393,
            EVENT_N_ZERO_393, //E4, numero de bytes = 0
            receive_crc_stf_393,
            receive_idle_smf_393
        },
        /*evento de espera do payload bytes*/
        {
            STATE_PAYLOAD_393,
            STATE_PAYLOAD_393,
            EVENT_RECEBE_BYTE_393,
            receive_payload_stf_393,
            receive_idle_smf_393
        },
        /*evento espera de CRC bytes*/
        {
            STATE_CRC_393,
            STATE_CRC_393,
            EVENT_RECEBE_BYTE_393,
            receive_crc_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_CRC_393,
            STATE_ID_IDLE_393,
            EVENT_N_ZERO_393, //E4, final do CRC valido
            receive_message_stf_393,
            receive_idle_smf_393
        },
        /*evento de timeout*/
        {
            STATE_STX_393,
            STATE_ID_IDLE_393,
            EVENT_TIMEOUT_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_COMMAND_393,
            STATE_ID_IDLE_393,
            EVENT_TIMEOUT_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_FORMAT_393,
            STATE_ID_IDLE_393,
            EVENT_TIMEOUT_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_NUMBER_393,
            STATE_ID_IDLE_393,
            EVENT_TIMEOUT_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_PAYLOAD_393,
            STATE_ID_IDLE_393,
            EVENT_TIMEOUT_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_CRC_393,
            STATE_ID_IDLE_393,
            EVENT_TIMEOUT_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        /*evento de erro*/
        {
            STATE_STX_393,
            STATE_ID_IDLE_393,
            EVENT_ERRO_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_COMMAND_393,
            STATE_ID_IDLE_393,
            EVENT_ERRO_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_FORMAT_393,
            STATE_ID_IDLE_393,
            EVENT_ERRO_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_NUMBER_393,
            STATE_ID_IDLE_393,
            EVENT_ERRO_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        },
        {
            STATE_CRC_393,
            STATE_ID_IDLE_393,
            EVENT_ERRO_393,
            receive_erro_stf_393,
            receive_idle_smf_393
        }
};
#endif /* _machine_prot_393_h */
