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
#include "uart2.h"
#include "interrupts.h"
#include "state_machine.h"
#include "machine_prot_393.h"
#include "machine_main.h"
#include "machine_w.h"


/******************************************************************************/
/* Constants initializing                                                     */
/******************************************************************************/
static char PosC_inicial = PosC_LIVRE;
static char PosC = PosC_LIVRE;
static uint32_t Di = 0;
static int erro_armazenado = 0;

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/
/**
 *  Libera a máquina atual de execução
 * 
 * @param state
 */
void release_STF_w(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event){
    current_exclusion[MachineWNr] = NOT_OWNED;
}

/**
 * inicia a execução de tarefas desta máquina
 * 
 * @param state
 */
void mutex_SMF_w(enum STATE_ID_W state){
    if (current_exclusion[MachineWNr] == OWNED) {
        event[MachineWNr] = EVENT_LOCK_W;
    }
}

/**
 * Monitora se:
 *  alguma mensagem válida foi recebida e gera o evento correspondente
 *
 * @param state
 */
void idle_SMF_w(enum STATE_ID_W state) {
        gera_eventos_mensagens(MachineWNr);
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
void idle_STF_w(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event){

}

/**
 * rotina de passagem, não efetua monitoração
 * 
 * @param state
 */
void none_SMF_w(enum STATE_ID_W state) {

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
void passagem_STF_w(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_MAIN new_event) {

    ensaio_passagem_INSTR_PC(MachineWNr);
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
void erro_STF_w(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event) {

    erro_handler(MachineWNr, &new_event, &erro_armazenado);
}



/**
 * Rotina para determinar o valor de W real do veículo.
 *
 *    É necessário intervenção do metrologista para deslocar o veículo sobre a
 * rampa da pista reduzida.
 *    O usuário deve reiniciar a contagem de pulsos no inicio da pista. Eviden-
 * ciado por uma indicação 0 no DI.
 *    PC ao verificar que o valor do DI for reiniciado, pode-se iniciar a contagem de deslo-
 * camento na pista.
 *    O metrologista deve deslocar o veículo com uma velocidade entre 0.5 km/h a
 * 10km/h durante o percurso da pista reduzida.
 *
 *    Verificar se o valor de n_pulsos incrementa a cada 1 segundo.
 * se n for menor que anterior, deve indicar erro e sair da verificação.
 *    Para calculo estimado de velocidade máxima 10 km/h, ler valor de W do
 * intrumento e calcular o intervalo mínimo de tempo cada pulso.
 *    Se não houver incremento de pulsos em 1 segundo, interromper o incremento
 * de pulsos. o PC vai acusar erro de W, que terá valor diferente do armazenado
 * no instrumento.
 *
 *    Quando a velocidade é menor que 0.5 km/h é verificado se houve mudança do
 * estado do mototaxímetro (MODO DE VERIFICAÇÃO para o MODO LIVRE).
 *    Em caso de mudança para o MODO LIVRE o ensaio é encerrado.
 *    Para Wmin = 12.000 pulsos/km:
 *       V = 10 km/h -> DeltaT = 30 ms/pulso
 *       v = 3 km/h -> DeltaT = 100 ms/pulso
 *       v = 0.5 km/h -> DeltaT = 600 ms/pulso
 *   Determina-se que se em 1 segundo não ocorreu um pulso de entrada, estima-se
 * que o veículo está parado e começa a arguição de mudança de MODO do taxímetro
 *
 *    A leitura do dispositivo indicador (DI) confirma o número de pulsos rece-
 * bido pelo sensor de entrada (pulse_in).
 *
 *
 *    A rotina ensaio_levanta_w_STF_main habilita as entradas de interrupção
 * para:
 *
 * pulso recebido por mudança velocidade de transição
 * pulso recebido por mudança no dispositivo indicador
 * pulso recebido por outro evento, que deve gerar erro no ensaio.
 * timeout para mensagens de comunicação
 * A VERIFICAÇÃO SE HOUVE MUDANÇA NO ESTADO É RESPONSABILIDADE
 * DO PC, ATRAVÉS DE COMANDO DE LEITURA DO DISPOSITIVO INDICADOR
 * E COMANDO DE POSIÇÃO DE COMANDO E MODO DE OPERAÇÃO
 *
 *
 * habilita a entrada de pulsos.
 * na entrada de levantamento de W, reinicia contadores.
 * nos comandos repetitivos, responde ao PC e continua a contagem
 *
 *  *
 * OPÇÃO PARA IMPLEMENTAR:
 * calcula o valor de W do veículo, com a formula:
 * W = (pulsos * 1000)/distancia_pista
 *
 * responde o comando com o valor de W calculado.
 * 
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void ensaio_levanta_w_STF(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_W new_event) {

    char i;

    /*Ao primeiro comando, reinicia, recebe e armazena os parametros recebidos*/
    for (i = 0; i < SIZE_PC_LEVANTA_W; i++)
        param_ensaio.W_A4[i] =
            mensagem_Davi.payload.parameter[i];

    /*corrige tamanho da pista de ensaio*/
    param_ensaio.LEV_W.D_PISTA
            = endian_shortint(param_ensaio.LEV_W.D_PISTA);
    /*converter valor de entrada, se estiver em BCD*/
    if (mensagem_Davi.overhead.format == FORMAT_BCD_DAVI) {
        param_ensaio.LEV_W.D_PISTA =
                Converter_BCD_to_HEX((uint32_t) param_ensaio.LEV_W.D_PISTA);
    }
    /*verifica se valor é válido*/
    if (param_ensaio.LEV_W.D_PISTA < D_PISTA_MIN) {
        /*valor do comando inválido, sai do ensaio*/
        event[MachineWNr] = EVENT_ERRO_W;
        /*payload de resposta ao erro*/
            erro_armazenado = ANSWER_VALOR_DAVI;
        return;
    }

    /*corrige valor de K no ensaio. para determinar v_maxima*/
    param_ensaio.LEV_W.K_ENSAIO
            = endian_shortint(param_ensaio.LEV_W.K_ENSAIO);
    /*converter valor de entrada, se estiver em BCD*/
    if (mensagem_Davi.overhead.format == FORMAT_BCD_DAVI) {
        param_ensaio.LEV_W.K_ENSAIO =
                Converter_BCD_to_HEX((uint32_t) param_ensaio.LEV_W.K_ENSAIO);
    }
    /*verifica se valor é válido*/
    if (param_ensaio.LEV_W.K_ENSAIO < K_MIN) {
        /*valor do comando inválido, sai do ensaio*/
        event[MachineWNr] = EVENT_ERRO_W;
        /*payload de resposta ao erro*/
            erro_armazenado = ANSWER_VALOR_DAVI;
        return;
    }

    /*reinicia contadores*/
    ConfigPulseIn(false);
    odometro_in = 0;
    param_ensaio.LEV_W.N_PULSO = 0;
    Delta_pulso = param_ensaio.LEV_W.N_PULSO;
    ConfigPulseIn(true);
    flag_int.tempo_fracao = false;
    /*contar borda de subida e borda de descida*/
    flag_int.levanta_w_mode = true;

    /*responder ao comando, indica que iniciou contagem*/
    event[MachineWNr] = EVENT_RECEBE_COMANDO_PC_W;
}
/**
 *  * Rotina de resposta de comandos recebidos pelo PC
 *
 * verifica mensagens do PC
 * se receber comando:
 *  EVENT_RECEBE_COMANDO_PC_MAIN, analisa:
 *      se abortar, EVENT_ENSAIO_ABORTAR_MAIN
 *          passa para estado de saída/erro
 *      se ensaio lev W, EVENT_ENSAIO_NOP_MAIN
 *          responder com valor pulsos atual
 *      se outro comando, EVENT_ENSAIO_NOP_MAIN
 *          responder com ocupado
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void ensaio_levanta_w_comando_STF(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_MAIN new_event) {

    /*evento de retorno*/
    event[MachineWNr] = EVENT_ENSAIO_NOP_W;
    switch (mensagem_Davi.overhead.command) {
        case COMMAND_ABORTAR_DAVI:
            event[MachineWNr] = EVENT_ENSAIO_ABORTAR_W;
            break;
        case COMMAND_LEVANTA_W_DAVI:
            mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
            /*command já preenchido*/
            mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
            /*responde com o parametro disponível*/
            mensagem_Davi.overhead.length = SIZE_ANSWER_PC_LEVANTA_W;
            mensagem_Davi.valor_long.size_32[0] =
                    endian_big_to_little(param_ensaio.LEV_W.N_PULSO);
            ensaio_mensagem_envio_PC();
            break;
        default:
            /*outros comandos, recebe erro ANSWER_OCUPADO_DAVI*/
            mensagem_Davi.valor_int.size_int[0] =
                    endian_shortint(ANSWER_OCUPADO_DAVI);
            mensagem_Davi.overhead.head = STX_ERROR_DAVI;
            mensagem_Davi.overhead.command = COMMAND_NOP_DAVI;
            mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
            mensagem_Davi.overhead.length = SIZE_ANSWER_ERRO_DAVI;
            ensaio_mensagem_envio_PC();
            break;
    }
}

/**
 *  captura o número de pulsos pela interrupção e carrega no
 * registrador de resposta ao PC
 *
 *    Verificar se o valor de n_pulsos incrementa a cada 1 segundo.
 * se n for menor que anterior, deve indicar erro e sair da verificação.
 *    Para calculo estimado de velocidade máxima 10 km/h, ler valor de W do
 * intrumento e calcular o intervalo mínimo de tempo cada pulso.
 *    Se não houver incremento de pulsos em 1 segundo, interromper o incremento
 * de pulsos. o PC vai acusar erro de W, que terá valor diferente do armazenado
 * no instrumento.
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void ensaio_levanta_w_pulso_STF(enum STATE_ID_W cur_state,
        enum STATE_ID_W new_state, enum EVENT_ID_MAIN new_event){
   
    /*Inicia a verificação de velocidade máxima com
     * Vmax = 10 km/h = 2*K_ensaio / 360.
     * item 7.3.1 alínea c
     */
    uint16_t PulsoSegMax =
            param_ensaio.LEV_W.K_ENSAIO / 180;

    /*atualiza contador*/
    param_ensaio.LEV_W.N_PULSO = odometro_in;
    flag_int.tempo_fracao = false;

    /*verifica condições de incremento a cada segundo*/
    if (flag_int.flag_1_segundo) {
        /* deve ser incremental, sem retorno de valor a cada segundo
         * não deve ultrapassar velocidade de 10 km/h
         */
        if (Delta_pulso > param_ensaio.LEV_W.N_PULSO ||
                (param_ensaio.LEV_W.N_PULSO - Delta_pulso) > PulsoSegMax) {
            /*ENVIAR resposta de erro do instrumento*/
            event[MachineWNr] = EVENT_ERRO_W;
            /*payload de resposta ao erro*/
            erro_armazenado = ANSWER_ERRO_INST_VMAX_W; /*velocidade > 10 km/h */
        }
        Delta_pulso = param_ensaio.LEV_W.N_PULSO;
        flag_int.flag_1_segundo = false;
    }
}

