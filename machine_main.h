/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef _machine_main_h
#define _machine_main_h

#include "user.h"
#include "machine_prot_davi.h"
#include "machine_prot_393.h"

/******************************************************************************/
/* User Level #define Macros                                                  */
/******************************************************************************/
#define max_state_tbl_main_len sizeof(state_tbl_main)/sizeof(state_tbl_main[0])

#define apaga true
#define nao_apaga false
#define MODO_ENSAIO true
#define MODO_TRANSPARENTE false
#define ascendente true
#define descendente false

/*determinado por 7.3.1 alínea d*/
#define D_PISTA_MIN 12500L

/*determinado por 8.1.2 alínea i*/
#define D_ENSAIO_MAX 200000L
#define D_ENSAIO_MIN 10000L
#define K_MIN 12000L

typedef enum EVENT_ID_MAIN {
    EVENT_NONE_MAIN,
    EVENT_RECEBE_MENSAGEM_INSTRUMENTO_MAIN,
    EVENT_RECEBE_ERRO_INSTRUMENTO_MAIN,
    EVENT_RECEBE_COMANDO_PC_MAIN,
    EVENT_LOCK_MAIN,
    EVENT_RELEASE_MAIN,
    EVENT_ENSAIO_ABORTAR_MAIN,
    EVENT_TIMEOUT_MAIN,
    EVENT_ERRO_MAIN,
    EVENT_ENSAIO_NOP_MAIN,
    EVENT_ERRO_INTEGRIDADE
};

typedef enum STATE_ID_MAIN {
    STATE_IDLE_MAIN,
    STATE_ERRO_MAIN,
    STATE_RECEBE_COMANDO_PC_MAIN,
    STATE_OWNED_MAIN,
};

enum ANSWER_ERRO_MAIN {
    ANSWER_ERRO_NULO_MAIN = ANSWER_OCUPADO_DAVI,
    ANSWER_ERRO_INST_TIMEOUT_MAIN,
    ANSWER_ERRO_PC_TIMEOUT_MAIN,
    ANSWER_ERRO_INST_SEQUENCIA_MAIN,
    ANSWER_ERRO_INST_MODO_MAIN,
    ANSWER_ERRO_INST_POSC_MAIN,
    ANSWER_ERRO_INST_FRACAO_MAIN,
    ANSWER_ERRO_INST_VT_MAIN,
    ANSWER_ERRO_INST_DIST_MAIN,
    ANSWER_ERRO_INST_VMAX_MAIN,
    ANSWER_ERRO_HASH_EEPROM_FREE,
    ANSWER_ERRO_DADOS_EEPROM,
    ANSWER_ERRO_APP_ROM,
    ANSWER_ERRO_BOOT_ROM,
    ANSWER_ERRO_COMM_HELPER,
};

union param_ensaio_t { /*OS VALORES ESTÃO EM BIG-ENDIAN (MSB para LSB)*/

    struct {
        /*parametros recebidos e utilizados no ensaio de W*/
        uint16_t D_PISTA; /*distância da pista reduzida em decímetros*/
        uint16_t K_ENSAIO; /*K informado pelo PC, para ensaio*/

        /*parametros encaminhados para PC*/
        uint32_t N_PULSO; /*numero de pulsos recebidos no ensaio*/
    } LEV_W;

    struct {/*valores recebidos do PC no ensaio de W*/
        char W_A4[SIZE_PC_LEVANTA_W]; /*distancia a percorrer na pista reduzida*/
    };

    struct { /*valores de resposta ao comando ensaio W*/
        char unused[SIZE_PC_LEVANTA_W];
        char W_A6[SIZE_ANSWER_PC_LEVANTA_W];
    };

    struct {
        /*parametros recebidos e utilizados no ensaio de modo quilométrico*/
        uint32_t D_ENSAIO; /*distância a percorrer, na velocidade de ensaio, em decímetros*/
        char V_ENSAIO; /*Velocidade desejada para realizar o ensaio*/
        uint16_t K_ENSAIO; /*K informado pelo PC, para ensaio*/
        char VT_TEORICO; /*velocidade de transição calculada pelo PC*/

