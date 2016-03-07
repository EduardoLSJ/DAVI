/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/
#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#endif

#if defined(__XC)
#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#include <stdio.h>
#include <string.h>
#endif

//#include "user.h"
#include "interrupts.h"
#include "uart1.h"
#include "uart2.h"
#include "crc.h"
//#include "machine_main.h"
#include "state_machine.h"
#include "machine_prot_393.h"


/******************************************************************************/
/* Variables definition                                                       */
/******************************************************************************/
int mensagem_393_index; //numero de bytes recebidos
char indice_comando_393;
volatile int contador_timeout_393;
char protocol_received_caracter_393;
bool flag_mensagem_ok_393 = false;

/******************************************************************************/
/* Constant definition                                                       */
/******************************************************************************/
const char LISTA_STX_VALUE_393[] = {
    //    STX_MASTER_393, //valor do cabeçalho do PC
    STX_INSTRUMENTO_393, //A3 cabeçalho do instrumento
    //    STX_MASTER_MOTOVER_393, //A4 cabeçalho do PC ao motover
    STX_ERROR_393, //A5 cabeçalho de erro, todos dispositivos
    //    STX_MOTOVER_393 //A6 cabeçalho do motover resposta ao PC
};

const char LISTA_COMMAND_LIST_PROTOCOL_393[] = {
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
    COMMAND_RST_393 //reset do dispositivo
};

const char LISTA_N_BYTES_PROTOCOL_393 [] = {
    SIZE_ANSWER_COMMAND_NOP_393, 
    SIZE_ANSWER_COMMAND_VERSAO_393, //Leitura da versão do instrumento,
    SIZE_ANSWER_COMMAND_HASH_393, //Leitura de Hash do intervalo solicitado,
    SIZE_ANSWER_COMMAND_DI_393, //Leitura do Dispositivo Indicador
    SIZE_ANSWER_COMMAND_K_393, //Leitura da constante K
    SIZE_ANSWER_COMMAND_TI_393, //Leitura da tarifa inicial
    SIZE_ANSWER_COMMAND_TH_393, //Leitura da tarifa horária
    SIZE_ANSWER_COMMAND_TQ_393, //Leitura da Tarifa Quilometrica
    SIZE_ANSWER_COMMAND_PosC_393, //Leitura da Posicao de comando
    SIZE_ANSWER_COMMAND_MO_393, //Leitura do modo de operacao
    SIZE_ANSWER_COMMAND_RST_393, //reset do dispositivo
};

const char LISTA_N_BYTES_ERRO_PROTOCOL_393 [] = {
    2, //   TODOS OS COMANDOS
};

//Para cada estado, uma resposta de erro é atribuída
const unsigned short LISTA_RESPOSTA_ERRO_PROTOCOL_393[] = {
    ANSWER_OCUPADO_393, 
    ANSWER_NAOUSADO_393, 
    ANSWER_COMANDO_393, 
    ANSWER_QUADRO_393, 
    ANSWER_COMPRIMENTO_393, 
    ANSWER_NAOUSADO_393, 
    ANSWER_CRC_393, 
    ANSWER_ATRASO_393,
    ANSWER_NAOUSADO_393,
};

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/
bool le_mensagem_393(bool clear) {
    //chamada de outra maquina de estado.
    //verifica se há nova mensagem disponível no buffer do protocolo
    //enquanto a mensagem não for lida, a maquina protocolo responde com OCUPADO

    if (flag_mensagem_ok_393) {
        //libera a recepção de nova mensagem
        if (clear) {
            flag_mensagem_ok_393 = false;
            mensagem_393_index = 0;
        }
        //informa que há mensagem no buffer, ou que a apagou
        return true;
    }
    //informa que não há mensagem no buffer
    return false;
}

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
void receive_idle_smf_393(enum STATE_ID_PROTOCOL_393 state) {
    //não atualizar se houver evento, resultado de STF anterior
    if (event[MachineProt393Nr] == EVENT_ID_NONE_393 ||
            state == STATE_ID_IDLE_393) {
        //wait a timeout without priority
        if (contador_timeout_393 == 0)
            event[MachineProt393Nr] = EVENT_TIMEOUT_393; //E2
        //wait a received byte
        protocol_received_caracter_393 = serInchar();
        if (BufferMtRcv) {
            if (flag_mensagem_ok_393) {
                // maquina ocupada, não recebe mais mensagens
                event[MachineProt393Nr] = EVENT_OCUPADO_393;
            } else {
                mensagem_393.buffer[mensagem_393_index]
                        = protocol_received_caracter_393;
                //incrementa buffers
                mensagem_393_index++;
                event[MachineProt393Nr] = EVENT_RECEBE_BYTE_393; //E1
                contador_timeout_393 = TIMEOUT_VALUE;
            }
        }
    }
}

void receive_stx_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state,
        enum EVENT_ID_PROTOCOL_393 new_event) {
    char i;

    //try received byte
    for (i = 0; i < max_lista_stx_393_value_len; i++) {
        if (protocol_received_caracter_393 == LISTA_STX_VALUE_393[i]) {
            event[MachineProt393Nr] = EVENT_ID_NONE_393; //E0
            return;
        }
    }
    event[MachineProt393Nr] = EVENT_ERRO_393; //E3
}

