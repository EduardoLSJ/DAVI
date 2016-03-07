/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef _machine_w_h
#define	_machine_w_h

#include <xc.h>
#include "machine_prot_davi.h"


/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#define max_state_tbl_w_len sizeof(state_tbl_W)/sizeof(state_tbl_W[0])

typedef enum EVENT_ID_W {
    EVENT_NONE_W,
    EVENT_RECEBE_MENSAGEM_INSTRUMENTO_W,
    EVENT_RECEBE_ERRO_INSTRUMENTO_W,
    EVENT_RECEBE_COMANDO_PC_W,
    EVENT_LOCK_W,
    EVENT_RELEASE_W,
    EVENT_ENSAIO_ABORTAR_W,
    EVENT_TIMEOUT_W,
    EVENT_ERRO_W,
    EVENT_ENSAIO_NOP_W,
    EVENT_1_SEC_W,
    EVENT_PULSE_IN_W, /*pulso de medição ou mudança de estado, portaria 393*/
    EVENT_ENSAIO_LEVANTA_W,
};

typedef enum STATE_ID_W {
    STATE_NOT_OWNED_W,
    STATE_ENSAIO_LEVANTA_W,
    STATE_E_LW_COMANDO,
    STATE_E_LW_PASSAGEM,
    STATE_ERRO_W
};

enum ANSWER_ERRO_W {
    ANSWER_ERRO_NULO_W = ANSWER_OCUPADO_DAVI,
    ANSWER_ERRO_INST_TIMEOUT_W,
    ANSWER_ERRO_PC_TIMEOUT_W,
    ANSWER_ERRO_INST_SEQUENCIA_W,
    ANSWER_ERRO_INST_MODO_W,
    ANSWER_ERRO_INST_POSC_W,
    ANSWER_ERRO_INST_FRACAO_W,
    ANSWER_ERRO_INST_VT_W,
    ANSWER_ERRO_INST_DIST_W,
    ANSWER_ERRO_INST_VMAX_W
};

void release_STF_w(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event);
void mutex_SMF_w(enum STATE_ID_W state);
void idle_SMF_w(enum STATE_ID_W state);
void idle_STF_w(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event);
void none_SMF_w(enum STATE_ID_W state);
void erro_STF_w(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event);
void ensaio_levanta_w_STF(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event);
void ensaio_levanta_w_comando_STF(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event);
void ensaio_levanta_w_pulso_STF(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event);
void ensaio_levanta_w_SMF(enum STATE_ID_W state);
void passagem_STF_w(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event);

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
const struct STATE_DESCRIPTION
state_tbl_W[] = {
    /*eventos de entrada da maquina de estado MUTEX*/
    {
        STATE_NOT_OWNED_W,
        STATE_NOT_OWNED_W,
        EVENT_NONE_W,
        idle_STF_w,
        mutex_SMF_w
    },
    {
        STATE_NOT_OWNED_W,
        STATE_ENSAIO_LEVANTA_W,
        EVENT_LOCK_W,
        ensaio_levanta_w_STF,
        ensaio_levanta_w_SMF
    },
    /*eventos do estado levanta_w da máquina*/
    {
        STATE_ENSAIO_LEVANTA_W,
        STATE_ENSAIO_LEVANTA_W,
        EVENT_PULSE_IN_W,
        ensaio_levanta_w_pulso_STF,
        ensaio_levanta_w_SMF
    },
    {
        STATE_ENSAIO_LEVANTA_W,
        STATE_E_LW_PASSAGEM,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_W,
        passagem_STF_w,
        none_SMF_w
    },
    {
        STATE_ENSAIO_LEVANTA_W,
        STATE_E_LW_PASSAGEM,
        EVENT_RECEBE_ERRO_INSTRUMENTO_W,
        passagem_STF_w,
        none_SMF_w
    },
    {
        STATE_E_LW_PASSAGEM,
        STATE_ENSAIO_LEVANTA_W,
        EVENT_ENSAIO_NOP_W,
        idle_STF_w,
        ensaio_levanta_w_SMF
    },
    {
        STATE_ENSAIO_LEVANTA_W,
        STATE_E_LW_COMANDO,
        EVENT_RECEBE_COMANDO_PC_W,
        ensaio_levanta_w_comando_STF,
        none_SMF_w
    },
    {
        STATE_E_LW_COMANDO,
        STATE_ENSAIO_LEVANTA_W,
        EVENT_ENSAIO_NOP_W,
        idle_STF_w,
        ensaio_levanta_w_SMF
    },
    /*eventos de saida da maquina de estado MUTEX*/
    {
        STATE_ENSAIO_LEVANTA_W,
        STATE_ERRO_W,
        EVENT_ERRO_W,
        erro_STF_w,
        idle_SMF_w
    },
    {
        STATE_E_LW_COMANDO,
        STATE_ERRO_W,
        EVENT_ENSAIO_ABORTAR_W,
        erro_STF_w,
        idle_SMF_w
    },
    {
        STATE_ERRO_W,
        STATE_ERRO_W,
        EVENT_ERRO_W,
        erro_STF_w,
        idle_SMF_w
    },
    {
        STATE_ERRO_W,
        STATE_ERRO_W,
        EVENT_RECEBE_COMANDO_PC_W,
        erro_STF_w,
        idle_SMF_w
    },
    {
        STATE_ERRO_W,
        STATE_ERRO_W,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_W,
        erro_STF_w,
        idle_SMF_w
    },
    {
        STATE_ERRO_W,
        STATE_ERRO_W,
        EVENT_RECEBE_ERRO_INSTRUMENTO_W,
        erro_STF_w,
        idle_SMF_w
    },
    {
        STATE_ERRO_W,
        STATE_NOT_OWNED_W,
        EVENT_RELEASE_W,
        release_STF_w,
        mutex_SMF_w
    }
};
#endif	/* _machine_w_h */

