/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef _machine_horario_h
#define	_machine_horario_h

#include <xc.h> // include processor files - each processor file is guarded.  
#include "state_machine.h"
#include "machine_prot_davi.h"

/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#define max_state_tbl_horario_len sizeof(state_tbl_Horario)/sizeof(state_tbl_Horario[0])
  
/*determinado por 8.1.2 alínea j
 tempo máximo = 30 minutos*/
#define T_MAX 30
/*determinado por 6.4.2, fração mínima = 0,10*/
#define FR_MIN 0.1
#define N_FRACOES_MIN 1
 /*determinado por 6.5 tarifa (máx = R$100/hora)*/
#define TAR_MAX 100
#define N_FRACOES_MAX 500L //T_MAX*TAR_MAX*(1/FR_MIN)*(1/60)

typedef enum EVENT_ID_HORA {
    EVENT_NONE_HORA,
    EVENT_RECEBE_MENSAGEM_INSTRUMENTO_HORA,
    EVENT_RECEBE_ERRO_INSTRUMENTO_HORA,
    EVENT_RECEBE_COMANDO_PC_HORA,
    EVENT_LOCK_HORA,
    EVENT_RELEASE_HORA,
    EVENT_ENSAIO_ABORTAR_HORA,
    EVENT_TIMEOUT_HORA,
    EVENT_ERRO_HORA,
    EVENT_ENSAIO_NOP_HORA,
    EVENT_1_SEC_HORA,
    EVENT_PULSE_IN_HORA, /*pulso de medição ou mudança de estado, portaria 393*/
    EVENT_ENSAIO_HORARIO
};

typedef enum STATE_ID_HORA {
    STATE_NOT_OWNED_HORA,
    STATE_ERRO_HORA,
    STATE_OWNED_HORA,
    STATE_ENSAIO_HORARIO,
    STATE_E_H_DELTA,
    STATE_E_H_PASSAGEM,
    STATE_E_H_COMANDO,
};

typedef enum STATE_HORARIO_DELTA {
    HDELTA_INICIO, /*configura ensaio*/
    HDELTA_FRA_INI, /*aguarda evento e armazena fração inicial*/
    HDELTA_HORARIO, /*guarda intervalo de tempo entre as frações*/
    HDELTA_SAIDA_OU_ERRO /*saída do ensaio*/
};


enum ANSWER_ERRO_HORA {
    ANSWER_ERRO_NULO_HORA = ANSWER_OCUPADO_DAVI,
    ANSWER_ERRO_INST_TIMEOUT_HORA,
    ANSWER_ERRO_PC_TIMEOUT_HORA,
    ANSWER_ERRO_INST_SEQUENCIA_HORA,
    ANSWER_ERRO_INST_MODO_HORA,
    ANSWER_ERRO_INST_POSC_HORA,
    ANSWER_ERRO_INST_FRACAO_HORA
};

void release_STF_hora(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event);
void mutex_SMF_hora(enum STATE_ID_HORA state);

void idle_SMF_hora(enum STATE_ID_HORA state);
void idle_STF_hora(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event);
void none_SMF_hora(enum STATE_ID_HORA state);
void erro_STF_hora(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event);
void ensaio_horario_STF(enum STATE_ID_HORA cur_state,
            enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event);
