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
#include "machine_prot_393.h"
#include "machine_main.h"
#include "machine_quilometrico.h"
#include "machine_horario.h"

/******************************************************************************/
/* Constants initializing                                                     */
/******************************************************************************/
static char QDeltaState = QDELTA_INICIO; /*fase atual do ensaio*/
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
void release_STF_quilo(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event){
    current_exclusion[MachineQuilometricoNr] = NOT_OWNED;
}

/**
 * inicia a execução de tarefas desta máquina
 * 
 * @param state
 */
void mutex_SMF_quilo(enum STATE_ID_QUILO state){
    if (current_exclusion[MachineQuilometricoNr] == OWNED) {
        event[MachineQuilometricoNr] = EVENT_LOCK_QUILO;
    }
}

/**
 * Monitora se:
 *  alguma mensagem válida foi recebida e qual seu valor
 * (mensagem/comando) e gera o evento correspondente
 *
 * @param state
 */
void idle_SMF_quilo(enum STATE_ID_QUILO state) {
        gera_eventos_mensagens(MachineQuilometricoNr);
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
void idle_STF_quilo(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event){

}

/**
 * rotina de passagem de comando, não efetua monitoração
 * 
 * @param state
 */
void none_SMF_quilo(enum STATE_ID_QUILO state) {

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
void passagem_STF_quilo(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event) {

    ensaio_passagem_INSTR_PC(MachineQuilometricoNr);
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
void erro_STF_quilo(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event) {

    erro_handler(MachineQuilometricoNr, &new_event, &erro_armazenado);
}

/**
 * INÍCIO do ensaio quilometrico
 *
 *
 * armazena os valores dos parametros recebidos pelo comando DIST
 * verifica se os valores estão dentro do permitido:
 *  20000.0 m >= distancia > 12.5 m
 *  150 km/h >= velocidade >= 1 km/h
 *  65535 pulsos/km >= K > 12000 pulsos/km
 *  150 km/h >= Vt >= 1 km/h
 * valores corretos: responde com o valor inicial zero dos parametros
 * valores fora da faixa: responde com erro e encerra ensaio
 *
 * reinicia contadores(distancia percorrida, frações, etc)
 *
 * habilita a entrada de eventos:
 * pulso recebido por mudança velocidade de transição
 * pulso recebido por mudança no dispositivo indicador
 * pulso recebido por outro evento, que deve gerar erro no ensaio.
 * timeout para mensagens de comunicação
 *
 * responder:
 *  comando Dist:
 *    valor inicial dos resultados
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 * @author eduardo lopes
 */
void ensaio_quilom_STF(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event) {

    extern volatile char atraso1seg;
    char i;

    /*recebe e armazena os valores dos parametros recebidos pelo comando DIST*/
    for (i = 0; i < SIZE_PC_QUILO; i++)
        param_ensaio.QUILO_A4[i] =
            mensagem_Davi.payload.parameter[i];

    param_ensaio.QUILO.D_ENSAIO =
            endian_big_to_little(param_ensaio.QUILO.D_ENSAIO);
    param_ensaio.QUILO.K_ENSAIO =
            endian_shortint(param_ensaio.QUILO.K_ENSAIO);
    param_ensaio.QUILO.V_ATUAL = 0;
    /*V_ENSAIO não precisa de correção*/

    /*converter valor de entrada, se estiver em BCD*/
    if (mensagem_Davi.overhead.format == FORMAT_BCD_DAVI) {
        param_ensaio.QUILO.D_ENSAIO =
                Converter_BCD_to_HEX(param_ensaio.QUILO.D_ENSAIO);
        param_ensaio.QUILO.K_ENSAIO =
                Converter_BCD_to_HEX((uint32_t)param_ensaio.QUILO.K_ENSAIO);
    }

    /* verifica se os valores estão dentro do permitido */
    if (param_ensaio.QUILO.D_ENSAIO > D_ENSAIO_MAX ||
            param_ensaio.QUILO.D_ENSAIO < D_ENSAIO_MIN ||
            param_ensaio.QUILO.V_ENSAIO > V_ENSAIO_MAX ||
            param_ensaio.QUILO.V_ENSAIO < V_ENSAIO_MIN ||
            param_ensaio.QUILO.K_ENSAIO < K_MIN ||
            param_ensaio.QUILO.V_ENSAIO < param_ensaio.QUILO.VT_TEORICO) {
        /*carrega o código de erro para a resposta ao PC e encerra ensaio*/
        erro_armazenado = ANSWER_VALOR_DAVI;
        event[MachineQuilometricoNr] = EVENT_ERRO_MAIN;
        le_mensagem_davi(apaga);
    }
    else {
        /* reinicia contadores(distancia percorrida,frações, etc)*/
        for (i = 0; i < SIZE_ANSWER_PC_QUILO; i++) {
            param_ensaio.QUILO_A6[i] = 0;
            mensagem_Davi.payload.parameter[i] = 0;
        }
        /*Envia resposta ao comando, com valores = 0*/
        mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
        mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
        mensagem_Davi.overhead.length = SIZE_ANSWER_PC_QUILO;
        ensaio_mensagem_envio_PC();

        /*prepara para acelerar os motores em até 1 segundo*/
        atraso1seg = 0;
        flag_int.flag_1_segundo = 0;
    }
}
/**
 * Ensaio quilométrico
 *
 *  Após a configuração inicial, aguarda a indicação de 1 segundo.
 *
 *  Muda para MODO_ENSAIO, onde as respostas do INSTRUMENTO ficam bloqueadas,
 * assim como os comandos do PC.
 * 
 *  Solicita ao INSTRUMENTO a posição de comando atual e armazena.
 * 
 *  Gera para o INSTRUMENTO sinal que simula uma rampa de subida de velocidade.
 * Esta rampa tem a aceleração de 7 m/s^2 até (VT -3)
 * Segue com aceleração de 1 m/s^2 até (VT + 3) ou se identificar VT.
 * Após esta fase volta a acelerar com 7 m/s^2 até Vatual = V_ensaio
 *
 *  A cada 1_SEGUNDO verifica se o INSTRUMENTO está no modo Quilometrico, para
 * registrar VT. Através de comando/resposta.
 *   - se sair de modo quilométrico após registrar VT, sinaliza ERRO INSTRUMENTO
 *   -
 *
 *  Se houver uma sinalização do INSTRUMENTO por PULSE_IN é verificado se houve
 * incremento de fração ou troca de posição de comando (OCUPADO ou A PAGAR).
 *
 *  Se o PULSE_IN ocorrer depois de V_ENSAIO, reiniciar contagem de distância.
 *
 *  Encaminhar pergunta de MODO DE OPERAÇÃO para o INSTRUMENTO:
 *   -se confirmar modo != modo quilométrico e != POSIÇÃO DE COMANDO armazenada
 *    sair com ERRO INSTRUMENTO.
 *
 * desacelera com 7 m/s^2 até Velocidade = 0
 *
 * @param cur_state
 * @param new_state
 * @param new_event
 */
void ensaio_quilom_delta_STF(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event) {

    static uint32_t odometro_total = 0;
    static char contador_s = 0; /*contador para espera em aceleração*/

    /* n pulsos que correspondem a 1000 m em K pré-fornecido: param_ensaio.QUILO.K_ENSAIO*/

    /* entrada a partir do modo quilometrico
     * no evento de 1 segundo
     * primeira solicitação de estado atual do instrumento*/
    if (cur_state == STATE_ENSAIO_QUILOMETRICO) {
        MsgNr = 0;
        flags_ensaio.NovaMensagem = true;
        ConfigPulseIn(false);
        QDeltaState = QDELTA_INICIO;
    } else
        if (new_event == EVENT_RECEBE_MENSAGEM_INSTRUMENTO_QUILO) {
        /* a cada nova mensagem recebida do instrumento:
         * se foi solicitado, verifica sequência e conteúdo
         * senão, repassar para o PC (resposta a comando externo)
         */
        le_mensagem_393(apaga);
        /*volta a permitir PC enviar comandos pro INSTRUMENTO*/
        if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE) {
            ensaio_passagem_INSTR_PC(MachineQuilometricoNr);
            return;
        }
        /*verifica se as respostas acontecem em sequência*/
        if (mensagem_393.overhead.command ==
                LISTA_COMMAND_ENSAIO_HORARIO_393[MsgNr] ||
                MsgNr == 0) {
            /*lê parâmetro de resposta do INSTRUMENTO*/
            le_mensagem(&flags_ensaio, &PosC, &Di);
            MsgNr++;
            if (MsgNr >= sizeof (LISTA_COMMAND_ENSAIO_HORARIO_393)) {
                /*informa que há novo bloco de mensagens e reinicia ponteiro*/
                MsgNr = 0;
                flags_ensaio.Parametro = true;
                flags_ensaio.MensagemModo = MODO_TRANSPARENTE;
            } else {
                /*solicita novo parâmetro*/
                flags_ensaio.NovaMensagem = true;
            }
        } else {
            /*comando fora da sequência, denota falha ou burla*/
            erro_armazenado = ANSWER_ERRO_INST_SEQUENCIA_QUILO;
            flags_ensaio.Erro = true;
            QDeltaState = QDELTA_SAIDA_OU_ERRO;
        }
    }

    switch (QDeltaState) {
        case QDELTA_INICIO:
            /* entrada ao receber a primeira leitura do instrumento:
             * conferir se permanece em modo horário, v_ini=0;
             * se não estiver em modo de verificação, a pagar ou livre, armazenar
             * a posição atual;
             * reiniciar contador de distancia;
             * reiniciar indicador de distancia final;
             * ajustar distância máxima em 1000 m;
             */
            /*remove pedidos de interrupção*/
            flag_int.flag_1_segundo = false;
            flag_int.tempo_fracao = false;
            flag_int.distancia = false;
            if (flags_ensaio.Parametro == true) {
                /* verifica se instrumento está no modo horário
                 * se estiver em modo quilometrico nesta fase
                 * indica falha ou fraude
                 */
                if (PosC == PosC_A_PAGAR ||
                        PosC == PosC_LIVRE ||
                        PosC == PosC_MODO_VERIFICACAO ||
                        flags_ensaio.ModoOpr == MO_QUILOMETRICO) {
                    /*Posição de comando incorreta, denota falha ou burla*/
                    erro_armazenado = ANSWER_ERRO_INST_POSC_QUILO;
                    if (flags_ensaio.ModoOpr == MO_QUILOMETRICO)
                        /*modo incorreto, denota falha ou burla*/
                        erro_armazenado = ANSWER_ERRO_INST_MODO_QUILO;
                    flags_ensaio.Erro = true;
                    QDeltaState = QDELTA_SAIDA_OU_ERRO;
                } else {
                    /*armazena o valor inicial da tarifa na primeira leitura*/
                    PosC_inicial = PosC;
                    /*somente permitir que a aceleração e busca de Vt ocorra em até
                     1000m, para controlar falha do instrumento ou tentativa de burla*/
                    odometro_out_max = param_ensaio.QUILO.K_ENSAIO; /*1000 m*/
                    odometro_out = 0;
                    flag_int.distancia = false;
                    flags_ensaio.DistPend = false;
                    flags_ensaio.PulsoPend = false;
                    QDeltaState = QDELTA_VT;
                }
                flags_ensaio.Parametro = false;
            }
            break;
        case QDELTA_VT:
            /* fase de aceleração para identificar Vt real:
             * a cada segundo, solicitar leitura do instrumento;
             * após a leitura do instrumento (f_parametro):
             * conferir se permanece na posição de comando inicial;
             *      se posição mudou: sai com erro;
             *          ou
             *      se passou para modo quilometrico: armazena Vatual;
             *      se em modo horário: acelerar;
             *      se atingiu Vensaio, ainda em modo horário: sai com erro;
             *      se atingiu distância máxima: sai com erro;
             */
            /*descarta eventos não relevantes*/
            flag_int.tempo_fracao = false;
            if (flag_int.flag_1_segundo) {
                flag_int.flag_1_segundo = false;
                if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE)
                    /*solicita leitura do instrumento a cada segundo, se ocioso*/
                    flags_ensaio.NovaMensagem = true;
            }
            /*recebeu leitura dos parametros do instrumento*/
            if (flags_ensaio.Parametro == true) {
                /* verifica se instrumento está na mesma bandeira*/
                if (PosC == PosC_inicial) {
                    /*verifica se atingiu VT*/
                    if (flags_ensaio.ModoOpr == MO_QUILOMETRICO) {
                        /*Vt  identificado com sucesso*/
                        param_ensaio.QUILO.VT_REAL =
                                param_ensaio.QUILO.V_ATUAL;
                        /*aguardar no máximo 1000 m para nova aceleração*/
                        odometro_out = 0;
                        flag_int.distancia = false;
                        /*habilita a contagem de pulsos de fração*/
                        ConfigPulseIn(true);
                        QDeltaState = QDELTA_VENSAIO;
                    } else {
                        /*se atingir V_ENSAIO ainda em modo horário,
                         * ou se atingir 1000m sem determinar VT, sai com erro*/
                        if (param_ensaio.QUILO.V_ATUAL == param_ensaio.QUILO.V_ENSAIO
                                || flag_int.distancia) {
                            /*não encontrou Vt, denota falha ou burla*/
                            erro_armazenado = ANSWER_ERRO_INST_VT_QUILO;
                            flags_ensaio.Erro = true;
                            QDeltaState = QDELTA_SAIDA_OU_ERRO;
                        } else {
                            /*acelera em busca do valor de VT_REAL*/
                            contador_s++;
                            if (contador_s >= ATRASO_ACELERADOR) {
                                acelerador(&param_ensaio.QUILO.V_ATUAL, &param_ensaio.QUILO.V_ENSAIO);
                                gera_velocidade(&param_ensaio.QUILO.V_ATUAL, &param_ensaio.QUILO.K_ENSAIO);
                                contador_s = 0;
                            }
                        }
                    }
                } else {
                    /*posição incorreto, denota falha ou burla*/
                    erro_armazenado = ANSWER_ERRO_INST_POSC_QUILO;
                    flags_ensaio.Erro = true;
                    QDeltaState = QDELTA_SAIDA_OU_ERRO;
                }
                flags_ensaio.Parametro = false;
            }
            break;
        case QDELTA_VENSAIO:
            /* fase de aceleração até a velocidade de ensaio:
             * a cada segundo, solicitar leitura do instrumento;
             * após a leitura do instrumento (f_parametro):
             * conferir se permanece na posição de comando inicial;
             *      se posição mudou: sai com erro;
             *      se passou para modo horário: sai com erro;
             *      se atingiu distância máxima: sai com erro;
             *      se não atingiu Vensaio:
             *          acelerar;
             */

            /*descarta eventos não relevantes*/
            flag_int.tempo_fracao = false;
            if (flag_int.flag_1_segundo) {
                flag_int.flag_1_segundo = false;
                if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE)
                    /*solicita leitura do instrumento a cada segundo, se ocioso*/
                    flags_ensaio.NovaMensagem = true;
            }
            if (flag_int.distancia) {
                /*não recebeu fração inicial em 1000 m*/
                erro_armazenado = ANSWER_ERRO_INST_DIST_QUILO;
                flags_ensaio.Erro = true;
                flags_ensaio.Parametro = false;
                flag_int.distancia == false;
                QDeltaState = QDELTA_SAIDA_OU_ERRO;
            }
            /*recebeu leitura dos parametros do instrumento*/
            if (flags_ensaio.Parametro == true) {
                if (flags_ensaio.ModoOpr == MO_QUILOMETRICO &&
                        PosC == PosC_inicial) {
                    if (param_ensaio.QUILO.V_ATUAL ==
                            param_ensaio.QUILO.V_ENSAIO) {
                        /*aguardar no máximo 1000 m para primeira fração*/
                        /* odometro_out_max = D_ENSAIO*K_ENSAIO/c = dm*pulso/c, onde c = 10000 dm/km
                         * para evitar overflow, c = 2500 dm/km e D_ENSAIO = D_ENSAIO/4
                         */
                        odometro_out_max = (divl10(param_ensaio.QUILO.D_ENSAIO) *
                                param_ensaio.QUILO.K_ENSAIO) / 1000;
                        di();
                        odometro_out = 0;
                        odometro_total = 0;
                        flag_int.distancia = false;
                        flag_int.tempo_fracao = false;
                        ei();
                        /*aguarda próxima fração ao atingir V_ENSAIO*/
                        QDeltaState = QDELTA_FRA_INI;
                    } else {
                        /*acelera até o valor de V_ENSAIO*/
                        acelerador(&param_ensaio.QUILO.V_ATUAL, &param_ensaio.QUILO.V_ENSAIO);
                        gera_velocidade(&param_ensaio.QUILO.V_ATUAL, &param_ensaio.QUILO.K_ENSAIO);
                    }
                } else {
                    /*Posiçao de comando alterada, denota falha ou burla*/
                    erro_armazenado = ANSWER_ERRO_INST_POSC_QUILO;
                    if (flags_ensaio.ModoOpr == MO_HORARIO)
                        /*modo incorreto, denota falha ou burla*/
                        erro_armazenado = ANSWER_ERRO_INST_MODO_QUILO;
                    flags_ensaio.Erro = true;
                    QDeltaState = QDELTA_SAIDA_OU_ERRO;
                }
                flags_ensaio.Parametro = false;
            }
            break;
        case QDELTA_FRA_INI:
            /* fase de espera pela primeira fração:
             * verificar se houve mudança de fração:
             *      solicitar leitura do instrumento;
             *      recalcular valor a percorrer;
             * após a leitura do instrumento (f_parametro):
             * conferir se permanece na posição de comando inicial;
             *      se posição mudou: sai com erro;
             *      se passou para modo horário: sai com erro;
             *      se atingiu distância máxima: sai com erro;
             *      se incrementou fração, armazene valor DI e avance;
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
            if (flag_int.distancia) {
                /*não recebeu fração inicial em 1000 m*/
                erro_armazenado = ANSWER_ERRO_INST_DIST_QUILO;
                flags_ensaio.Erro = true;
                flags_ensaio.Parametro = false;
                flag_int.distancia == false;
                QDeltaState = QDELTA_SAIDA_OU_ERRO;
            }
            /*recebeu leitura dos parametros do instrumento*/
            if (flags_ensaio.Parametro == true) {
                /*recebeu leitura dos parametros do instrumento*/
                /*verifica se está no modo quilometrico*/
                /*verifica se está na mesma posição de comando*/
                /*verifica se percorreu distância sem registrar fração*/
                if (flags_ensaio.ModoOpr == MO_QUILOMETRICO &&
                        PosC == PosC_inicial &&
                        flag_int.distancia == false) {
                    if (flags_ensaio.ParametroFracao == true) {
                        /*registra primeira fração*/
                        param_ensaio.QUILO.IND_INICIAL = Di;
                        /*inicia contagem de distância para o ensaio*/
                        odometro_total = odometro_fracao;
                        if (odometro_total) {
                            /*corrige distancia de ensaio, com valor inicial na primeira fração*/
                            odometro_out_max += odometro_total;
                            flags_ensaio.DistPend = false;
                            QDeltaState = QDELTA_DISTANCIA;
                        }
                        flags_ensaio.ParametroFracao = false;
                    }
                } else {
                    /*Posiçao de comando alterada, denota falha ou burla*/
                    erro_armazenado = ANSWER_ERRO_INST_POSC_QUILO;
                    if (flags_ensaio.ModoOpr == MO_HORARIO) {
                        /*modo incorreto, denota falha ou burla*/
                        erro_armazenado = ANSWER_ERRO_INST_MODO_QUILO;
                    } else
                        if (flag_int.distancia)
                        /*distância percorrida, sem resposta*/
                        erro_armazenado = ANSWER_ERRO_INST_DIST_QUILO;
                    flags_ensaio.Erro = true;
                    QDeltaState = QDELTA_SAIDA_OU_ERRO;
                }
                flags_ensaio.Parametro = false;
            }
            break;
        case QDELTA_DISTANCIA:
            /* fase de espera pela distância a percorrer:
             * verificar se houve mudança de fração:
             *      solicitar leitura do instrumento;
             * após a leitura do instrumento (f_parametro):
             * conferir se permanece na posição de comando inicial;
             *      se posição mudou: sai com erro;
             *      se passou para modo horário: sai com erro;
             *      se incrementou fração, armazene valor;
             * conferir se atingiu distância máxima:
             *      armazenar fração atual e incrementa estado;
             */
            if (flag_int.distancia) {
                /*percorreu a distancia de ensaio*/
                /*armazenamento temporário*/
                flags_ensaio.DistPend = true;
                flags_ensaio.ParametroFracao = false;
                /*prepara para máximo percurso antes a fração*/
                odometro_out_max += param_ensaio.QUILO.K_ENSAIO;

                flags_ensaio.Parametro = false;
                flag_int.distancia = false;
            }
            if (flag_int.tempo_fracao) {
                /*no evento fração, armazenar pedido de leitura DI*/
                flags_ensaio.PulsoPend = true;
                /*libera pedido e resposta anterior*/
                flags_ensaio.Parametro = false;

                flag_int.tempo_fracao = false;
            }

            if (flags_ensaio.DistPend &&
                    flags_ensaio.MensagemModo == MODO_TRANSPARENTE) {
                /*pedido de leitura após distância percorrida*/
                flags_ensaio.ParametroFracao = true;
                /*se cancela pedido e resposta anterior*/
                flags_ensaio.Parametro = false; /*aguarda nova leitura*/
                /*solicitar parâmetros*/
                flags_ensaio.NovaMensagem = true;
                flags_ensaio.DistPend = false;
            } else
                if (flags_ensaio.PulsoPend &&
                    flags_ensaio.ParametroFracao == false &&
                    flags_ensaio.MensagemModo == MODO_TRANSPARENTE) {
                /*se cancela pedido e resposta anterior*/
                flags_ensaio.Parametro = false; /*aguarda nova leitura*/
                /*solicitar parâmetros*/
                flags_ensaio.NovaMensagem = true;
                flags_ensaio.PulsoPend = false;
            }
            if (flag_int.flag_1_segundo) {
                flag_int.flag_1_segundo = false;
                if (flags_ensaio.MensagemModo == MODO_TRANSPARENTE)
                    /*solicita leitura do instrumento a cada segundo, se ocioso*/
                    flags_ensaio.NovaMensagem = true;
            }

            /*recebeu leitura dos parametros do instrumento*/
            if (flags_ensaio.Parametro == true) {
                /*verifica se está no modo quilometrico*/
                /*verifica se está na mesma posição de comando*/
                if (flags_ensaio.ModoOpr == MO_QUILOMETRICO &&
                        PosC == PosC_inicial) {
                    /*registra o valor atual, a cada fração*/
                    param_ensaio.QUILO.IND_FINAL = Di;
                    if (flags_ensaio.ParametroFracao) {
                        /*se percorreu total, armazena e sai*/
                        QDeltaState = QDELTA_FRA_FINAL;
                        flags_ensaio.ParametroFracao = false;
                    }
                } else {
                    /*Posiçao de comando alterada, denota falha ou burla*/
                    erro_armazenado = ANSWER_ERRO_INST_POSC_QUILO;
                    if (flags_ensaio.ModoOpr == MO_HORARIO)
                        /*modo incorreto, denota falha ou burla*/
                        erro_armazenado = ANSWER_ERRO_INST_MODO_QUILO;
                    flags_ensaio.Erro = true;
                    QDeltaState = QDELTA_SAIDA_OU_ERRO;
                }
                flags_ensaio.Parametro = false;
            }
            break;
        case QDELTA_FRA_FINAL:
            /* fase de espera pela distância em uma valor inteiro de fração:
             * verificar se houve mudança de fração;
             *      armazenar valor atual de distância;
             *      solicita valor do dispositivo indicador;
             * após a leitura do instrumento (f_parametro):
             * conferir se permanece na posição de comando inicial;
             *      se posição mudou: sai com erro;
             *      se passou para modo horário: sai com erro;
             *      se incrementou fração, armazene valor;
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
            } else
                /*verifica se houve falha no percurso*/
                if (flag_int.distancia) {
                /*percorreu 1000 m sem registrar fração*/
                erro_armazenado = ANSWER_ERRO_INST_DIST_QUILO;
                /*informar erro do instrumento*/
                flags_ensaio.Erro = true;
                flags_ensaio.Parametro = false;
                flag_int.distancia == false;
                QDeltaState = QDELTA_SAIDA_OU_ERRO;
            }

            /*lê o valor atual, da última fração*/
            if (flags_ensaio.Parametro == true) {
                /*verifica se está no modo quilometrico*/
                /*verifica se está na mesma posição de comando*/
                if (flags_ensaio.ModoOpr == MO_QUILOMETRICO &&
                        PosC == PosC_inicial) {
                    if (flags_ensaio.ParametroFracao == true) {
                        /*captura distância de mudança de fração*/
                        param_ensaio.QUILO.IND_FINAL_FRACAO = Di;
                        /*
                         *D_FINAL_FRACAO = c * odometro_fracao / K_ENSAIO
                         *D_FINAL_FRACAO = c * pulso / (pulso/km), onde c = 10000 dm/km
                         *para evitar overflow, c = 10000/4 (dm/km)
                         *e D_FINAL_FRACAO = D_FINAL_FRACAO * 4
                         */
                        param_ensaio.QUILO.D_FINAL_FRACAO = (10000 *
                                (odometro_fracao - odometro_total)) /
                                param_ensaio.QUILO.K_ENSAIO;
                        flags_ensaio.ParametroFracao = false;
                        flags_ensaio.Erro = false;
                        QDeltaState = QDELTA_SAIDA_OU_ERRO;
                    }
                } else {
                    /*Posiçao de comando alterada, denota falha ou burla*/
                    erro_armazenado = ANSWER_ERRO_INST_POSC_QUILO;
                    if (flags_ensaio.ModoOpr == MO_HORARIO)
                        /*modo incorreto, denota falha ou burla*/
                        erro_armazenado = ANSWER_ERRO_INST_MODO_QUILO;
                    flags_ensaio.Erro = true;
                    QDeltaState = QDELTA_SAIDA_OU_ERRO;
                }
                flags_ensaio.Parametro = false;
            }
            break;
        case QDELTA_SAIDA_OU_ERRO:
            /* fase de saída do ensaio:
             * a cada segundo, solicitar leitura do instrumento;
             * após a leitura do instrumento (f_parametro):
             * conferir se a velociade já chegou a zero;
             *      se positiva, desacelerar;
             *      se igual a zero, desabilitar entrada e geração de pulsos;
             *      se houve fonte de erro, sinalizar para a máquina de estado;
             */
            /*descarta eventos não relevantes*/
            flag_int.tempo_fracao = false;
            flag_int.distancia = false;
            if (flag_int.flag_1_segundo) {
                /*verifica se atingiu V=0*/
                if (param_ensaio.QUILO.V_ATUAL == 0) {
                    /*desabilita geradores de sinal e entrada de interrupção*/
                    ConfigPulseIn(false);
                    ConfigPulseOutOff();
                    if (flags_ensaio.Erro) {
                        /*indica que falhou*/
                        event[MachineQuilometricoNr] = EVENT_RECEBE_ERRO_INSTRUMENTO_QUILO;
                    }
                } else {
                    /*desacelera V_ATUAL até zero*/
                    param_ensaio.QUILO.V_ENSAIO = 0;
                    acelerador(&param_ensaio.QUILO.V_ATUAL, &param_ensaio.QUILO.V_ENSAIO);
                    gera_velocidade(&param_ensaio.QUILO.V_ATUAL, &param_ensaio.QUILO.K_ENSAIO);
                    /*reinicia contagem de distância*/
                    odometro_out = 0;
                }
                flag_int.flag_1_segundo = false;
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
 * verifica mensagens do PC durante ensaio e responder:
 *  comando ENSAIO_QUILOMETRICO:
 *    valor intermediario dos resultados
 *  comando abortar:
 *    abortar confirmação e sair do ensaio
 *  outros comandos:
 *    ocupado
 *
 * exemplo:
 * se receber comando:
 *  EVENT_RECEBE_COMANDO_PC_MAIN, analisa:
 *      se abortar, EVENT_ENSAIO_ABORTAR_MAIN
 *      se ensaio quilometrico, EVENT_ENSAIO_NOP_MAIN
 *          responder com valores atuais
 *      se outro comando, EVENT_NOP
 *          responder com ocupado
 * se estado anterior for:
 *  STATE_ENSAIO_QUILOMETRICO_MAIN
 *      gera evento EVENT_ENSAIO_NOP_MAIN
 *  STATE_E_Q_ACELERAR_MAIN
 *      gera evento EVENT_E_Q_1_MAIN
 *  STATE_E_Q_DELTA_MAIN
 *      gera evento EVENT_E_Q_2_MAIN
 *  STATE_E_Q_DESACELERAR_MAIN
 *      gera evento EVENT_E_Q_3_MAIN
 *
 * @param cur_state
 * @param new_state
 * @param new_event evento recebido pra chegar aqui
 */
void ensaio_quilom_comando_STF(enum STATE_ID_QUILO cur_state,
        enum STATE_ID_QUILO new_state, enum EVENT_ID_QUILO new_event) {
    char i;

    /*condição de retorno forçada*/
    event[MachineQuilometricoNr] = EVENT_ENSAIO_NOP_QUILO;

    mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
    /*command já preenchido*/
    mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;

    switch (mensagem_Davi.overhead.command) {
        case COMMAND_ABORTAR_DAVI:
            ConfigPulseIn(false);
            ConfigPulseOutOff();
            event[MachineQuilometricoNr] = EVENT_ENSAIO_ABORTAR_QUILO;
            break;
        case COMMAND_QUILO_DAVI:
            /*retorna para teste Delta, se veio de lá*/
            if (cur_state == STATE_E_Q_DELTA) {
                event[MachineQuilometricoNr] = EVENT_E_Q_1;
            }
            /*prepara o cabeçalho de resposta*/
            mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
            /*command já preenchido*/
            mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;
            /*responde com os parametros disponíveis*/
            mensagem_Davi.overhead.length = SIZE_ANSWER_PC_QUILO;
            for (i = 0; i < SIZE_ANSWER_PC_QUILO; i++)
                mensagem_Davi.payload.parameter[i] =
                    param_ensaio.QUILO_A6[i];
            /*corrige endianness dos parametros de 32 bits*/
            for (i = 0; i <= 3; i++) {
                mensagem_Davi.valor_long.size_32[i] =
                        endian_big_to_little(mensagem_Davi.valor_long.size_32[i]);
            }
            ensaio_mensagem_envio_PC();
            break;
        default:
            /*outros comandos, recebe erro ANSWER_OCUPADO_DAVI*/
            /*retorna para teste Delta, se veio de lá*/
            if (cur_state == STATE_E_Q_DELTA) {
                event[MachineQuilometricoNr] = EVENT_E_Q_1; //EVENT_OCUPADO_DELTA_MAIN;
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
 *
 *  *  verificar se está em modo quilometrico com COMMAND_MO_393
 *  armazena valor de Vt
 * na velocidade final:
 *   espera Pulso_in
 *   verifica se é mudança de fração
 *   armazena valor em Fração_Inicial
 * percorre k * distancia pulsos e:
 *   armazena valor da fração_final
 * no valor de Pulso_in
 *   armazena valor da Fração_inteira
 *
 * * monitora atividade e CONVERTE em eventos durante modo horário.
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
void ensaio_quilom_SMF(enum STATE_ID_QUILO state) {

    /*sai, há evento pendente na fila*/
    if (event[MachineQuilometricoNr])
        return;
    /*verifica se houve novo pulso válido na entrada*/
    /*indica que mediu o intervalo de uma mudança indicada pelo instrumento:
     * o PC deve verificar se foi incremento de fração com o comando LE_DI
     * */
    if (flag_int.tempo_fracao) {
        event[MachineQuilometricoNr] = EVENT_PULSE_IN_QUILO;
    } else
        if (flag_int.distancia) {
        event[MachineQuilometricoNr] = EVENT_DISTANCIA_QUILO;
    } else {
        /*verifica se houve alguma mensagem nas portas de comunicação*/
        gera_eventos_mensagens(MachineQuilometricoNr);
        if (event[MachineQuilometricoNr] == EVENT_RECEBE_ERRO_INSTRUMENTO_QUILO) {
            /*transmite erro recebido do instrumento*/
            erro_armazenado = mensagem_393.valor_int.size_int[0];
            event[MachineQuilometricoNr] = EVENT_ERRO_QUILO;
        }
        if (contador_PC_timeout == 0) {
            /*se não receber comando PC, sair do ensaio*/
            event[MachineQuilometricoNr] = EVENT_ERRO_QUILO;
            /*payload de resposta ao erro*/
            erro_armazenado = ANSWER_ERRO_PC_TIMEOUT_QUILO;
            contador_PC_timeout = TIMEOUT_PC_VALUE;
        }
        if (contador_timeout == 0) {
            /*este evento tem menor prioridade. Se nova mensagem, descarta evento*/
            if (flags_ensaio.MensagemModo == MODO_ENSAIO) {
                erro_armazenado = ANSWER_ATRASO_DAVI;
                event[MachineQuilometricoNr] = EVENT_ERRO_QUILO;
            }
            contador_timeout = TIMEOUT_VALUE;
        } else
            if (flag_int.flag_1_segundo) {
            event[MachineQuilometricoNr] = EVENT_1_SEC_QUILO;
        }
    }
}


/**
 * calcula o valor da velocidade atual:
 *
 * com a aceleração de 7 m/s^2 fora da faixa de Vt
 * com aceleração de 1 m/s^2 na faixa de Vt (+- 3 km/h)
 *
 * @param velocidade_atual -> resultado da aceleração
 * @param velocidade_desejada -> valor final desejado, ao final das iterações.
 */
void acelerador(char* velocidade_atual, char* velocidade_desejada) {

    int rascunho;
    char VT_MAX;
    char VT_MIN;

    /*calcula o limite de velocidade de transição de acordo com o RTM*/
    /*item 8.1.2 alínea h (Vt +- 3 km/h*/
    if (param_ensaio.QUILO.VT_TEORICO <= V_ENSAIO_MAX - 3){
        VT_MAX = param_ensaio.QUILO.VT_TEORICO + 3;
    }
    else{
        VT_MAX = V_ENSAIO_MAX;
    }
    if(param_ensaio.QUILO.VT_TEORICO >= V_ENSAIO_MIN + 3){
        VT_MIN = param_ensaio.QUILO.VT_TEORICO - 3;
    }
    else{
        VT_MIN = V_ENSAIO_MIN;
    }

    /*usa rascunho para calcular V_ATUAL*/
    rascunho = *velocidade_atual;
    /*verificar velocidade de entrada e comparar com V_ATUAL*/
    if (rascunho == *velocidade_desejada) {
        /*nada faz!*/
    } else {
        /*ajustar sentido a partir do calculo anterior*/
        if (rascunho < *velocidade_desejada) { /*ascendente*/
            if (param_ensaio.QUILO.VT_REAL) {
                rascunho += 7;
            } else if (rascunho < (int)(VT_MIN - 7)) {
                rascunho += 7;
            } else if (rascunho < VT_MIN) {
                rascunho = VT_MIN;
            } else if (rascunho < VT_MAX) {
                rascunho += 1;
            } else if (rascunho >= VT_MAX) {
                rascunho += 7;
            }
            if (rascunho >= *velocidade_desejada) {
                rascunho = *velocidade_desejada;
            }
        } else { /*descendente*/
            rascunho -= 7;
            if (rascunho <= *velocidade_desejada)/*testa se maior que v*/
                rascunho = *velocidade_desejada;
        }
        /*recalcula valor de CCP para velocidade desejada*/
        *velocidade_atual = rascunho;
    }
}
