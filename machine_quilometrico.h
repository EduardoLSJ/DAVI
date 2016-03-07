/* 
 * File:   machine_quilometrico.h
 * Author: edu
 *
 * Created on 24 de Outubro de 2015, 18:44
 */

#ifndef _machine_quilometrico_h
#define	_machine_quilometrico_h

#include "state_machine.h" //teste
#include "machine_prot_davi.h"

/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#define max_state_tbl_quilometrico_len sizeof(state_tbl_Quilometrico)/sizeof(state_tbl_Quilometrico[0])

#define ATRASO_ACELERADOR 2 /*tempo em segundos para cada velocidade na busca de VT*/

enum VALUE_MODO_QUILOMETRICO {
    V_ENSAIO_MAX = 150,
    V_ENSAIO_MIN = 1,
};

typedef enum EVENT_ID_QUILO {
    EVENT_NONE_QUILO,
    EVENT_RECEBE_MENSAGEM_INSTRUMENTO_QUILO,
    EVENT_RECEBE_ERRO_INSTRUMENTO_QUILO,
    EVENT_RECEBE_COMANDO_PC_QUILO,
    EVENT_LOCK_QUILO,
    EVENT_RELEASE_QUILO,
    EVENT_ENSAIO_ABORTAR_QUILO,
    EVENT_TIMEOUT_QUILO,
    EVENT_ERRO_QUILO,
    EVENT_ENSAIO_NOP_QUILO,
    EVENT_1_SEC_QUILO,
    EVENT_PULSE_IN_QUILO, /*pulso de medição ou mudança de estado, portaria 393*/
    EVENT_ENSAIO_QUILOMETRICO,
    EVENT_E_Q_1,
    EVENT_DISTANCIA_QUILO,
};

typedef enum STATE_ID_QUILO {
    STATE_NOT_OWNED_QUILO,
    STATE_ERRO_QUILO,
    STATE_OWNED_QUILO,
    STATE_ENSAIO_QUILOMETRICO,
    STATE_E_Q_DELTA,
    STATE_E_Q_PASSAGEM,
    STATE_E_Q_COMANDO,
};

typedef enum STATE_QUILO_DELTA {
    QDELTA_INICIO, /*configura ensaio*/
    QDELTA_VT, /*testa e armazena velocidade de transição*/
    QDELTA_VENSAIO, /*acelera até velocidade de ensaio*/
    QDELTA_FRA_INI, /*aguarda evento e armazena fração inicial*/
    QDELTA_DISTANCIA, /*guarda distância entre as frações*/
    QDELTA_FRA_FINAL, /*aguarda e armazena distância da última fração*/
    QDELTA_SAIDA_OU_ERRO /*saída do ensaio*/
};

enum ANSWER_ERRO_QUILO {
    ANSWER_ERRO_NULO_QUILO = ANSWER_OCUPADO_DAVI,
    ANSWER_ERRO_INST_TIMEOUT_QUILO,
    ANSWER_ERRO_PC_TIMEOUT_QUILO,
    ANSWER_ERRO_INST_SEQUENCIA_QUILO,
    ANSWER_ERRO_INST_MODO_QUILO,
    ANSWER_ERRO_INST_POSC_QUILO,
    ANSWER_ERRO_INST_FRACAO_QUILO,
    ANSWER_ERRO_INST_VT_QUILO,
    ANSWER_ERRO_INST_DIST_QUILO
};