void ensaio_horario_comando_STF(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event);
void ensaio_horario_delta_STF(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event);
void ensaio_horario_SMF(enum STATE_ID_HORA state);
void passagem_STF_hora(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event);

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
const struct STATE_DESCRIPTION
state_tbl_Horario[] = {
    /*eventos de entrada da maquina de estado MUTEX*/
    {
        STATE_NOT_OWNED_HORA,
        STATE_NOT_OWNED_HORA,
        EVENT_NONE_HORA,
        idle_STF_hora,
        mutex_SMF_hora
    },
    {
        STATE_NOT_OWNED_HORA,
        STATE_ENSAIO_HORARIO,
        EVENT_LOCK_HORA,
        ensaio_horario_STF,
        ensaio_horario_SMF
    },    
    /*eventos do estado horário da máquina*/
    {
        STATE_ENSAIO_HORARIO,
        STATE_E_H_DELTA,
        EVENT_PULSE_IN_HORA,
        ensaio_horario_delta_STF,
        ensaio_horario_SMF
    },
    {
        STATE_ENSAIO_HORARIO,
        STATE_E_H_COMANDO,
        EVENT_RECEBE_COMANDO_PC_HORA,
        ensaio_horario_comando_STF,
        none_SMF_hora
    },
    {
        STATE_E_H_COMANDO,
        STATE_ENSAIO_HORARIO,
        EVENT_ENSAIO_HORARIO,
        idle_STF_hora,
        ensaio_horario_SMF
    },
    {
        STATE_ENSAIO_HORARIO,
        STATE_E_H_PASSAGEM,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_HORA,
        passagem_STF_hora,
        none_SMF_hora
    },
    {
        STATE_ENSAIO_HORARIO,
        STATE_E_H_PASSAGEM,
        EVENT_RECEBE_ERRO_INSTRUMENTO_HORA,
        passagem_STF_hora,
        none_SMF_hora
    },
    {
        STATE_E_H_PASSAGEM,
        STATE_ENSAIO_HORARIO,
        EVENT_ENSAIO_NOP_HORA,
        idle_STF_hora,
        ensaio_horario_SMF
    },
    {
        STATE_E_H_DELTA,
        STATE_E_H_DELTA,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_HORA,
        ensaio_horario_delta_STF,
        ensaio_horario_SMF
    },
    {
        STATE_E_H_DELTA,
        STATE_E_H_DELTA,
        EVENT_PULSE_IN_HORA,
        ensaio_horario_delta_STF,
        ensaio_horario_SMF
    },
    {
        STATE_E_H_DELTA,
        STATE_E_H_DELTA,
        EVENT_1_SEC_HORA,
        ensaio_horario_delta_STF,
        ensaio_horario_SMF
    },
    {
        STATE_E_H_DELTA,
        STATE_E_H_COMANDO,
        EVENT_RECEBE_COMANDO_PC_HORA,
        ensaio_horario_comando_STF,
        none_SMF_hora
    },
    {
        STATE_E_H_COMANDO,
        STATE_E_H_DELTA,
        EVENT_ENSAIO_NOP_HORA,
        idle_STF_hora,
        ensaio_horario_SMF
    },
    /*eventos de saida da maquina de estado MUTEX*/
    {
        STATE_ENSAIO_HORARIO,
        STATE_ERRO_HORA,
        EVENT_ERRO_HORA,
        erro_STF_hora,
        idle_SMF_hora
    },
    {
        STATE_E_H_DELTA,
        STATE_ERRO_HORA,
        EVENT_ERRO_HORA,
        erro_STF_hora,
        idle_SMF_hora
    },
    {
        STATE_E_H_COMANDO,
        STATE_ERRO_HORA,
        EVENT_ENSAIO_ABORTAR_HORA,
        erro_STF_hora,
        idle_SMF_hora
    },
    {
        STATE_ERRO_HORA,
        STATE_ERRO_HORA,
        EVENT_ERRO_HORA,
        erro_STF_hora,
        idle_SMF_hora
    },
    {
        STATE_ERRO_HORA,
        STATE_ERRO_HORA,
        EVENT_RECEBE_COMANDO_PC_HORA,
        erro_STF_hora,
        idle_SMF_hora
    },
        {
        STATE_ERRO_HORA,
        STATE_ERRO_HORA,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_HORA,
        erro_STF_hora,
        idle_SMF_hora
    },
    {
        STATE_ERRO_HORA,
        STATE_ERRO_HORA,
        EVENT_RECEBE_ERRO_INSTRUMENTO_HORA,
        erro_STF_hora,
        idle_SMF_hora
    },
    {
        STATE_ERRO_HORA,
        STATE_NOT_OWNED_HORA,
        EVENT_RELEASE_HORA,
        release_STF_hora,
        mutex_SMF_hora
    }
};

#endif	/* _machine_horario_h */

