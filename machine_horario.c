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
#include "interrupts.h"
#include "uart2.h"
#include "machine_horario.h"

/******************************************************************************/
/* Constants initializing                                                     */
/******************************************************************************/
volatile char HDeltaState = HDELTA_INICIO; /*fase atual do ensaio*/
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
void release_STF_hora(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event){
    current_exclusion[MachineHorarioNr] = NOT_OWNED;
}

/**
 * inicia a execução de tarefas desta máquina
 * 
 * @param state
 */
void mutex_SMF_hora(enum STATE_ID_HORA state){
    if (current_exclusion[MachineHorarioNr] == OWNED) {
        event[MachineHorarioNr] = EVENT_LOCK_HORA;
    }
}

/**
 * Monitora se:
 *  alguma mensagem válida foi recebida e gera o evento correspondente
 *
 * @param state
 */
void idle_SMF_hora(enum STATE_ID_HORA state) {

        gera_eventos_mensagens(MachineHorarioNr);
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
void idle_STF_hora(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event){

}

/**
 * rotina de passagem de comando, não efetua monitoração
 * 
 * @param state
 */
void none_SMF_hora(enum STATE_ID_HORA state) {

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
void erro_STF_hora(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event) {

    erro_handler(MachineHorarioNr, &new_event, &erro_armazenado);

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
}
    
    

/**
 * inicia tarefas na entrada do modo horário
 * ou retorno de funções
 *
 * Na entrada do ENSAIO MODO HORARIO:
 * se o numero de frações recebido no comando for menor que 1, responda com erro
 *  ev.erro_main
 * se o numero for positivo, armazene na variável horario_main.N_FRACOES;
 * reiniciar contadores:
 *  tempo total de frações horario_main.TEMPO_32 (us)
 *  reiniciar contadores de fração horario_main.CONT_FRACOES
 *  reiniciar contador de frações/pulsos
 *  reiniciar contador de tempo na rotina de interrupção
 *  reiniciar contador de timeout
 * retorna mensagem para o PC, indica que o ensaio começou. Leitura atual = 0.
 * reiniciar contador de timeout
 *
 * Na reentrada de outros estados, nada faz. Apenas passagem.
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void ensaio_horario_STF(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event) {

    char i;

    /*Ao primeiro comando, reinicia, recebe e armazena os parametros recebidos*/
    for (i = 0; i < SIZE_PC_HORARIO; i++)
        param_ensaio.HORARIO_A4[i] =
            mensagem_Davi.payload.parameter[i];

    param_ensaio.HORARIO.N_FRACOES =
            endian_shortint(param_ensaio.HORARIO.N_FRACOES);

    /*converter valor de entrada, se estiver em BCD*/
    if (mensagem_Davi.overhead.format == FORMAT_BCD_DAVI) {
        param_ensaio.HORARIO.N_FRACOES =
                Converter_BCD_to_HEX((uint32_t)param_ensaio.HORARIO.N_FRACOES);
    }
    /*se numero N de frações fora da faixa, retorna erro de valor*/
    if (param_ensaio.HORARIO.N_FRACOES < N_FRACOES_MIN ||
            param_ensaio.HORARIO.N_FRACOES > N_FRACOES_MAX) {
        /*carrega o código de erro para a resposta PC e sai do ensaio*/
        event[MachineHorarioNr] = EVENT_ERRO_HORA;
        le_mensagem_davi(apaga);
    } else {
        /*Envia resposta ao comando, com valor de tempo = 0*/
        mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
        mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
        mensagem_Davi.overhead.length = SIZE_ANSWER_PC_HORARIO;
        for (i = 0; i <= SIZE_ANSWER_PC_HORARIO; i++) {
            mensagem_Davi.payload.parameter[i] = 0;
        }
        ensaio_mensagem_envio_PC();
        /*passa para próximo estado*/
        event[MachineHorarioNr] = EVENT_PULSE_IN_HORA;
        flag_int.flag_1_segundo = false;
    }
}

/**
 * função para gerenciar as mensagens de atualização de parâmetros do INSTRUM
 * e de respostas a eventos
 *
 * Ao receber erros do INSTRUMENTO ou TIMEOUT de mensagens:
 *   carregar o valor de erro correspondente
 *   encerrar o ensaio
 *   aguarda contato do PC para responder com o erro armazenado
 *
 * Ao receber uma resposta de leitura do INSTRUMENTO:
 *   armazenar a resposta no registrador adequado
 *   Transmitir a proxima mensagem da LISTA ao INSTRUMENTO, se necessário
 *
 * Ao
 * a cada contagem de pulso o evento EVENT_TRANSM_MENSAGEM_INSTRUMENTO_MAIN
 * é acionado para iniciar a transmissão de mensagens de leitura
 *
 * a cada leitura, devido ao evento EVENT_RECEBE_MENSAGEM_INSTRUMENTO_MAIN
 * o parâmetro é copiado para o ensaio
 *
 * até que seja atingido o numero de parâmetros, nova mensagem é encaminhada.
 * para sinalizar que é necessário encaminhar nova mensagem da série, gerar o
 * evento EVENT_TRANSM_MENSAGEM_INSTRUMENTO_MAIN novamente.
 *
 *  * trata o recebimento de um novo pulso de entrada no modo de ensaio horario
 *
 * reinicia indicador de mensagem a cada novo pulso
 * verifica se o numero de medidas N_FRACOES foi completado
 * e SOMA valor coletado pela interrupção de tempo
 * Item 8.12 alínea j: tempo de até 30 minutos
 *
 * verificar se houve mudança de posição (bandeira), abortar por ERRO
 * verificar se houve mudança para A PAGAR, abortar por erro
 * verificar se houve mudança para LIVRE, abortar por erro
 * verificar se permanece no modo horário durante ensaio
 * encaminhar comando NOP antes de terminar o timeout, keep-alive
 *
 * Solicita nova verificação de estado do INSTRUMENTO através de mensagens:
 *   EVENT_TRANSM_MENSAGEM_INSTRUMENTO_MAIN
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void ensaio_horario_delta_STF(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event) {

    /*valor temporário*/
    uint32_t scratch32bits; 

    /*entrada a partir do modo horário*/
    if (cur_state == STATE_ENSAIO_HORARIO) {
        /*inicia solicitação de estado do instrumento*/
        MsgNr = 0;
        flags_ensaio.NovaMensagem = true;
        flags_ensaio.Parametro = false;
        flag_int.tempo_fracao = false;
        flag_int.flag_1_segundo = false;
        ConfigPulseIn(false);
        HDeltaState = HDELTA_INICIO;
    }        /* a cada nova mensagem recebida do instrumento:
         * se foi solicitado, verifica sequência e conteúdo
         * senão, repassar para o PC (resposta a comando externo)
         */
    else if (new_event == EVENT_RECEBE_MENSAGEM_INSTRUMENTO_HORA) {
        le_mensagem_393(apaga);
        /*volta a permitir PC enviar comandos pro INSTRUMENTO*/
        if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE) {
            ensaio_passagem_INSTR_PC(MachineHorarioNr);
            return;
        }
        /*verifica se as respostas acontecem em sequência*/
        if (mensagem_393.overhead.command ==
                LISTA_COMMAND_ENSAIO_HORARIO_393[MsgNr]) {
            /*lê parâmetro de resposta do INSTRUMENTO*/
            le_mensagem(&flags_ensaio, &PosC, &Di);
            MsgNr++;
            if (MsgNr >= sizeof (LISTA_COMMAND_ENSAIO_HORARIO_393)) {
                /*ao receber todos os parâmetros, informe.*/
                MsgNr = 0;
                flags_ensaio.Parametro = true;
                flags_ensaio.MensagemModo = MODO_TRANSPARENTE;
            } else {
                /*solicita novo parâmetro*/
                flags_ensaio.NovaMensagem = true;
            }
        } else {
            /*comando fora da sequência, denota falha ou burla*/
            erro_armazenado = ANSWER_ERRO_INST_SEQUENCIA_HORA;
            flags_ensaio.Erro = true;
            HDeltaState = HDELTA_SAIDA_OU_ERRO;
        }
    }

    switch (HDeltaState) {
        case HDELTA_INICIO:
            /* entrada. aguarda a primeira leitura do instrumento:
             * conferir se permanece em modo horário, v_ini=0;
             * se não estiver em modo de verificação, a pagar ou livre, armazenar
             * a posição atual;
             * reiniciar contador de tempo;
             */
            /*remove pedidos de interrupção*/
            flag_int.tempo_fracao = false;
            flag_int.flag_1_segundo = false;
            if (flags_ensaio.Parametro == true) {
                /* verifica se instrumento está no modo horário
                 * se estiver em modo quilometrico nesta fase
                 * indica falha ou fraude
                 */
                if (PosC == PosC_A_PAGAR ||
                        PosC == PosC_LIVRE ||
                        PosC == PosC_MODO_VERIFICACAO ||
                        flags_ensaio.ModoOpr == MO_QUILOMETRICO) {
                    /*Posição de comando incorreto, denota falha ou burla*/
                    erro_armazenado = ANSWER_ERRO_INST_POSC_HORA;
                    if (flags_ensaio.ModoOpr == MO_QUILOMETRICO)
                        /*modo incorreto, denota falha ou burla*/
                        erro_armazenado = ANSWER_ERRO_INST_MODO_HORA;
                    flags_ensaio.Erro = true;
                    HDeltaState = HDELTA_SAIDA_OU_ERRO;
                } else {
                    /*armazena o valor inicial da tarifa na primeira fase*/
                    PosC_inicial = PosC;
                    /*reinicia número de frações coletadas*/
                    param_ensaio.HORARIO.CONT_FRACOES = 0;
                    param_ensaio.HORARIO.IND_FINAL = 0;
                    param_ensaio.HORARIO.TEMPO_32 = 0;
                    param_ensaio.HORARIO.IND_INICIAL = Di;
                    /*habilita a contagem de tempo e
                     redefine o valor inicial do contador de tempo*/
                    di();
                    odometro_in = 0;
                    PIE2bits.TMR3IE = false;
                    WriteTimer3(0);
                    PIR2bits.TMR3IF = false;
                    PIE2bits.TMR3IE = true;
                    ConfigPulseIn(true);
                    ei();
                    flag_int.tempo_fracao = false;
                    flags_ensaio.PulsoPend = false;
                    /*aguarda fração para iniciar medição*/
                    HDeltaState = HDELTA_FRA_INI;
                }
                flags_ensaio.Parametro = false;
            }
            break;
        case HDELTA_FRA_INI:
            /* fase de espera pela primeira fração:
             * verificar se houve mudança de fração:
             *      solicitar leitura do instrumento;
             * após a leitura do instrumento (f_parametro):
             * conferir se permanece na posição de comando inicial;
             *      se posição mudou: sai com erro;
             *      se passou para modo quilometrico: sai com erro;
             */
            if (flag_int.tempo_fracao) {
                /*no evento fração, armazenar pedido de leitura DI*/
                flags_ensaio.PulsoPend = true;
                /*libera pedido e resposta anterior*/
                flags_ensaio.Parametro = false;
                flags_ensaio.ParametroFracao = false;
                flag_int.tempo_fracao = false;
            }
            if (flags_ensaio.PulsoPend &&
                    flags_ensaio.MensagemModo == MODO_TRANSPARENTE) {
                /*pedido de leitura após mudança de fração*/
                flags_ensaio.ParametroFracao = true;
                /*se cancela pedido e resposta anterior*/
                flags_ensaio.Parametro = false; /*aguarda nova leitura*/
                /*solicitar parâmetros*/
                flags_ensaio.NovaMensagem = true;
                flags_ensaio.PulsoPend = false;
            } else
                if (flag_int.flag_1_segundo) {
                flag_int.flag_1_segundo = false;
                if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE)
                    /*solicita leitura do instrumento a cada segundo, se ocioso*/
                    flags_ensaio.NovaMensagem = true;
            }
            /*recebeu leitura dos parametros do instrumento*/
            if (flags_ensaio.Parametro == true) {
                /*verifica se permanece em modo horario
                 * e na mesma posição de comando
                 */
                if (flags_ensaio.ModoOpr == MO_HORARIO &&
                        PosC == PosC_inicial) {
                    if (flags_ensaio.ParametroFracao == true) {
                        if (Di != param_ensaio.HORARIO.IND_INICIAL) {
                            /*armazena fração inicial*/
                            param_ensaio.HORARIO.IND_INICIAL = Di;
                            HDeltaState = HDELTA_HORARIO;
                        }
                        flags_ensaio.ParametroFracao = false;
                    }
                } else {
                    /*Posiçao de comando alterada, denota falha ou burla*/
                    erro_armazenado = ANSWER_ERRO_INST_POSC_HORA;
                    if (flags_ensaio.ModoOpr == MO_QUILOMETRICO)
                        /*modo incorreto, denota falha ou burla*/
                        erro_armazenado = ANSWER_ERRO_INST_MODO_HORA;
                    flags_ensaio.Erro = true;
                    HDeltaState = HDELTA_SAIDA_OU_ERRO;
                }
                flags_ensaio.Parametro = false;
            }
            break;
        case HDELTA_HORARIO:
            if (flag_int.tempo_fracao) {
                /*no evento fração, armazenar pedido de leitura DI*/
                flags_ensaio.PulsoPend = true;
                /*libera pedido e resposta anterior*/
                flags_ensaio.Parametro = false;
                flags_ensaio.ParametroFracao = false;
                flag_int.tempo_fracao = false;
            }
            if (flags_ensaio.PulsoPend &&
                    flags_ensaio.MensagemModo == MODO_TRANSPARENTE) {
                /*pedido de leitura após mudança de fração*/
                flags_ensaio.ParametroFracao = true;
                /*se cancela pedido e resposta anterior*/
                flags_ensaio.Parametro = false; /*aguarda nova leitura*/
                /*solicitar parâmetros*/
                flags_ensaio.NovaMensagem = true;
                flags_ensaio.PulsoPend = false;
            } else
                if (flag_int.flag_1_segundo) {
                flag_int.flag_1_segundo = false;
                if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE)
                    /*solicita leitura do instrumento a cada segundo, se ocioso*/
                    flags_ensaio.NovaMensagem = true;
            }
            /*recebeu leitura dos parametros do instrumento*/
            if (flags_ensaio.Parametro == true) {
                /*verifica se está no modo horário*/
                /*verifica se está na mesma posição de comando*/
                if (flags_ensaio.ModoOpr == MO_HORARIO &&
                        PosC == PosC_inicial) {
                    /*somente contar número de intervalos determinados pelo ensaio*/
                    if (param_ensaio.HORARIO.CONT_FRACOES <
                            param_ensaio.HORARIO.N_FRACOES) {
                        if (flags_ensaio.ParametroFracao == true) {
                            /*verifica se houve incremento de indicação*/
                            if (Di != param_ensaio.HORARIO.IND_FINAL) {
                                /*adiciona intervalo de tempo entre as frações*/
                                scratch32bits = tempo_fracao;
                                param_ensaio.HORARIO.TEMPO_32 += divl10(scratch32bits); //0.1us
                                /*incrementa o numero de frações recebidas*/
                                param_ensaio.HORARIO.CONT_FRACOES++;
                                param_ensaio.HORARIO.IND_FINAL = Di;
                            } else {
                                /*falha no incremento de frações*/
                                erro_armazenado = ANSWER_ERRO_INST_FRACAO_HORA;
                                flags_ensaio.Erro = true;
                                HDeltaState = HDELTA_SAIDA_OU_ERRO;
                            }
                            flags_ensaio.ParametroFracao = false;
                        }
                    } else {
                        flags_ensaio.Erro = false;
                        HDeltaState = HDELTA_SAIDA_OU_ERRO;
                    }
                } else {
                    /*Posiçao de comando alterada, denota falha ou burla*/
                    erro_armazenado = ANSWER_ERRO_INST_POSC_HORA;
                    if (flags_ensaio.ModoOpr == MO_QUILOMETRICO)
                        /*modo incorreto, denota falha ou burla*/
                        erro_armazenado = ANSWER_ERRO_INST_MODO_HORA;
                    flags_ensaio.Erro = true;
                    HDeltaState = HDELTA_SAIDA_OU_ERRO;
                }
                flags_ensaio.Parametro = false;
            }
            break;
        case HDELTA_SAIDA_OU_ERRO:
            /*saída do ensaio*/
            /*ENCERRA a coleta de pulsos */
            ConfigPulseIn(false);
            /*descarta eventos de Pulso de fração*/
            flag_int.tempo_fracao = false;
            if (flags_ensaio.Erro) {
                /*indica que falhou*/
                event[MachineHorarioNr] = EVENT_RECEBE_ERRO_INSTRUMENTO_HORA;
            }
            break;
        default:
            break;
    }
    /*se necessário, envia pedido de parâmetro ao INSTRUMENTO*/
    if (flags_ensaio.NovaMensagem == true) {
        /*usar ponteiro para montar a mensagem de envio ao instrumento*/
        mensagem_393.overhead.head = STX_MASTER_393;
        mensagem_393.overhead.command = LISTA_COMMAND_ENSAIO_HORARIO_393[MsgNr];
        mensagem_393.overhead.format = FORMAT_HEX_393;
        mensagem_393.overhead.length = SIZE_COMMAND_PosC_393;
        ensaio_mensagem_envio_INSTR();
        /*bloqueia mensagens para não atrapalhar*/
        flags_ensaio.MensagemModo = MODO_ENSAIO;
        flags_ensaio.NovaMensagem = false;
    }
}

