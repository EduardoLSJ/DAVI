/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/
#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#endif

#if defined(__XC)
#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#endif

#include "user.h"
#include "md5.h"
#include "crc.h"
#include "uart1.h"
#include "uart2.h"
#include "interrupts.h"
#include "machine_main.h"
#include "machine_horario.h"
#include <EEP.h>
        
/******************************************************************************/
/* Constants initializing                                                     */
/******************************************************************************/
volatile int contador_timeout = TIMEOUT_VALUE;
volatile int contador_PC_timeout = TIMEOUT_PC_VALUE;
volatile char MsgNr = 0;
volatile uint16_t Delta_pulso = 0;
static char PosC_inicial = PosC_LIVRE;
static char PosC = PosC_LIVRE;
static uint32_t Di = 0;
static int erro_armazenado = 0;
union param_ensaio_t param_ensaio;

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/******************************************************************************/
/* Table driven state machine                                                 */
/******************************************************************************/
/**
 * Monitora se:
 *  alguma mensagem válida foi recebida e qual seu valor
 * (mensagem/comando) e gera o evento correspondente
 *
 * @param state
 */
void idle_SMF_main(enum STATE_ID_MAIN state) {

    gera_eventos_mensagens(MachineMainNr);
    /*informa que houve erro na inicialização*/
    if (causa_erro_inicializacao) {
        /*payload de resposta ao erro*/
        causa_erro_inicializacao = FALHA_INEXISTENTE;
        erro_armazenado = ANSWER_ERRO_DADOS_EEPROM;
        event[MachineMainNr] = EVENT_ERRO_INTEGRIDADE;
    }
}

/**
 * verifica se as máquinas de ensaio destravaram
 * 
 * @param state
 */
void mutex_SMF_main(enum STATE_ID_MAIN state) {

    if (current_exclusion[MachineQuilometricoNr] == NOT_OWNED &&
            current_exclusion[MachineWNr] == NOT_OWNED &&
            current_exclusion[MachineHorarioNr] == NOT_OWNED &&
            current_exclusion[MachineCalibracaoNr] == NOT_OWNED &&
            current_exclusion[MachineAjusteNr] == NOT_OWNED) {
        event[MachineMainNr] = EVENT_RELEASE_MAIN;
    }
}

/**
 * esta função faz o que diz: idle
 * 
 * não trata nenhuma variável, apenas é uma transição.
 * 
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void idle_STF_main(enum STATE_ID_MAIN cur_state,
        enum STATE_ID_MAIN new_state, enum EVENT_ID_MAIN new_event){

}

/**
 * Quando solicitada, retransmitir:
 * 1) mensagens de resposta do INSTRUMENTO A3 e A5 para PC
 *
 * esta função completa o modo transparente
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void passagem_STF_main(enum STATE_ID_MAIN cur_state,
        enum STATE_ID_MAIN new_state, enum EVENT_ID_MAIN new_event) {

    ensaio_passagem_INSTR_PC(MachineMainNr);
    
}

/**
 * verifica qual foi o comando recebido do PC e encaminha para o ensaio adequado
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void trata_comando_STF_main(enum STATE_ID_MAIN cur_state,
        enum STATE_ID_MAIN new_state, enum EVENT_ID_MAIN new_event) {

    /*verificar qual o comando solicitado e gerar o evento necessário*/
    event[MachineMainNr] = EVENT_LOCK_MAIN;
    switch (mensagem_Davi.overhead.command) {
        case COMMAND_NOP_DAVI:
            nop_DAVI();
            event[MachineMainNr] = EVENT_RELEASE_MAIN;
            break;
        case COMMAND_HORARIO_DAVI:
            flags_ensaio.MensagemModo = MODO_TRANSPARENTE;
            current_exclusion[MachineHorarioNr] = OWNED;
            break;
        case COMMAND_QUILO_DAVI:
            flags_ensaio.MensagemModo = MODO_TRANSPARENTE;
            current_exclusion[MachineQuilometricoNr] = OWNED;
            break;
        case COMMAND_LEVANTA_W_DAVI:
            flags_ensaio.MensagemModo = MODO_TRANSPARENTE;
            current_exclusion[MachineWNr] = OWNED;
            break;
        case COMMAND_ABORTAR_DAVI:
            event[MachineMainNr] = EVENT_ENSAIO_ABORTAR_MAIN;
            le_mensagem_davi(apaga);
            break;
        case COMMAND_VERSAO_DAVI:
            versao_DAVI();
            break;
        case COMMAND_HASH_DAVI:
            auto_hash_DAVI();
            break;
        case COMMAND_CALIBRACAO_DAVI:
            current_exclusion[MachineCalibracaoNr] = OWNED;
            break;
        case COMMAND_AJUSTE_DAVI:
            current_exclusion[MachineAjusteNr] = OWNED;
            break;
        case COMMAND_NUMERO_SERIE:
            numero_serie_DAVI();
            break;
        default:
            /*responde com comando não valido*/
            erro_armazenado = ANSWER_COMANDO_DAVI;
            event[MachineMainNr] = EVENT_ERRO_MAIN;
            break;
    }
}
/**
 * rotina de passagem de comando, não efetua monitoração
 * 
 * @param state
 */