        /*parametros encaminhados para PC*/
        uint32_t IND_INICIAL; /*indicação monetária na mudança de fração,
        em modo Quilometrico e na velocidade de ensaio*/
        uint32_t IND_FINAL; /*Indicação monetária na distância percorrida de ensaio*/
        uint32_t IND_FINAL_FRACAO; /*Indicação monetária na mudança de fração*/
        uint32_t D_FINAL_FRACAO; /*distância percorrida na mudança de fração RTM 8.3.2.c*/
        char V_ATUAL; /*velocidade atual gerada pelo DAVI*/
        char VT_REAL; /*Velocidade de transição, na mudança de modo Horario/Quilometrico*/
    } QUILO;

    struct { /*valores recebidos do PC no ensaio de modo quilometrico*/
        char QUILO_A4[SIZE_PC_QUILO];
    };

    struct { /*valores de resposta ao comando DIST no modo quilometrico*/
        char unused[SIZE_PC_QUILO];
        char QUILO_A6[SIZE_ANSWER_PC_QUILO];
    };

    struct {
        /*parâmetro recebido do PC para ensaio em modo Horário*/
        uint16_t N_FRACOES;

        /*parâmetros encaminhados para o PC no ensaio horário*/
        uint32_t TEMPO_32; /*intervalo de tempo total das frações (us)
                                 * registra até 1h 11 min e 34 s
                                 Item 8.12 alínea j: tempo de até 30 minutos*/
        uint32_t IND_INICIAL; /*indicação monetária no INICIO da contagem de tempo*/
        uint32_t IND_FINAL; /*Indicação monetária no FINAL da contagem de tempo*/
        uint16_t CONT_FRACOES; /*número sequencial de fração, que identifica*/
        /*o resultado apresentado nos parâmetros acima*/
    } HORARIO;

    struct { /*valor recebido do PC no ensaio de modo horário*/
        char HORARIO_A4[SIZE_PC_HORARIO];
    };

    struct { /*valores de resposta ao comando TEMPO no modo horário*/
        char unused[SIZE_PC_HORARIO];
        char HORARIO_A6[SIZE_ANSWER_PC_HORARIO];
    };

    struct {/*valores recebidos do PC no comando VERSAO*/
        char VERSAO_A4[SIZE_COMMAND_VERSAO_DAVI];
    };
    
    struct {/*valores de resposta ao comando VERSAO*/
        /*parametros recebidos e utilizados no comando VERSAO*/
        uint8_t Identificador;
        
        /*parametro encaminhado para o PC*/
        uint16_t versao_atual;
    } VERSAO;

    struct {/*valores de resposta ao comando VERSAO*/
        uint8_t unused[SIZE_COMMAND_VERSAO_DAVI];
        char VERSAO_A6[SIZE_ANSWER_COMMAND_VERSAO_DAVI];
    };

    struct { /*valores recebidos do PC no comando HASH*/
        char HASH_A4[SIZE_COMMAND_HASH_DAVI];
    };

    struct {
        /*parametros recebidos e utilizados no comando HASH*/
        uint8_t identificador;
        uint32_t end_ini_zero;
        uint32_t end_ini;
        uint32_t end_final_zero;
        uint32_t end_final;

        /*parametros encaminhados para PC*/
        uint8_t digest[SIZE_ANSWER_COMMAND_HASH_DAVI]; /*valor de HASH calculado */
    } HASH;
    
    struct { /*valores de resposta ao comando HASH*/
        char unused[SIZE_COMMAND_HASH_DAVI];
        char HASH_A6[SIZE_ANSWER_COMMAND_HASH_DAVI];
    };

    struct {/*valores de resposta ao comando NUMERO SERIE*/
        uint16_t numero_serie;
    } SERIE;

    struct {/*valores de resposta ao comando NUMERO SERIE*/
        /** cópia do número de série em forma de array de bytes
     */
        uint8_t SERIE_A6[SIZE_ANSWER_COMMAND_NUMERO_SERIE];
    };

    struct { /*parâmetros recebidos no comando calibração*/
        uint8_t V_ENSAIO; /*Velocidade desejada para realizar o ensaio*/
        uint16_t K_ENSAIO; /*K informado pelo PC, para ensaio*/

