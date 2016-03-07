/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#ifndef _state_machine_h
#define _state_machine_h

/******************************************************************************/
/* All state machines defines                                                 */

/******************************************************************************/
enum MachineNr {
    MachineMainNr, //0
    MachineProtDaviNr, //1
    MachineProt393Nr, //2
    MachineQuilometricoNr, //3
    MachineWNr, //4
    MachineHorarioNr, //5
    MachineCalibracaoNr, //6
    MachineAjusteNr, //7
    MachineNumberMax
};

typedef enum MUTEX {//mutual exclusion state
    NOT_OWNED,
    OWNED
};

typedef enum EVENT_ID {
    EVENT_ID_NONE
};

typedef enum STATE_ID {
    STATE_ID_IDLE
};

typedef struct STATE_DESCRIPTION {
    unsigned char cur_state,
            new_state,
            event;
    void (*transition_func)(enum STATE_ID, enum STATE_ID,
            enum EVENT_ID);
    void (*monitor_func)(enum STATE_ID);
};

enum EVENT_ID event[MachineNumberMax], tmp_event;
enum STATE_ID current_exclusion[MachineNumberMax];
enum STATE_ID current_state[MachineNumberMax];
unsigned char state_tbl_indx[MachineNumberMax];
unsigned char state_tbl_size[MachineNumberMax];
const struct STATE_DESCRIPTION*
state_tbl[MachineNumberMax];

/** 1
 * 2 Descrição da função
 * 3
 * 4 tomada de 
 */
void state_machine_initializer(void);
void state_machine_dispatcher(unsigned char);

#endif /* _state_machine_h */