void none_SMF_main(enum STATE_ID_MAIN state) {

}

void nop_DAVI(){
    
    /*montar o cabeçalho de resposta ao PC*/
    mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
    /*command já preenchido*/
    mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
    /*responde com o parametro disponível*/
    mensagem_Davi.overhead.length = SIZE_ANSWER_NOP_DAVI;

    /*responder para o PC*/
    ensaio_mensagem_envio_PC();
}

/**
 * Rotina para informar a versão de firmware do DAVI
 * 
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void versao_DAVI() {

    /*copiar o número da versão gravada em ROM para o buffer*/
    param_ensaio.VERSAO.versao_atual = 
            endian_shortint(DAVI_VERSAO_ATUAL);

    /*montar o cabeçalho de resposta ao PC*/
    mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
    /*command já preenchido*/
    mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
    /*responde com o parametro disponível*/
    mensagem_Davi.overhead.length = SIZE_ANSWER_COMMAND_VERSAO_DAVI;
    /*copiar o resultado para o buffer de resposta ao PC*/
    mensagem_Davi.valor_int.size_int[0] = param_ensaio.VERSAO.versao_atual;

    /*responder para o PC com o valor calculado*/
    ensaio_mensagem_envio_PC();

    /*sair depois da tarefa*/
    event[MachineMainNr] = EVENT_RELEASE_MAIN; //EVENT_ERRO_MAIN;
}

/**
 * Rotina para calcular o Hash MD5 do intervalo solicitado
 * 
 * É permitido o cálculo de qualquer intervalo dentro da faixa de memória do
 * microcontrolador.
 * Se o pedido solicitar um intervalo acima do valor máximo, este máximo é
 * truncado no valor máximo ROMSIZE.
 * Isso permite a identificação do instrumento através do cálculo do hash de
 * toda a memória. inicial = 0, final = ffffffffffffffff
 * 
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void auto_hash_DAVI() {
    MD5_CONTEXT contextMd5; // Context for MD5
    uint8_t digest[16];//64
    uint8_t i;
    
    /*recebe e armazena os valores dos parametros recebidos pelo comando HASH*/
    for (i = 0; i < SIZE_COMMAND_HASH_DAVI; i++)
        param_ensaio.HASH_A4[i] =
            mensagem_Davi.payload.parameter[i];
    
    param_ensaio.HASH.end_ini =
            endian_big_to_little(param_ensaio.HASH.end_ini);
    param_ensaio.HASH.end_final =
            endian_big_to_little(param_ensaio.HASH.end_final);

    /*verifica se o valor de endereçamento é válido*/
    if ((param_ensaio.HASH.end_ini_zero == 0 &&
            param_ensaio.HASH.end_final >=
            param_ensaio.HASH.end_ini &&
            param_ensaio.HASH.end_final_zero == 0 &&
            param_ensaio.HASH.end_final <=
            (uint32_t) (_ROMSIZE - 1)) ||
            /*condição de teste em toda a memoria de programa (universal)*/
            (param_ensaio.HASH.end_ini_zero == 0 &&
            param_ensaio.HASH.end_ini == 0 &&
            param_ensaio.HASH.end_final_zero == 0xFFFFFFFF &&
            param_ensaio.HASH.end_final == 0xFFFFFFFF)) {
    } else {
        /*valor do comando inválido, sai do ensaio*/
        event[MachineMainNr] = EVENT_ERRO_MAIN;
        /*payload de resposta ao erro*/
        erro_armazenado = ANSWER_VALOR_DAVI;
        return;
    }
    /*corrige o valor máximo de cálculo. 
     * Em caso de identificação universal de hardware*/
    if(param_ensaio.HASH.end_final_zero){
        param_ensaio.HASH.end_final = 0;
        param_ensaio.HASH.end_final = _ROMSIZE - 1;
    }
        
    /*carrega o valor inicial de memoria*/
    MD5_Initialize(&contextMd5);
    /*solicita calculo de hash no intervalo solicitado*/
    MD5_ROMDataAdd(&contextMd5, (const uint8_t *) param_ensaio.HASH.end_ini,
            (param_ensaio.HASH.end_final -
            param_ensaio.HASH.end_ini + 1));
    /*encerra calculo de hash*/
    MD5_Calculate(&contextMd5, digest);
    /*concatenar o valor hash para o buffer*/
    for (i = 0; i < 16; i++) {
        param_ensaio.HASH.digest[i] = digest[i];
    }
    
    /*montar o cabeçalho de resposta ao PC*/
    mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
    /*command já preenchido*/
    mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
    /*responde com o parametro disponível*/
    mensagem_Davi.overhead.length = SIZE_ANSWER_COMMAND_HASH_DAVI;
    /*copiar o resultado para o buffer de resposta ao PC*/
    for (i = 0; i < SIZE_ANSWER_COMMAND_HASH_DAVI; i++) {
        mensagem_Davi.payload.parameter[i] =
                param_ensaio.HASH_A6[i];
    }

    /*responder para o PC com o valor calculado*/
    ensaio_mensagem_envio_PC();

    /*sair depois da tarefa*/
    event[MachineMainNr] = EVENT_RELEASE_MAIN;
}