/**
 *  verifica mensagens do PC
 * se receber comando:
 *  EVENT_RECEBE_COMANDO_PC_MAIN, analisa:
 *      se abortar, EVENT_ENSAIO_ABORTAR_MAIN
 *      se ensaio tempo, EVENT_RECEBE_COMANDO_PC_MAIN
 *          responder com valor tempo e fracao atual
 *      se outro comando, EVENT_NOP
 *          responder com ocupado
 * se estado anterior for:
 *  STATE_ENSAIO_HORARIO_MAIN
 *      gera evento EVENT_NOP
 *  STATE_TESTA_DELTA_MAIN
 *      gera evento EVENT_RECEBE_COMANDO_PC_MAIN
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void ensaio_horario_comando_STF(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event) {

    char i;

    /*condição de retorno forçada*/
    event[MachineHorarioNr] = EVENT_ENSAIO_HORARIO;

    mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
    /*command já preenchido*/
    mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;

    switch (mensagem_Davi.overhead.command) {
        case COMMAND_ABORTAR_DAVI:
            /*ENCERRA a coleta de pulsos */
            ConfigPulseIn(false);
            event[MachineHorarioNr] = EVENT_ENSAIO_ABORTAR_HORA;
            break;
        case COMMAND_HORARIO_DAVI:
            /*retorna para teste Delta, se veio de lá*/
            if (cur_state == STATE_E_H_DELTA) {
                event[MachineHorarioNr] = EVENT_ENSAIO_NOP_HORA;
            }

            /*responde com os parametros disponíveis*/
            mensagem_Davi.overhead.length = SIZE_ANSWER_PC_HORARIO;
            for (i = 0; i < SIZE_ANSWER_PC_HORARIO; i++)
                mensagem_Davi.payload.parameter[i] =
                    param_ensaio.HORARIO_A6[i];
            /*corrige endianness dos parametros de 32 bits*/
            for (i = 0; i <= 2; i++) {
                mensagem_Davi.valor_long.size_32[i] =
                endian_big_to_little(mensagem_Davi.valor_long.size_32[i]);
            }
            /*corrige endianness dos parametros de 16 bits*/
            mensagem_Davi.valor_int.size_int[6] =
                    endian_shortint(mensagem_Davi.valor_int.size_int[6]);

            ensaio_mensagem_envio_PC();
            break;
        default:
            /*outros comandos, recebe erro ANSWER_OCUPADO_DAVI*/
            if (cur_state == STATE_E_H_DELTA) {
                event[MachineHorarioNr] = EVENT_ENSAIO_NOP_HORA;
            }
            mensagem_Davi.valor_int.size_int[0] =
                    endian_shortint(ANSWER_OCUPADO_DAVI);

            mensagem_Davi.overhead.head = STX_ERROR_DAVI;
            mensagem_Davi.overhead.command = COMMAND_NOP_DAVI;
            mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
            mensagem_Davi.overhead.length = SIZE_ANSWER_ERRO_DAVI;
            ensaio_mensagem_envio_PC();
            break;
    }
    le_mensagem_davi(apaga);
}

