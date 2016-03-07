/* Device header file */
#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#endif

#include "system.h"
#include "rn41.h"
#include "user.h"
#include "uart2.h"
#include "uart1.h"
#include "machine_prot_davi.h"

#pragma warning disable 359

#if (MOTOVER == RevA)
#define namedevice "S-,Inmetro-EASY\r" //anexa os dois ultimos bytes MAC ao nome
#define pincode "EASY" //usar no minimo 4 digitos
#elif (MOTOVER == RevB)
#define namedevice "SN,Inmetro-HC05\r" //CR
#define pincode "HC05" //usar no minimo 4 digitos
#elif (MOTOVER == RevC)
#define namedevice "S-,Inmetro-DAVI\r" //anexa os dois ultimos bytes MAC ao nome
#define pincode "DAVI" //usar no minimo 4 digitos
#else
#error Revision number of Hardware not defined. Please define it in user.h file.
#endif

// responses to parse
enum {
    BT_PIN,// DAVI (GP command)
    BT_CMD,// = 1, // CMD
    BT_ASK, //?
    BT_AOK, // AOK
    BT_END, // END
    BT_REBOOT, // Reboot!
    BT_ERROR,
    BT_maximo,
    BT_CONFIG_OK,
    BT_TIMEOUT
};

const char * resposta[] = {
    pincode,
    "CMD",
    "?",
    "AOK",
    "END",
    "Reboot!",
    "ERR"
};

enum {
//    CmdMode,
//    ExtStatString,
    SetMode,
    Authentication,
    CmdExit,
    CmdReboot,
//    CmdKill,
    SecPinCode,
//    PinCodeLabel,
    NameDevice,
    PinCode,
    GetPinCode,
};

const char * comando[] = {
//    "$$$",
    //"SO, \r",
    "SM,0\r",
    "SA,1\r",
    "---\r",
    "R,1\r",
    //"K,\r",
    "SP,",
    //"PinCod=",
    namedevice,
    pincode"\r",
    "GP\r",
};

const uint8_t comando_size[] = {
//    3,
    //5,
    5,
    5,
    4,
    4,
    //3,
    3,
    //"PinCod=",
    16,
    5,
    3,    
};

enum{
    bt_state_reset,
    bt_state_cmd,
    bt_state_check_cmd,
    bt_state_check_cfg,
    bt_state_atualizado,
    bt_state_reprogramar,
    bt_state_reprogramar1,
    bt_state_reprogramar2,
    bt_state_reprogramar3,
    bt_state_reboot,
    bt_state_retorno
};

//;*********************************************************************************************************
//;				Initialize RN41 module
//;
//;
//;*********************************************************************************************************

void Rn41Init() {
    uint8_t state = bt_state_reset;
    char bt_response = BT_ERROR;

    do {
        switch (state) {
            case bt_state_reset: //(reset do módulo)
                USART2_RESET = 0;
                Delay_ms(50);
                USART2_RESET = 1;
                state = bt_state_cmd;
            case bt_state_cmd: //(modo de comando)
                Delay_ms(500); //RN41 após Reset em aprox. 470ms
                ser2Outchar('$');
                ser2Outchar('$');
                ser2Outchar('$');
                Delay_ms(250); //RN41 após Reset em aprox. 470ms
                if (bt_response == BT_TIMEOUT) {
                    ser2Outchar('\r');
                    state = bt_state_check_cmd;
                }
                if (bt_response == BT_CMD) {
                    state = bt_state_check_cfg;
                    ser2Outstring(comando[GetPinCode], comando_size[GetPinCode]);
                }
                break;
            case bt_state_check_cmd:
                if (bt_response == BT_ASK) {
                    state = bt_state_check_cfg;
                    ser2Outstring(comando[GetPinCode], comando_size[GetPinCode]);
                } else
                    state = bt_state_cmd;
                break;
            case bt_state_check_cfg: //(solicita config atual: senha)
                if (bt_response == BT_PIN) {
                    ser2Outstring(comando[CmdExit], comando_size[CmdExit]);
                    state = bt_state_atualizado;
                } else
                    if (bt_response == BT_ERROR) {
                        ser2Outstring(comando[NameDevice], comando_size[NameDevice]);
                    state = bt_state_reprogramar;
                } else
                    state = bt_state_cmd;
                break;
            case bt_state_atualizado: //(programação atualizada, pode sair)
                if (bt_response == BT_END) {
                    return;
                } else
                    state = bt_state_cmd;
                break;
            case bt_state_reprogramar: //(reprogramar: namedevice)
                if (bt_response == BT_AOK) {
                    ser2Outstring(comando[SetMode], comando_size[SetMode]);
                    state = bt_state_reprogramar1;
                } else
                    state = bt_state_cmd;
                break;
            case bt_state_reprogramar1: //(reprogramar: modo operação)
                if (bt_response == BT_AOK) {
                    ser2Outstring(comando[Authentication], comando_size[Authentication]);
                    state = bt_state_reprogramar2;
                } else
                    state = bt_state_cmd;
                break;
            case bt_state_reprogramar2: //(reprogramar: autenticação)
                if (bt_response == BT_AOK) {
                    ser2Outstring(comando[SecPinCode], comando_size[SecPinCode]);
                    ser2Outstring(comando[PinCode], comando_size[PinCode]);
                    state = bt_state_reprogramar3;
                } else
                    state = bt_state_cmd;
                break;
            case bt_state_reprogramar3: //(reprogramar: pincode)
                if (bt_response == BT_AOK) {
                    //reinicia modulo para que as mudancas tenham efeito
                    ser2Outstring(comando[CmdReboot], comando_size[CmdReboot]);
                    state = bt_state_reboot;
                } else
                    state = bt_state_cmd;
                break;
            case bt_state_reboot: //(reboot, para salvar programação)
                if (bt_response == BT_REBOOT) {
                    return;
                } else
                    state = bt_state_cmd;
                break;
            default:
                break;
        }
        bt_response = BT_Get_Response();
    } while (state < bt_state_retorno);
}

// Get BlueTooth response, if there is any
char BT_Get_Response() {
    char tmp;
    uint16_t i;
    uint8_t len;
    char buffer[256];
    char *string;

    contador_timeout_davi = TIMEOUT_VALUE;
    
    //recebe caracteres no buffer até receber 13 10 ou limite máximo
    len = 0;

    //test a received byte that could mask a timeout event
    do {
        tmp = ser2Inchar();
        if (BufferPcRcv) {
            if ((tmp == 13)||(tmp == 10))
                buffer[len] = '\x00';
            else
                buffer[len++] = tmp;
            contador_timeout_davi = TIMEOUT_VALUE;
        }
        if (contador_timeout_davi == 0) {
            contador_timeout_davi = TIMEOUT_VALUE;
            return BT_TIMEOUT;
        }
    } while ((tmp != 10) && (len < 255));
    
    //try received message
    //compara buffer com as mensagens gravadas
    //retorna 0 ou o indice da mensagem coincidente
    string = buffer;
    for (len = 0; len < BT_maximo; len++) {
        if ((i = strcmp(resposta[len], string)) == 0){
            return len;
        }
    }
    return BT_ERROR;
    }