void numero_serie_DAVI(){
    uint8_t i;

    /*copiar o número de séie gravada em EEPROM para o buffer*/
    Busy_eep();
    param_ensaio.SERIE_A6[0] = Read_b_eep(EE_NUM_SERIE);
    param_ensaio.SERIE_A6[1] = Read_b_eep(EE_NUM_SERIE + 1);
    
    /*montar o cabeçalho de resposta ao PC*/
    mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
    /*command já preenchido*/
    mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
    /*responde com o parametro disponível*/
    mensagem_Davi.overhead.length = SIZE_ANSWER_COMMAND_NUMERO_SERIE;
    /*copiar o resultado para o buffer de resposta ao PC*/
    for (i = 0; i < SIZE_ANSWER_COMMAND_NUMERO_SERIE; i++) {
        mensagem_Davi.payload.parameter[i] =
                param_ensaio.SERIE_A6[i];
    }

    /*responder para o PC com o valor armazenado*/
    ensaio_mensagem_envio_PC();

    /*sair depois da tarefa*/
    event[MachineMainNr] = EVENT_RELEASE_MAIN;
}

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
 * @param cur_state
 * @param new_state
 * @param new_event Evento recebido (EVENT_OCUPADO_MAIN, EVENT_OCUPADO_DELTA_MAIN,
 *  EVENT_RECEBE_COMANDO_PC_MAIN, EVENT_ERRO_MAIN, EVENT_ENSAIO_ABORTAR_MAIN,
 *  EVENT_TIMEOUT_MAIN, EVENT_RECEBE_ERRO_INSTRUMENTO_MAIN)
 */