/**
 * monitora atividade e CONVERTE em eventos durante modo horário.
 *
 * verifica se há pulso na entrada
 *  gera EVENT_PULSE_IN_MAIN
 * verifica mensagens do instrumento
 *   EVENT_RECEBE_ERRO_INSTRUMENTO_MAIN
 *   EVENT_RECEBE_MENSAGEM_INSTRUMENTO_MAIN
 * se timeout, armazenar para responder para o PC
 *   EVENT_TIMEOUT_MAIN
 *
 * verifica mensagens do PC
 * se receber comando:
 *  EVENT_RECEBE_COMANDO_PC_MAIN
 *      na função ensaio_horario_mensagem_STF_main, analisa:
 *      abortar, EVENT_ENSAIO_ABORTAR_MAIN
 *      ensaio tempo, EVENT_RECEBE_COMANDO_PC_MAIN
 *      outro comando, EVENT_RECEBE_COMANDO_PC_MAIN
 *      responder com valor tempo e fracao atual
 *      se receber outros comandos responder com ocupado
 *
 * @param state
 */
void ensaio_horario_SMF(enum STATE_ID_HORA state) {

    if (flag_int.tempo_fracao) {
        event[MachineHorarioNr] = EVENT_PULSE_IN_HORA;
        //flag.tempo_fracao = false;
    }  else
        if (contador_timeout == 0) {
        /*este evento tem menor prioridade. Se nova mensagem, descarta evento*/
        if (flags_ensaio.MensagemModo == MODO_ENSAIO) {
            event[MachineHorarioNr] = EVENT_TIMEOUT_HORA;
        }
        contador_timeout = TIMEOUT_VALUE;
    } else
        if (flag_int.flag_1_segundo) {
        event[MachineHorarioNr] = EVENT_1_SEC_HORA;
    }
    /*verifica se houve alguma mensagem nas portas de comunicação*/
    gera_eventos_mensagens(MachineHorarioNr);
   
    if (contador_PC_timeout == 0) {
            /*se não receber comando PC, sair do ensaio*/
            event[MachineHorarioNr] = EVENT_ERRO_HORA;
            /*payload de resposta ao erro*/
            erro_armazenado = ANSWER_ERRO_PC_TIMEOUT_HORA;
            contador_PC_timeout = TIMEOUT_PC_VALUE;
        }

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
void passagem_STF_hora(enum STATE_ID_HORA cur_state,
        enum STATE_ID_HORA new_state, enum EVENT_ID_HORA new_event) {

    ensaio_passagem_INSTR_PC(MachineHorarioNr);
}
