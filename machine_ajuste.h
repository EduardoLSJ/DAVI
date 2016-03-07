/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef _machine_ajuste_h
#define	_machine_ajuste_h

#include <xc.h> // include processor files - each processor file is guarded.  
#include "state_machine.h"


/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#define max_state_tbl_ajuste_len sizeof(state_tbl_Ajuste)/sizeof(state_tbl_Ajuste[0])

typedef enum EVENT_ID_AJUSTE {
    EVENT_NONE_AJUSTE,
    EVENT_RECEBE_MENSAGEM_INSTRUMENTO_AJUSTE,
    EVENT_RECEBE_ERRO_INSTRUMENTO_AJUSTE,
    EVENT_RECEBE_COMANDO_PC_AJUSTE,
    EVENT_LOCK_AJUSTE,
    EVENT_RELEASE_AJUSTE,
    EVENT_ENSAIO_ABORTAR_AJUSTE,
    EVENT_TIMEOUT_AJUSTE,
    EVENT_ERRO_AJUSTE,
    EVENT_ENSAIO_NOP_AJUSTE,
    EVENT_AUTO_AJUSTE
};

/**
 ESTADOS PERMITIDOS NA MAQUINA AJUSTE
 */
typedef enum STATE_ID_AJUSTE {
    STATE_IDLE_AJUSTE,
    STATE_AUTO_AJUSTE,
    STATE_ERRO_AJUSTE
};

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
void idle_SMF_aju(enum STATE_ID_AJUSTE state);
void idle_STF_aju(enum STATE_ID_AJUSTE cur_state,
        enum STATE_ID_AJUSTE new_state, enum EVENT_ID_AJUSTE new_event);
void erro_STF_aju(enum STATE_ID_AJUSTE cur_state,
        enum STATE_ID_AJUSTE new_state, enum EVENT_ID_AJUSTE new_event);

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
const struct STATE_DESCRIPTION
state_tbl_Ajuste[] = {
    /*eventos do estado Recebe_comando da máquina*/
    {
        STATE_IDLE_AJUSTE,
        STATE_IDLE_AJUSTE,
        EVENT_RECEBE_COMANDO_PC_AJUSTE,
        idle_STF_aju,
        idle_SMF_aju
    },
    {/*TODO criar funcao para ajustar o instrumento*/
        STATE_IDLE_AJUSTE,
        STATE_AUTO_AJUSTE,
        EVENT_AUTO_AJUSTE,
        idle_STF_aju,
        idle_SMF_aju
    },
    /*eventos do estado calibração da máquina*/
    
    /*eventos de saida da maquina de estado MUTEX*/
    {
        STATE_AUTO_AJUSTE,
        STATE_IDLE_AJUSTE,
        EVENT_RELEASE_AJUSTE,
        idle_STF_aju,
        idle_SMF_aju
    },
    {
        STATE_AUTO_AJUSTE,
        STATE_ERRO_AJUSTE,
        EVENT_ENSAIO_ABORTAR_AJUSTE,
        erro_STF_aju,
        idle_SMF_aju
    },
    {
        STATE_AUTO_AJUSTE,
        STATE_ERRO_AJUSTE,
        EVENT_ERRO_AJUSTE,
        erro_STF_aju,
        idle_SMF_aju
    },
    
    {
        STATE_ERRO_AJUSTE,
        STATE_ERRO_AJUSTE,
        EVENT_ERRO_AJUSTE,
        erro_STF_aju,
        idle_SMF_aju
    },
    {
        STATE_ERRO_AJUSTE,
        STATE_ERRO_AJUSTE,
        EVENT_RECEBE_COMANDO_PC_AJUSTE,
        erro_STF_aju,
        idle_SMF_aju
    },
    {
        STATE_ERRO_AJUSTE,
        STATE_ERRO_AJUSTE,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_AJUSTE,
        erro_STF_aju,
        idle_SMF_aju
    },
    {
        STATE_ERRO_AJUSTE,
        STATE_ERRO_AJUSTE,
        EVENT_RECEBE_ERRO_INSTRUMENTO_AJUSTE,
        erro_STF_aju,
        idle_SMF_aju
    },
    {
        STATE_ERRO_AJUSTE,
        STATE_IDLE_AJUSTE,
        EVENT_RELEASE_AJUSTE,
        idle_STF_aju,
        idle_SMF_aju
    }
};

#endif	/* _state_ajuste_h */