        /*parametros encaminhados para PC*/
        uint32_t TEMPO_32; /*último intervalo de tempo registrado*/
        uint32_t N_PULSO; /*número de pulsos no intervalo registrado*/
        uint32_t N_JANELAS_IN;/*número de janelas de tempo  medido*/
    } CALIBRA;
    
    struct { /*valores recebidos do PC*/
        uint8_t CALIBRA_A4[SIZE_COMMAND_CALIBRACAO_DAVI];
    };
    
    struct { /*valores de resposta ao comando calibracao*/
        char unused[SIZE_COMMAND_CALIBRACAO_DAVI];
        char CALIBRA_A6[SIZE_ANSWER_COMMAND_CALIBRACAO_DAVI];
    };
    
    struct { /*parâmetros recebidos no comando ajuste*/
        char V_ENSAIO; /*Velocidade desejada para realizar o ensaio*/
        uint16_t K_ENSAIO; /*K informado pelo PC, para ensaio*/
    }AJUSTE;
};

const char LISTA_COMMAND_ENSAIO_HORARIO_393[] = {
    COMMAND_NOP_393, /*mantém instrumento em operação*/
    COMMAND_MO_393, /*Leitura do modo de operacao*/
    COMMAND_PosC_393, /*Leitura da Posicao de comando*/
    COMMAND_DI_393, /*Leitura do Dispositivo Indicador*/
};

extern union param_ensaio_t param_ensaio;
extern volatile char MsgNr;
extern volatile int contador_timeout;
extern volatile int contador_PC_timeout;
extern volatile uint16_t Delta_pulso;

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
void mutex_SMF_main(enum STATE_ID_MAIN state);
void idle_SMF_main(enum STATE_ID_MAIN state);
void idle_STF_main(enum STATE_ID_MAIN cur_state,
        enum STATE_ID_MAIN new_state, enum EVENT_ID_MAIN new_event);
void passagem_STF_main(enum STATE_ID_MAIN cur_state,
        enum STATE_ID_MAIN new_state, enum EVENT_ID_MAIN new_event);
void trata_comando_STF_main(enum STATE_ID_MAIN cur_state,
        enum STATE_ID_MAIN new_state, enum EVENT_ID_MAIN new_event);
void none_SMF_main(enum STATE_ID_MAIN state);
void erro_STF_main(enum STATE_ID_MAIN cur_state,
        enum STATE_ID_MAIN new_state, enum EVENT_ID_MAIN new_event);


void ensaio_passagem_INSTR_PC(enum MachineNr MachineNr);
/**
 * Responde ao PC com as mensagens de erro
 *
 * Ao receber erros do INSTRUMENTO ou TIMEOUT:
 *   carregar o valor de erro correspondente
 *   encerrar o ensaio
 *   aguarda contato do PC para responder com o erro armazenado
 *
 * Ao receber comando ABORTAR, NOVO comando ou comando com valor inválido:
 *   responder imediatamente ao comando ABORTAR;
 *   responder imediatamente com OCUPADO
 *   responder imediatamante ao comando inválido com resposta VALOR_INVALIDO
 * 
 * chamada:
 *      erro_STF(MachineMainNr, &new_event, &erro_armazenado);
 * 
 * @param MachineNr, maquina atual
 * @param new_event, evento 
 * @param erro_armazenado, erro de chamada
 */
void erro_handler(enum MachineNr MachineCaller, enum EVENT_ID_MAIN *new_event, int *erro_armazenado);
void ensaio_mensagem_envio_PC(void);
void ensaio_mensagem_envio_INSTR(void);
/**
 * Rotina para resposta ao comando NOP
 */
void nop_DAVI(void);
/**
 * Rotina para informar a versão de firmware do DAVI
 * 
 * O retorno do comando pedido de versão é:
 * STX COMM Format Bytes <8> <versao> <MD5> CRC16
 * 
 * O calculo de MD5 inclui toda a memória disponível do DAVI
 */
void versao_DAVI(void);
/**
 * Rotina para calcular o hash MD5 do intervalo silicitado
 * 
 * Avalia se a faixa é válida, dentro da memória de programa disponível.
 * A faixa deve ser maior que zero e com tamanho maximo de 0x8000
 * 
 * @param end_ini endereço inicial da faixa de memória flash
 * @param end_fin edndereço final da faixa de memória flash
 */
