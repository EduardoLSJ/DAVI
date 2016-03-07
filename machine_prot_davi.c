/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/
#if defined(__XC)
#include <xc.h> /* XC8 General Include File */
#endif

#if defined(__XC)
#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#include <stdio.h>
#include <string.h>
#endif

#include "user.h"
#include "uart1.h"
#include "uart2.h"
#include "crc.h"
#include "state_machine.h"
#include "machine_main.h"
#include "machine_prot_davi.h"

/******************************************************************************/
/* Variables definition                                                       */
/******************************************************************************/
static int mensagem_davi_index;
char indice_comando_davi;
volatile int contador_timeout_davi;
char protocol_received_caracter_davi;
bool flag_mensagem_ok_davi = false;

/******************************************************************************/
/* Constant definition                                                       */
/******************************************************************************/
const char LISTA_STX_VALUE_DAVI[] = {
    //STX_MASTER_DAVI, //valor do cabeçalho do PC
    //STX_INSTRUMENTO_DAVI, //A3 cabeçalho do instrumento
    STX_MASTER_MOTOVER_DAVI, //A4 cabeçalho do PC ao motover
    //STX_ERROR_DAVI, //A5 cabeçalho de erro, todos dispositivos
    //STX_MOTOVER_DAVI //A6 cabeçalho do motover resposta ao PC
};

const char LISTA_COMMAND_LIST_DAVI[] = { //A4
    COMMAND_NOP_DAVI,
    COMMAND_VERSAO_DAVI,
    COMMAND_HASH_DAVI,
    COMMAND_QUILO_DAVI,
    COMMAND_HORARIO_DAVI,
    COMMAND_ABORTAR_DAVI,
    COMMAND_LEVANTA_W_DAVI,
    COMMAND_CALIBRACAO_DAVI,
    COMMAND_AJUSTE_DAVI,
    COMMAND_NUMERO_SERIE
};

const char LISTA_N_BYTES_DAVI [] = { //A4
    SIZE_NOP_DAVI, 
    SIZE_COMMAND_VERSAO_DAVI, //    COMMAND_VERSAO,
    SIZE_COMMAND_HASH_DAVI, // end inicial 16 bytes, end final 16 bytes
    SIZE_PC_QUILO, 
    SIZE_PC_HORARIO,
    SIZE_PC_ABORTAR,
    SIZE_PC_LEVANTA_W,
    SIZE_COMMAND_CALIBRACAO_DAVI,
    SIZE_COMMAND_AJUSTE_DAVI,
    SIZE_COMMAND_NUMERO_SERIE
};

const char LISTA_N_BYTES_ERRO_PROTOCOL_DAVI [] = { //A5
    2, //   TODOS OS COMANDOS
};

//Para cada estado, uma resposta de erro é atribuída
const unsigned short LISTA_RESPOSTA_ERRO_PROTOCOL_DAVI [] = {
    ANSWER_OCUPADO_DAVI, //STATE_ID_IDLE_393,
    ANSWER_NAOUSADO_DAVI, //STATE_STX_393,
    ANSWER_COMANDO_DAVI, //STATE_COMMAND_393,
    ANSWER_QUADRO_DAVI, //STATE_FORMAT_393,
    ANSWER_COMPRIMENTO_DAVI, //STATE_NUMBER_393,
    ANSWER_NAOUSADO_DAVI, //STATE_PAYLOAD_393,
    ANSWER_CRC_DAVI, //STATE_CRC_393,
    ANSWER_ATRASO_DAVI, //STATE_TIMEOUT_393,
    ANSWER_NAOUSADO_DAVI, //STATE_ERRO_393
};

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/
/**
 *  Função de interface com a camada superior, que informa se uma nova mensagem
 * está disponível no buffer.
 *  Enquanto a camada de aplicação não limpar o buffer, todas as mensagens novas
 * serão descartadas e receberão uma mensagem de erro indicando o estado OCUPADO
 *
 * @param clear parâmetro que limpa o buffer de entrada e habilita nova mensagem
 * @return retorna a indicação de nova mensagem armazenada em buffer
 */