/**
 * Rotina para coletar o valor de W do instrumento.
 *
 *    A cada intervalo de 1 verifica se o veículo está parado, através da
 * contagem de pulsos.
 *    Quando a velocidade é menor que 0.5 km/h é verificado se houve mudança do
 * estado do mototaxímetro (MODO DE VERIFICAÇÃO para o MODO LIVRE).
 *    Em caso de mudança para o MODO LIVRE o ensaio é encerrado.
 *    Para Wmin = 12.000 pulsos/km:
 *       V = 10 km/h -> DeltaT = 30 ms/pulso
 *       v = 3 km/h -> DeltaT = 100 ms/pulso
 *       v = 0.5 km/h -> DeltaT = 600 ms/pulso
 *   Determina-se que se em 1 segundo não ocorreu um pulso de entrada, estima-se
 * que o veículo está parado e começa a arguição de mudança de MODO do taxímetro
 *
 *    Se receber a resposta do intrumento com a indicação de mudança de estado
 * para MODO LIVRE, termina o ensaio e informa o valor de W calculado.
 *
 *    Se o comando COMMAND_ABORTAR_DAVI for recebido, termina o ensaio.
 *    Para outras mensagens do PC, responde com OCUPADO.
 * 
 * @param state
 */
void ensaio_levanta_w_SMF(enum STATE_ID_W state) {

    /*sai, há evento pendente na fila*/
    if (event[MachineWNr])
        return;
    /*verifica se houve novo pulso válido na entrada*/
    if (flag_int.tempo_fracao) {
        event[MachineWNr] = EVENT_PULSE_IN_W;
    } else {
        /*verifica se houve alguma mensagem nas portas de comunicação*/
        gera_eventos_mensagens(MachineWNr);

        if (contador_PC_timeout == 0) {
            /*se não receber comando PC, sair do ensaio*/
            event[MachineWNr] = EVENT_ERRO_W;
            /*payload de resposta ao erro*/
            erro_armazenado = ANSWER_ERRO_PC_TIMEOUT_W;
            contador_PC_timeout = TIMEOUT_PC_VALUE;
        }
    }
}

