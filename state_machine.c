/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/
#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#endif

#if defined(__XC)
#include <stdint.h>         /* For uint8_t definition */
#endif

#include "state_machine.h"
#include "machine_main.h"
#include "machine_quilometrico.h"
#include "machine_w.h"
#include "machine_horario.h"
#include "machine_calibracao.h"
#include "machine_ajuste.h"

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
void state_machine_initializer(void){
    unsigned char machine;

    //inicializa tabela de chamada de funções da maquina de estado
    state_tbl[MachineMainNr] = state_tbl_main;
    state_tbl_size[MachineMainNr] = max_state_tbl_main_len;
    state_tbl[MachineProtDaviNr] = state_tbl_davi;
    state_tbl_size[MachineProtDaviNr] = max_state_tbl_davi_len;
    state_tbl[MachineProt393Nr] = state_tbl_393;
    state_tbl_size[MachineProt393Nr] = max_state_tbl_protocol393_len;

    state_tbl[MachineQuilometricoNr] = state_tbl_Quilometrico;
    state_tbl_size[MachineQuilometricoNr] = max_state_tbl_quilometrico_len;
    state_tbl[MachineWNr] = state_tbl_W;
    state_tbl_size[MachineWNr] = max_state_tbl_w_len;
    state_tbl[MachineHorarioNr] = state_tbl_Horario;
    state_tbl_size[MachineHorarioNr] = max_state_tbl_horario_len;
    state_tbl[MachineCalibracaoNr] = state_tbl_Calibracao;
    state_tbl_size[MachineCalibracaoNr] = max_state_tbl_calibracao_len;
    state_tbl[MachineAjusteNr] = state_tbl_Ajuste;
    state_tbl_size[MachineAjusteNr] = max_state_tbl_ajuste_len;

    //inicializa o estado de cada máquina
    for (machine = 0; machine < MachineNumberMax; machine++) {
        event[machine] = EVENT_ID_NONE;
        current_state[machine] = STATE_ID_IDLE;
        current_exclusion[machine] = NOT_OWNED; //MUTEX_machine[machine];
    }
}

void state_machine_dispatcher(unsigned char n) {
    unsigned char j;

    if (event[n] != EVENT_ID_NONE) {
        for (j = 0; j < state_tbl_size[n]; j++) {
            if (state_tbl[n][j].cur_state == current_state[n] &&
                    state_tbl[n][j].event == event[n]) {
                tmp_event = event[n];
                event[n] = EVENT_ID_NONE;
                (*state_tbl[n][j].transition_func)(
                        current_state[n],
                        state_tbl[n][j].new_state,
                        tmp_event);
                current_state[n] = state_tbl[n][j].new_state;
                state_tbl_indx[n] = j;
                break;
            }
        }
    }
    (*state_tbl[n][state_tbl_indx[n]].monitor_func)(current_state[n]);
    CLRWDT();
}