bool le_mensagem_davi(bool clear) {
    if (flag_mensagem_ok_davi) {
        //libera a recepção de nova mensagem
        if (clear) {
            flag_mensagem_ok_davi = false;
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

/**
 * Estado de monitoração de bytes provenientes da máquina de mensagens com o PC.
 *
 * monitora a entrada de novo byte e informa para o estado atual.
 * caso tenha uma mensagem armazenada:
 *   descarte os bytes recebidos.
 *   se compor uma mensagem válida, responde com ocupado.
 *   A mensagem válida tem o header com intervalo de tempo de pelo menos 3 bytes
 * de atraso da última mensagem.
 * caso ultrapasse o tempo de espera entre bytes, passa para estado de erro
 * 
 * @param state
 */
void receive_idle_smf_davi(enum STATE_ID_PROTOCOL_DAVI state) {
    //não atualizar se houver evento, resultado de STF anterior
    if (event[MachineProtDaviNr] == EVENT_ID_NONE_DAVI ||
            state == STATE_ID_IDLE) {
        //wait an timeout with priority
        if (contador_timeout_davi == 0)
            event[MachineProtDaviNr] = EVENT_TIMEOUT_DAVI; //E2
        //test a received byte that could mask a timeout event
        protocol_received_caracter_davi = ser2Inchar();
        if (BufferPcRcv) {
            if (flag_mensagem_ok_davi) {
                // maquina ocupada, não recebe mais mensagens
                /*tratar mensagem: descartar tudo até receber
                 mensagem válida. Ao menos testar STX valido com a 
                 função receive_stx_stf_davi*/
                event[MachineProtDaviNr] = EVENT_OCUPADO_DAVI;
            } else {
                mensagem_Davi.buffer[mensagem_davi_index]
                        = protocol_received_caracter_davi;
                //incrementa buffers
                mensagem_davi_index++;
                event[MachineProtDaviNr] = EVENT_RECEBE_BYTE_DAVI; //E1
                contador_timeout_davi = TIMEOUT_VALUE;
            }
        }
    }
}

/**
 *  Aguarda a recepção de um valor de cabeçalho válido e passa para próximo es-
 * tado da máquina.
 *
 *  Se o byte recebido não for válido, solicita mensagem de erro.
 * onde será tratado como erro ou como modo transparente
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void receive_stx_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state,
        enum EVENT_ID_PROTOCOL_DAVI new_event) {
    char i;

    //try received byte
    for (i = 0; i < max_lista_stx_davi_value_len; i++) {
        if (protocol_received_caracter_davi == LISTA_STX_VALUE_DAVI[i]) {
            event[MachineProtDaviNr] = EVENT_ID_NONE_DAVI; //E0
            return;
        }
    }
    event[MachineProtDaviNr] = EVENT_ERRO_DAVI; //E3
}

/**
 *  Aguarda a recepção de um valor de comando válido e passa para próximo es-
 * tado da máquina.
 *
 *  Se o byte recebido não for válido, solicita mensagem de erro.
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void receive_command_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI
        new_event) {
    char i;

    //try received byte
    for (i = 0; i < max_lista_comm_davi_value_len; i++) {
        if (protocol_received_caracter_davi == LISTA_COMMAND_LIST_DAVI[i]) {
            event[MachineProtDaviNr] = EVENT_ID_NONE_DAVI; //E0
            indice_comando_davi = i;
            return;
        }
    }
    //erro de comando inválido
    event[MachineProtDaviNr] = EVENT_ERRO_DAVI; //E3
}

/**
 *  Aguarda a recepção de um valor de formato válido e passa para próximo es-
 * tado da máquina.
 *
 *  Se o byte recebido não for válido, solicita mensagem de erro.
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void receive_format_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI
        new_event) {

    //compara byte recebido com lista de referencia
    if (protocol_received_caracter_davi == FORMAT_HEX_DAVI ||
            protocol_received_caracter_davi == FORMAT_BCD_DAVI) {
        event[MachineProtDaviNr] = EVENT_ID_NONE_DAVI; //E0
    } else {
        //        protocol_received_caracter_davi = ANSWER_QUADRO_DAVI;
        event[MachineProtDaviNr] = EVENT_ERRO_DAVI; //E3
    }
}

/**
 *  Aguarda a recepção de um valor de tamanho permitido e passa para próximo es-
 * tado da máquina.
 *
 *  Se o byte recebido não for válido, solicita mensagem de erro.
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void receive_number_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI
        new_event) {

    //compara byte recebido com lista de referencia
    //se valor recebido iguala o valor definido de bytes, continue
    if (protocol_received_caracter_davi ==
            LISTA_N_BYTES_DAVI[indice_comando_davi]
            || protocol_received_caracter_davi ==
            LISTA_N_BYTES_ERRO_PROTOCOL_DAVI[indice_comando_davi]) {
        //se nbytes igual a zero, pula para cálculo de CRC
        if (protocol_received_caracter_davi)
            event[MachineProtDaviNr] = EVENT_ID_NONE_DAVI; //E0
        else
            event[MachineProtDaviNr] = EVENT_N_ZERO_DAVI; //E4
    } else {
        //se valor não coincide, event = EVENT_ERRO
        event[MachineProtDaviNr] = EVENT_ERRO_DAVI; //E3
    }
}

/**
 *  Aguarda a recepção de todos os bytes definidos pela mensagem.
 *  após o fim do payload, passa para o cálculo e comparação de CRC.
 *
 *  O único erro possível neste estado é timeout, fornecido pela função de moni-
 * toração receive_idle_smf_davi
 * 
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void receive_payload_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI
        new_event) {

    //se valor recebido igual ao valor definido de bytes, termina
    if ((mensagem_davi_index
            - sizeof (mensagem_Davi.overhead)) ==
            mensagem_Davi.overhead.length)
        event[MachineProtDaviNr] = EVENT_N_ZERO_DAVI; //E4
    else
        //se valor não coincide, aguarda próximo evento
        event[MachineProtDaviNr] = EVENT_ID_NONE_DAVI; //E0
}

/**
 *  Aguarda a recepção dos bytes de CRC.
 *  calcula o valor do CRC do buffer com que foi recebido.
 *
 *  se houver erro, solicita o informe na mensagem de retorno.
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void receive_crc_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI
        new_event) {
    //testa se recebeu os dois bytes do CRC
    if ((mensagem_davi_index
            - sizeof (mensagem_Davi.overhead)
            - sizeof (unsigned short)) ==
            mensagem_Davi.overhead.length)
                //testa se o CRC coincide com a mensagem
        if (generate_16bit_crc(mensagem_Davi.buffer, mensagem_davi_index,
                CRC_16)) {
            //indica erro de CRC
            event[MachineProtDaviNr] = EVENT_ERRO_DAVI;
        } else
            //CRC correto
            event[MachineProtDaviNr] = EVENT_N_ZERO_DAVI;
    else
        //aguarda próximo byte do CRC
        event[MachineProtDaviNr] = EVENT_ID_NONE_DAVI; //E0
}

/**
 *  Estado final da máquina, quando toda a mensagem sem erros foi recebida e
 * armazenada no buffer.
 *  Esta condição é informada no flag.
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void receive_message_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI
        new_event) {
    //informa que a mensagem está completa
    flag_mensagem_ok_davi = true;
    mensagem_davi_index = 0;
}

/**
 *   Gera a mensagem de erro, de acordo com o estado anterior da máquina.
 *
 *   A mensagem de ERRO não corrompe o buffer da mensagem atual.
 *   Para mensagens novas, a mensagem de erro depende da tabela cadastrada em
 * ANSWER_ERRO_DAVI.
 *   A liberação para nova mensagem não depende desta camada (física) e sim da
 * camada de aplicação.
 *   Ou seja, enquanto a aplicação não liberar nova mensagem, a física responde
 * com erro de OCUPADO, sem corromper o buffer atual, em uso pela aplicação.
 *
 *   O modo transparente é habilitado pela camada de aplicação.
 *   No modo transparente todas as mensagens não interpretadas são repassadas
 * para a porta remota conectada ao instrumento sob teste.
 *   As mensagens A4 serão sempre tratadas e respondidas na camada física.
 *
 *   Nesta interface não chegam mensagens com o cabeçalho A3, A5 ou A6. Se che-
 * garem, serão tratadas com o modo: Transparente/ensaio (retransmite/descarta).
 * 
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void receive_erro_stf_davi(enum STATE_ID_PROTOCOL_DAVI cur_state,
        enum STATE_ID_PROTOCOL_DAVI new_state, enum EVENT_ID_PROTOCOL_DAVI
        new_event) {
    unsigned short crc;
    union protocolDAVI_erro mensagem_erro_DAVI;
    int i;

    if ((new_event == EVENT_ERRO_DAVI)
            && (cur_state == STATE_STX_DAVI)) {
        if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE)
            //repassa a mensagem recebida em STX
            serOutchar(mensagem_Davi.buffer[0]);
        //ou descarta caracteres até receber o STX válido
    }
    else {
        if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE){
            //repassa a mensagem recebida no buffer
            for (i = 0; i < mensagem_davi_index; i++)
                serOutchar(mensagem_Davi.buffer[i]);
        }
        //altera estado atual para gerar mensagem de timeout
        if (new_event == EVENT_TIMEOUT_DAVI)
            cur_state = STATE_TIMEOUT_DAVI;
        //    monta mensagem com o erro adequado
        //    envia mensagem de erro para o correspondente
        mensagem_erro_DAVI.overhead.head = STX_ERROR_DAVI;
        mensagem_erro_DAVI.overhead.command = COMMAND_NOP_DAVI; // \x00
        mensagem_erro_DAVI.overhead.format = FORMAT_HEX_DAVI; // \x00
        mensagem_erro_DAVI.overhead.length = sizeof
                (LISTA_RESPOSTA_ERRO_PROTOCOL_DAVI[0]);
        mensagem_erro_DAVI.payload.parameter[0] =
                *((char*) (&LISTA_RESPOSTA_ERRO_PROTOCOL_DAVI[cur_state])
                + 1);
        mensagem_erro_DAVI.payload.parameter[1] =
                *((char*) (&LISTA_RESPOSTA_ERRO_PROTOCOL_DAVI[cur_state])
                + 0);
        crc = generate_16bit_crc(mensagem_erro_DAVI.buffer, 6, CRC_16);
        mensagem_erro_DAVI.payload.parameter[sizeof
                (LISTA_RESPOSTA_ERRO_PROTOCOL_DAVI[0])]
                = *((char*) (&crc) + 1);
        mensagem_erro_DAVI.payload.parameter[sizeof
                (LISTA_RESPOSTA_ERRO_PROTOCOL_DAVI[0]) + 1] = *((char*)
                (&crc) + 0);
        ser2Outstring(mensagem_erro_DAVI.buffer, 8);
    }
    mensagem_davi_index = 0;
}