void auto_hash_DAVI(void);
/**
 * Rotina para resposta com o numero de serie do DAVI
 * 
 * o número possui 2 bytes e é armazenado na EEPROM.
 */
void numero_serie_DAVI(void);
void gera_eventos_mensagens(enum MachineNr MachineCaller);
void le_mensagem(flags_struct_ensaio *flag_main, char* PosC, uint32_t* Di);
/**
 * Encerra o ensaio, antes de desativar a maquina de estado
 * 
 * Os geradores de pulso são desligados
 * O DAVI volta a permitir a passagem de mensagens
 * O buffer de mensagens antigas é limpo
 */
void encerra_ensaio(void);

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
const struct STATE_DESCRIPTION
state_tbl_main[] = {
    /*eventos do estado inicial da máquina*/
    {
        STATE_IDLE_MAIN,
        STATE_IDLE_MAIN,
        EVENT_ENSAIO_NOP_MAIN,
        idle_STF_main,
        idle_SMF_main
    },
    {
        STATE_IDLE_MAIN,
        STATE_IDLE_MAIN,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_MAIN,
        passagem_STF_main,
        idle_SMF_main
    },
    {
        STATE_IDLE_MAIN,
        STATE_IDLE_MAIN,
        EVENT_RECEBE_ERRO_INSTRUMENTO_MAIN,
        passagem_STF_main,
        idle_SMF_main
    },
    {
        STATE_IDLE_MAIN,
        STATE_RECEBE_COMANDO_PC_MAIN,
        EVENT_RECEBE_COMANDO_PC_MAIN,
        trata_comando_STF_main,
        none_SMF_main
    },
    {
        STATE_RECEBE_COMANDO_PC_MAIN,
        STATE_OWNED_MAIN,
        EVENT_LOCK_MAIN,
        idle_STF_main,
        mutex_SMF_main
    },
    {
        STATE_OWNED_MAIN,
        STATE_IDLE_MAIN,
        EVENT_RELEASE_MAIN,
        idle_STF_main,
        idle_SMF_main
    },
    /*evento de erro na inicialização*/
    {/*TODO registrar este novo evento no astah*/
        STATE_IDLE_MAIN,
        STATE_ERRO_MAIN,
        EVENT_ERRO_INTEGRIDADE,
        erro_STF_main,
        idle_SMF_main
    },
    /*eventos de saida da maquina de estado MUTEX*/
    {
        STATE_RECEBE_COMANDO_PC_MAIN,
        STATE_IDLE_MAIN,
        EVENT_RELEASE_MAIN,
        idle_STF_main,
        idle_SMF_main
    },
    {
        STATE_RECEBE_COMANDO_PC_MAIN,
        STATE_ERRO_MAIN,
        EVENT_ENSAIO_ABORTAR_MAIN,
        erro_STF_main,
        idle_SMF_main
    },
    {
        STATE_RECEBE_COMANDO_PC_MAIN,
        STATE_ERRO_MAIN,
        EVENT_ERRO_MAIN,
        erro_STF_main,
        idle_SMF_main
    },
    {
        STATE_ERRO_MAIN,
        STATE_ERRO_MAIN,
        EVENT_ERRO_MAIN,
        erro_STF_main,
        idle_SMF_main
    },
    {
        STATE_ERRO_MAIN,
        STATE_ERRO_MAIN,
        EVENT_RECEBE_COMANDO_PC_MAIN,
        erro_STF_main,
        idle_SMF_main
    },
    {
        STATE_ERRO_MAIN,
        STATE_ERRO_MAIN,
        EVENT_RECEBE_MENSAGEM_INSTRUMENTO_MAIN,
        erro_STF_main,
        idle_SMF_main
    },
    {
        STATE_ERRO_MAIN,
        STATE_ERRO_MAIN,
        EVENT_RECEBE_ERRO_INSTRUMENTO_MAIN,
        erro_STF_main,
        idle_SMF_main
    },
    {
        STATE_ERRO_MAIN,
        STATE_IDLE_MAIN,
        EVENT_RELEASE_MAIN,
        idle_STF_main,
        idle_SMF_main
    }
};
#endif