void erro_STF_main(enum STATE_ID_MAIN cur_state,
        enum STATE_ID_MAIN new_state, enum EVENT_ID_MAIN new_event) {

    erro_handler(MachineMainNr, &new_event, &erro_armazenado);
}

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
void erro_handler(enum MachineNr MachineCaller, enum EVENT_ID_MAIN *new_event, int *erro_armazenado) {
    /*
     * fontes de ação na entrada:
     * 1 - erro :
     *      responde com o erro_armazenado todos os comandos.
     *      aguarda ABORTAR para sair do estado
     *      timeout sai imediatamente, sem exportar mensagem
     * 2 - abortar :
     *      responde ao comando, com resposta ok
     *      sai imediatamente
     */

    switch (*new_event) {
        case EVENT_RECEBE_COMANDO_PC_MAIN:
            /*reentrada enquanto aguarda comando ABORTAR*/
            if (mensagem_Davi.overhead.command ==
                    COMMAND_ABORTAR_DAVI) {
                event[MachineCaller] = EVENT_RELEASE_MAIN;
                /* responder A6 */
                /*responde com o cabeçalho do comando ENSAIO_ABORTAR*/
                mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
                /*command já preenchido*/
                mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
                mensagem_Davi.overhead.length = SIZE_ANSWER_PC_ABORTAR;
            } else {
                /*retorna em idle, para mensagens pergunta-resposta*/
                if (mensagem_Davi.overhead.command == COMMAND_HASH_DAVI)
                    event[MachineCaller] = EVENT_RELEASE_MAIN;
                /*verifica se há erro pendente e*/
                /*monta a mensagem de erro pro PC*/
                /*responder A5 */
                mensagem_Davi.overhead.head = STX_ERROR_DAVI;
                mensagem_Davi.overhead.command = COMMAND_NOP_DAVI;
                mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
                mensagem_Davi.overhead.length = SIZE_ANSWER_ERRO_DAVI;
                /*payload definido antes de entrar na rotina de erro*/
                mensagem_Davi.valor_int.size_int[0] =
                        endian_shortint(*erro_armazenado);
            }
            ensaio_mensagem_envio_PC();
            break;
        case EVENT_ERRO_INTEGRIDADE:
        case EVENT_ERRO_MAIN:
            encerra_ensaio();
            /*responde imediatamente ao erro de parâmetro*/
            event[MachineCaller] = EVENT_RECEBE_COMANDO_PC_MAIN;
            if (*erro_armazenado == ANSWER_ERRO_PC_TIMEOUT_MAIN)
                /*sai imediatamente com timeout PC*/
                event[MachineCaller] = EVENT_RELEASE_MAIN;
            break;
        case EVENT_ENSAIO_ABORTAR_MAIN:
            encerra_ensaio();
            /* responder A6 */
            /*responde com o cabeçalho do comando ENSAIO_ABORTAR*/
            mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
            /*command já preenchido*/
            mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
            mensagem_Davi.overhead.length = SIZE_ANSWER_PC_ABORTAR;
            ensaio_mensagem_envio_PC();
            /*saida imediata*/
            event[MachineCaller] = EVENT_RELEASE_MAIN;
            break;
        case EVENT_RECEBE_MENSAGEM_INSTRUMENTO_MAIN:
        case EVENT_RECEBE_ERRO_INSTRUMENTO_MAIN:
            /*repassa mensagens do instrumento*/
            ensaio_passagem_INSTR_PC(MachineCaller);
            event[MachineCaller] = EVENT_NONE_MAIN;
            break;
        default:
            break;
    }
}

/**
 * le as mensagens recebidas do INSTRUMENTO
 *
 * @param flag_main.ModoOpr, indica MODO DE OPERAÇÃO (horário/quilometrico)
 * @param PosC, (00h=LIVRE, 01...7Fh=TARIFA, 80h=A PAGAR, 81h=VERIFICAÇÃO)
 * @param Di, valor do DISPOSITIVO INDICADOR
 */
void le_mensagem(flags_struct_ensaio *flag_main, char* PosC, uint32_t* Di) {
    switch (mensagem_393.overhead.command) {
        case COMMAND_MO_393:
            flag_main->ModoOpr = (bool) mensagem_393.payload.parameter[0];
            break;
        case COMMAND_PosC_393:
            *PosC = mensagem_393.payload.parameter[0];
            break;
        case COMMAND_DI_393:
            /*converter valor de entrada, se estiver em BCD*/
            *Di = endian_big_to_little(mensagem_393.valor.size_32[0]);
            if (mensagem_393.overhead.format == FORMAT_BCD_393) {
                *Di = Converter_BCD_to_HEX(*Di);
            }
            break;
        default:
            break;
    }
}

/**
 * função para criar a mensagem de saída para o PC
 *
 * Antes de chamar a função, os parâmetros de envio deve ser previamente
 * carregados no cabeçalho da mensagem, assim como o payload da mensagem
 */