void release_STF_quilo(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event);
void mutex_SMF_quilo(enum STATE_ID_QUILO state);
void idle_SMF_quilo(enum STATE_ID_QUILO state);
void idle_STF_quilo(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event);
void none_SMF_quilo(enum STATE_ID_QUILO state);
void erro_STF_quilo(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event);
void passagem_STF_quilo(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event);
void ensaio_quilom_STF(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event);
void ensaio_quilom_comando_STF(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event);
void ensaio_quilom_delta_STF(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event);
void ensaio_quilom_SMF(enum STATE_ID_QUILO state);
void acelerador(char* velocidade_atual, char* velocidade_desejada);

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
const struct STATE_DESCRIPTION
state_tbl_Quilometrico[] = {
    /*eventos de entrada da maquina de estado MUTEX*/
    {
        STATE_NOT_OWNED_QUILO,
        STATE_NOT_OWNED_QUILO,
        EVENT_NONE_QUILO,
        idle_STF_quilo,
        mutex_SMF_quilo
    },
    {
        STATE_NOT_OWNED_QUILO,
        STATE_ENSAIO_QUILOMETRICO,
        EVENT_LOCK_QUILO,
        ensaio_quilom_STF,
        ensaio_quilom_SMF
    },
    /*eventos do estado quilometrico da máquina*/
    {
        STATE_ENSAIO_QUILOMETRICO,
        STATE_E_Q_DELTA,
        EVENT_1_SEC_QUILO,
        ensaio_quilom_delta_STF,
        ensaio_quilom_SMF
    },
    {
        STATE_ENSAIO_QUILOMETRICO,
        STATE_E_Q_PASSAGEM,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_QUILO,
        passagem_STF_quilo,
        none_SMF_quilo
    },
    {
        STATE_ENSAIO_QUILOMETRICO,
        STATE_E_Q_PASSAGEM,
        EVENT_RECEBE_ERRO_INSTRUMENTO_QUILO,
        passagem_STF_quilo,
        none_SMF_quilo
    },
    {
        STATE_E_Q_PASSAGEM,
        STATE_ENSAIO_QUILOMETRICO,
        EVENT_ENSAIO_NOP_QUILO,
        idle_STF_quilo,
        ensaio_quilom_SMF
    },
    {
        STATE_ENSAIO_QUILOMETRICO,
        STATE_E_Q_COMANDO,
        EVENT_RECEBE_COMANDO_PC_QUILO,
        ensaio_quilom_comando_STF,
        none_SMF_quilo
    },
    {
        STATE_E_Q_COMANDO,
        STATE_ENSAIO_QUILOMETRICO,
        EVENT_ENSAIO_NOP_QUILO,
        idle_STF_quilo,
        ensaio_quilom_SMF
    },
    {
        STATE_E_Q_DELTA,
        STATE_E_Q_DELTA,
        EVENT_1_SEC_QUILO,
        ensaio_quilom_delta_STF,
        ensaio_quilom_SMF
    },
    {
        STATE_E_Q_DELTA,
        STATE_E_Q_DELTA,
        EVENT_DISTANCIA_QUILO,
        ensaio_quilom_delta_STF,
        ensaio_quilom_SMF
    },
    {
        STATE_E_Q_DELTA,
        STATE_E_Q_DELTA,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_QUILO,
        ensaio_quilom_delta_STF,
        ensaio_quilom_SMF
    },
    {
        STATE_E_Q_DELTA,
        STATE_E_Q_DELTA,
        EVENT_PULSE_IN_QUILO,
        ensaio_quilom_delta_STF,
        ensaio_quilom_SMF
    },
    {
        STATE_E_Q_DELTA,
        STATE_E_Q_COMANDO,
        EVENT_RECEBE_COMANDO_PC_QUILO,
        ensaio_quilom_comando_STF,
        none_SMF_quilo
    },
    {
        STATE_E_Q_COMANDO,
        STATE_E_Q_DELTA,
        EVENT_E_Q_1,
        idle_STF_quilo,
        ensaio_quilom_SMF
    },
    /*eventos de saida da maquina de estado MUTEX*/
    {
        STATE_ENSAIO_QUILOMETRICO,
        STATE_ERRO_QUILO,
        EVENT_ERRO_QUILO,
        erro_STF_quilo,
        idle_SMF_quilo
    },
    {
        STATE_E_Q_DELTA,
        STATE_ERRO_QUILO,
        EVENT_ERRO_QUILO,
        erro_STF_quilo,
        idle_SMF_quilo
    },    
    {
        STATE_E_Q_COMANDO,
        STATE_ERRO_QUILO,
        EVENT_ENSAIO_ABORTAR_QUILO,
        erro_STF_quilo,
        idle_SMF_quilo
    },
    {
        STATE_ERRO_QUILO,
        STATE_ERRO_QUILO,
        EVENT_ERRO_QUILO,
        erro_STF_quilo,
        idle_SMF_quilo
    },
    {
        STATE_ERRO_QUILO,
        STATE_ERRO_QUILO,
        EVENT_RECEBE_COMANDO_PC_QUILO,
        erro_STF_quilo,
        idle_SMF_quilo
    },
    {
        STATE_ERRO_QUILO,
        STATE_ERRO_QUILO,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_QUILO,
        erro_STF_quilo,
        idle_SMF_quilo
    },
    {
        STATE_ERRO_QUILO,
        STATE_ERRO_QUILO,
        EVENT_RECEBE_ERRO_INSTRUMENTO_QUILO,
        erro_STF_quilo,
        idle_SMF_quilo
    },
    {
        STATE_ERRO_QUILO,
        STATE_NOT_OWNED_QUILO,
        EVENT_RELEASE_QUILO,
        release_STF_quilo,
        mutex_SMF_quilo
    }
};
#endif	/* _machine_quilometrico_h */
