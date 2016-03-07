/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef _machine_calibracao_h
#define	_machine_calibracao_h

#include <xc.h>
#include "state_machine.h"


/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#define max_state_tbl_calibracao_len sizeof(state_tbl_Calibracao)/sizeof(state_tbl_Calibracao[0])

typedef enum EVENT_ID_CALIBRACAO {
    EVENT_NONE_CALIBRACAO,
    EVENT_RECEBE_MENSAGEM_INSTRUMENTO_CALIBRACAO,//não usado, permanece para manter árvore
    EVENT_RECEBE_ERRO_INSTRUMENTO_CALIBRACAO,//não usado, permanece para manter árvore
    EVENT_RECEBE_COMANDO_PC_CALIBRACAO,
    EVENT_LOCK_CALIBRACAO,
    EVENT_RELEASE_CALIBRACAO,
    EVENT_ENSAIO_ABORTAR_CALIBRACAO,
    EVENT_TIMEOUT_CALIBRACAO,
    EVENT_ERRO_CALIBRACAO,
    EVENT_ENSAIO_NOP_CALIBRACAO,//não usado, permanece para manter árvore
    EVENT_NOVA_CALIBRACAO,//não usado, permanece para manter árvore
    EVENT_PULSE_IN_CALIBRACAO
};

typedef enum STATE_ID_CALIBRACAO {
    STATE_NOT_OWNED_CAL,
    STATE_CALIBRACAO,
    STATE_CAL_COMANDO,
    STATE_ERRO_CAL
};

void release_STF_cal(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event);
void mutex_SMF_cal(enum STATE_ID_CALIBRACAO state);
void idle_SMF_cal(enum STATE_ID_CALIBRACAO state);
void idle_STF_cal(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event);
void none_SMF_cal(enum STATE_ID_CALIBRACAO state);
void erro_STF_cal(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event);
void calibra_comando_STF(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event);

void calibra_STF_cal(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event);
void calibra_SMF_cal(enum STATE_ID_CALIBRACAO state);
void calibra_pwm(void);

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
const struct STATE_DESCRIPTION
state_tbl_Calibracao[] = {
    /*eventos do estado Recebe_comando da máquina*/
    {
        STATE_NOT_OWNED_CAL,
        STATE_NOT_OWNED_CAL,
        EVENT_NONE_CALIBRACAO,
        idle_STF_cal,
        mutex_SMF_cal
    },
    /*eventos do estado calibração da máquina*/
    {
        STATE_NOT_OWNED_CAL,
        STATE_CALIBRACAO,
        EVENT_LOCK_CALIBRACAO,
        calibra_comando_STF,
        calibra_SMF_cal
    },
    {
        STATE_CALIBRACAO,
        STATE_CALIBRACAO,
        EVENT_PULSE_IN_CALIBRACAO,
        calibra_STF_cal,
        calibra_SMF_cal
    },
    {
        STATE_CALIBRACAO,
        STATE_CAL_COMANDO,
        EVENT_RECEBE_COMANDO_PC_CALIBRACAO,
        calibra_comando_STF,
        none_SMF_cal
    },
    {
        STATE_CAL_COMANDO,
        STATE_CALIBRACAO,
        EVENT_ENSAIO_NOP_CALIBRACAO,
        idle_STF_cal,
        calibra_SMF_cal
    },
    /*eventos de saida da maquina de estado MUTEX*/
    {
        STATE_CALIBRACAO,
        STATE_ERRO_CAL,
        EVENT_ERRO_CALIBRACAO,
        erro_STF_cal,
        idle_SMF_cal
    },
    {
        STATE_CAL_COMANDO,
        STATE_ERRO_CAL,
        EVENT_ENSAIO_ABORTAR_CALIBRACAO,
        erro_STF_cal,
        idle_SMF_cal
    },
    {
        STATE_ERRO_CAL,
        STATE_ERRO_CAL,
        EVENT_ERRO_CALIBRACAO,
        erro_STF_cal,
        idle_SMF_cal
    },
    {
        STATE_ERRO_CAL,
        STATE_ERRO_CAL,
        EVENT_RECEBE_COMANDO_PC_CALIBRACAO,
        erro_STF_cal,
        idle_SMF_cal
    },
    {
        STATE_ERRO_CAL,
        STATE_NOT_OWNED_CAL,
        EVENT_RELEASE_CALIBRACAO,
        release_STF_cal,
        mutex_SMF_cal
    }
};

#endif	/* _machine_calibracao_h */