void ensaio_mensagem_envio_PC(void) {
    unsigned short crc;

    crc = generate_16bit_crc(mensagem_Davi.buffer,
            sizeof (mensagem_Davi.overhead) + mensagem_Davi.overhead.length,
            CRC_16);
    mensagem_Davi.payload.parameter[mensagem_Davi.overhead.length]
            = *((char*) (&crc) + 1);
    mensagem_Davi.payload.parameter[mensagem_Davi.overhead.length + 1]
            = *((char*) (&crc) + 0);
    ser2Outstring(mensagem_Davi.buffer,
            sizeof (mensagem_Davi.overhead) + mensagem_Davi.overhead.length +
            sizeof (crc));
    le_mensagem_davi(apaga);
}

/**
 * função para criar a mensagem de saída para o INSTRUMENTO
 *
 * Antes de chamar a função, os parâmetros de envio devem ser previamente
 * carregados no cabeçalho da mensagem, assim como o payload da mensagem
 */
void ensaio_mensagem_envio_INSTR(void) {
    unsigned short crc;

    crc = generate_16bit_crc(mensagem_393.buffer,
            sizeof (mensagem_393.overhead) + mensagem_393.overhead.length,
            CRC_16);
    mensagem_393.payload.parameter[mensagem_393.overhead.length]
            = *((char*) (&crc) + 1);
    mensagem_393.payload.parameter[mensagem_393.overhead.length + 1]
            = *((char*) (&crc) + 0);
    serOutstring(mensagem_393.buffer,
            sizeof (mensagem_393.overhead) + mensagem_393.overhead.length +
            sizeof (crc));
}

/**
 * repassa o comando recebido do INSTRUMENTO e encaminha para o PC
 */
void ensaio_passagem_INSTR_PC(enum MachineNr MachineNr){
    uint8_t mensagem_tamanho;
    
    /*evento de retorno*/
    event[MachineNr] = EVENT_ENSAIO_NOP_MAIN;

    mensagem_tamanho = mensagem_393.overhead.length
            + sizeof (mensagem_393.overhead)
            + sizeof (unsigned short);
    ser2Outstring(mensagem_393.buffer, mensagem_tamanho);
    /*após tratar a mensagem, apague do buffer e libere nova recepção*/
    le_mensagem_393(apaga);
}

/**
 * Monitora as mensagens na camada de comunicação
 * 
 * em caso de mensagens novas ou comunição de erro gera os eventos adequados
 * 
 */
void gera_eventos_mensagens(enum MachineNr MachineCaller){

    /*sai, há evento pendente na fila*/
    if (event[MachineCaller])
        return;
    /* verifica se recebeu nova mensagem do PC*/
    if (le_mensagem_davi(nao_apaga)) {
        /* A4 seguido de comando, analisar.*/
        if (mensagem_Davi.overhead.head == STX_MASTER_MOTOVER_DAVI) {
            event[MachineCaller] = EVENT_RECEBE_COMANDO_PC_MAIN;
        }
        contador_PC_timeout = TIMEOUT_PC_VALUE;
    }/* verifica se recebeu nova mensagem do instrumento*/
    else if (le_mensagem_393(nao_apaga)) {
        /* A3 ou A5, encaminhar para o PC*/
        if (mensagem_393.overhead.head == STX_INSTRUMENTO_393)
            event[MachineCaller] = EVENT_RECEBE_MENSAGEM_INSTRUMENTO_MAIN;
        else if (mensagem_393.overhead.head == STX_ERROR_393)
            event[MachineCaller] = EVENT_RECEBE_ERRO_INSTRUMENTO_MAIN;
        contador_timeout = TIMEOUT_VALUE;
    }
}

/**
 * Encerra o ensaio, antes de desativar a maquina de estado
 * 
 * Os geradores de pulso são desligados
 * O DAVI volta a permitir a passagem de mensagens
 * O buffer de mensagens antigas é limpo
 */
void encerra_ensaio(void) {
    /*encerra a atividade*/
    ConfigPulseIn(false);
    ConfigPulseOutOff();
    /*limpa buffers*/
    le_mensagem_393(apaga);
    le_mensagem_davi(apaga);
    /*volta a permitir PC enviar comandos pro INSTRUMENTO*/
    flags_ensaio.MensagemModo = MODO_TRANSPARENTE;
    /*desliga modo levanta W*/
    flag_int.levanta_w_mode = false;
}