void receive_command_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event) {
    char i;

    //try received byte
    for (i = 0; i < max_lista_comm_393_value_len; i++) {
        if (protocol_received_caracter_393 == LISTA_COMMAND_LIST_PROTOCOL_393[i]) {
            event[MachineProt393Nr] = EVENT_ID_NONE_393; //E0
            indice_comando_393 = i;
            return;
        }
    }
    //erro de comando inválido
    event[MachineProt393Nr] = EVENT_ERRO_393; //E3
}

void receive_format_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event) {

    //compara byte recebido com lista de referencia
    if (protocol_received_caracter_393 == FORMAT_HEX_393 ||
            protocol_received_caracter_393 == FORMAT_BCD_393) {
        event[MachineProt393Nr] = EVENT_ID_NONE_393; //E0
    } else {
        //        protocol_received_caracter_393 = ANSWER_QUADRO_393;
        event[MachineProt393Nr] = EVENT_ERRO_393; //E3
    }
}

void receive_number_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event) {

    //compara byte recebido com lista de referencia
    //se valor recebido iguala o valor definido de bytes, continue
    if (protocol_received_caracter_393 ==
            LISTA_N_BYTES_PROTOCOL_393[indice_comando_393]
            || protocol_received_caracter_393 ==
            LISTA_N_BYTES_ERRO_PROTOCOL_393[indice_comando_393]) {
        //se nbytes igual a zero, pula para cálculo de CRC
        if (protocol_received_caracter_393)
            event[MachineProt393Nr] = EVENT_ID_NONE_393; //E0
        else
            event[MachineProt393Nr] = EVENT_N_ZERO_393; //E4
    } else {
        //se valor não coincide, event = EVENT_ERRO
        event[MachineProt393Nr] = EVENT_ERRO_393; //E3
    }
}

void receive_payload_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event) {

    //se valor recebido iguala o valor definido de bytes, termina
    if ((mensagem_393_index
            - sizeof (mensagem_393.overhead)) ==
            mensagem_393.overhead.length)
        event[MachineProt393Nr] = EVENT_N_ZERO_393; //E4
    else
        //se valor não coincide, aguarda próximo evento
        event[MachineProt393Nr] = EVENT_ID_NONE_393; //E0
}

void receive_crc_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event) {
    //testa se recebeu os dois bytes do CRC
    if ((mensagem_393_index
            - sizeof (mensagem_393.overhead)
            - sizeof (unsigned short)) ==
            mensagem_393.overhead.length)
        //testa se o CRC coincide com a mensagem
        if (generate_16bit_crc(mensagem_393.buffer, mensagem_393_index,
                CRC_16)) {
            //indica erro de CRC
            event[MachineProt393Nr] = EVENT_ERRO_393;
        } else
            //CRC correto
            event[MachineProt393Nr] = EVENT_N_ZERO_393;
    else
        //aguarda próximo byte do CRC
        event[MachineProt393Nr] = EVENT_ID_NONE_393; //E0
}

void receive_message_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event) {
    //informa que a mensagem está completa
    flag_mensagem_ok_393 = true;
}

void receive_erro_stf_393(enum STATE_ID_PROTOCOL_393 cur_state,
        enum STATE_ID_PROTOCOL_393 new_state, enum EVENT_ID_PROTOCOL_393 new_event) {
    unsigned short crc;
    union protocol393_erro mensagem_erro_393;
    int i;

    if ((new_event == EVENT_ERRO_393)
            && (cur_state == STATE_STX_393)) {
        if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE)
            //repassa a mensagem recebida em STX
            ser2Outchar(mensagem_393.buffer[0]);
    }
    else {
        if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE){
            //repassa a mensagem recebida no buffer
            for (i = 0; i < mensagem_393_index; i++)
                ser2Outchar(mensagem_393.buffer[i]);
        }
        //descarta caracteres até receber o STX válido
        if (new_event == EVENT_TIMEOUT_393)
            cur_state = STATE_TIMEOUT_393;
            //    monta mensagem com o erro adequado
            //    envia mensagem de erro para o correspondente
            mensagem_erro_393.overhead.head = STX_ERROR_393; // \xA5
            mensagem_erro_393.overhead.command = COMMAND_NOP_393; // \x00
            mensagem_erro_393.overhead.format = FORMAT_HEX_393; // \x00
            mensagem_erro_393.overhead.length = sizeof
                    (LISTA_RESPOSTA_ERRO_PROTOCOL_393[0]);
            mensagem_erro_393.payload.parameter[0] =
                    *((char*) (&LISTA_RESPOSTA_ERRO_PROTOCOL_393[cur_state]) + 1);
            mensagem_erro_393.payload.parameter[1] =
                    *((char*) (&LISTA_RESPOSTA_ERRO_PROTOCOL_393[cur_state]) + 0);
            crc = generate_16bit_crc(mensagem_erro_393.buffer, 6, CRC_16);
            mensagem_erro_393.payload.parameter[sizeof
                    (LISTA_RESPOSTA_ERRO_PROTOCOL_393[0])]
                    = *((char*) (&crc) + 1);
            mensagem_erro_393.payload.parameter[sizeof
                    (LISTA_RESPOSTA_ERRO_PROTOCOL_393[0]) + 1] = *((char*) (&crc) + 0);
            serOutstring(mensagem_erro_393.buffer, 8);
        }
    //reinicia o ponteiro da mensagem
    mensagem_393_index = 0;
}
