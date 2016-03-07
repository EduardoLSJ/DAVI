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

#include "machine_main.h"
#include "machine_calibracao.h"
#include "machine_prot_davi.h"
#include "machine_ajuste.h"
#include "interrupts.h"
#include "uart1.h"

/******************************************************************************/
/* Constants initializing                                                     */
/******************************************************************************/
static int erro_armazenado = 0;

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/
/**
 *  Libera a máquina atual de execução
 * 
 * @param state
 */
void release_STF_cal(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event){
    /*recupera comunicação com mototaximetro*/
    vInitU1(USART_BRG_1200, false);
    current_exclusion[MachineCalibracaoNr] = NOT_OWNED;
}

/**
 * inicia a execução de tarefas desta máquina
 * 
 * @param state
 */
void mutex_SMF_cal(enum STATE_ID_CALIBRACAO state){
    if (current_exclusion[MachineCalibracaoNr] == OWNED) {
        event[MachineCalibracaoNr] = EVENT_LOCK_CALIBRACAO;
    }
}

/**
 * Monitora se:
 *  alguma mensagem válida foi recebida e gera o evento correspondente
 *
 * @param state
 */
void idle_SMF_cal(enum STATE_ID_CALIBRACAO state) {
    /*preocupa-se*/
    gera_eventos_mensagens(MachineCalibracaoNr);
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
void idle_STF_cal(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event){

}

/**
 * rotina de passagem de comando, não efetua monitoração
 * 
 * @param state
 */
void none_SMF_cal(enum STATE_ID_CALIBRACAO state) {

}

/**
 * Função de calibração do DAVI
 * 
 * Durante o ensaio, ao receber (EVENT_PULSE_IN_CALIBRACAO):
 *      atende à solicitação de pulso de entrada 
 *      renova os dados de armazenamento.
 * 
 * @param cur_state
 * @param new_state 
 * @param new_event evento de entrada na função. EVENT_PULSE_IN_CALIBRACAO
 */
void calibra_STF_cal(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event){
    static uint32_t odometro_old = 0;/**armazena o valor total da medição anterior*/

    /*coletar o valor de tempo*/
    param_ensaio.CALIBRA.TEMPO_32 = tempo_fracao;
    /*coletar o numero de pulsos de saída no intervalo*/
    param_ensaio.CALIBRA.N_PULSO = odometro_fracao - odometro_old;
    odometro_old = odometro_fracao;    
    /*coleta o número de pulsos de entrada monitorados*/
    param_ensaio.CALIBRA.N_JANELAS_IN = odometro_in;
}

/**
 * Monitora eventos durante o ensaio:
 * 
 *  A medição de tempo foi efetuada, ao receber 
 *      gerar evento EVENT_PULSE_IN_CALIBRACAO
 *      para armazenar resultado
 *  Houve algum comando,
 *      gerar evento EVENT_RECEBE_COMANDO_PC_CALIBRACAO
 *  houve algum erro,
 *      armazena erro (PC_timeout)
 *      gera evento EVENT_ERRO_CALIBRACAO
 *  ignorar os outros eventos:
 *      tempo_1s
 *      mensagem_instrumento
 *      erro_instrumento
 *
 * @param state
 */
void calibra_SMF_cal(enum STATE_ID_CALIBRACAO state) {
    /*verifica condição de retorno forçada*/
    if(event[MachineCalibracaoNr] == EVENT_ENSAIO_NOP_CALIBRACAO)
        event[MachineCalibracaoNr] = EVENT_NONE_CALIBRACAO;
    
    /*verifica se houve novo pulso válido na entrada*/
    if (flag_int.tempo_fracao) {
        flag_int.tempo_fracao = false;
        event[MachineCalibracaoNr] = EVENT_PULSE_IN_CALIBRACAO;
    } else {
        /*verifica se houve alguma mensagem nas portas de comunicação*/
        gera_eventos_mensagens(MachineCalibracaoNr);
        /*descarta mensagens do instrumento*/
        le_mensagem_393(apaga);
        if (contador_PC_timeout == 0) {
            /*se não receber comando PC, sair do ensaio*/
            event[MachineCalibracaoNr] = EVENT_ERRO_CALIBRACAO;
            /*payload de resposta ao erro*/
            erro_armazenado = ANSWER_ERRO_PC_TIMEOUT_MAIN;
            contador_PC_timeout = TIMEOUT_PC_VALUE;
        }
    }
}

/**
 * solicita reajuste do gerador a partir dos parâmetros do PC.
 *
 * Retorna ao PC:
 *      A6 85 00 08 (4BYTES TEMPO32) (4BYTES N_PULSO) CRC16
 *  Valor de tempo medido entre as bordas (descida -> subida) do sinal de entrada.
 * Esta resposta não necessita de solicitação anterior. A solicitação é o próprio
 * sinal de entrada.
 * 
 * Avaliar no PC através do valor coletado de referência qual a correção no ajuste
 * de medição de tempo. Esta diferença é proporcional ao desvio da fonte de clock
 *  * 
 * Retorna ao PC:
 *  Valor de tempo medido entre as bordas (descida -> subida) do sinal de entrada.
 * Esta resposta não necessita de solicitação anterior. A solicitação é o próprio
 * sinal de entrada.
 * 
 * Avaliar no PC através do valor coletado de referência qual a correção no ajuste
 * de medição de tempo. Esta diferença é proporcional ao desvio da fonte de clock
---------------------
 *  * Na primeira entrada (EVENT_LOCK_CALIBRACAO)
 *      reinicia os valores dos contadores.
 *      chama a função de comando para responder ao PC.
 *  
 * verifica mensagens do PC durante ensaio e responder:
 *  comando COMMAND_CALIBRACAO_DAVI:
 *    responde com o valor armazenado dos resultados
 *    se há alteração nos parâmetros, reconfigurar o gerador (calibra_pwm)
 *  comando abortar:
 *    abortar confirmação e sair do ensaio
 *  outros comandos:
 *    ocupado
 *
 * exemplo:
 * se receber comando:
 *  EVENT_RECEBE_COMANDO_PC_CALIBRACAO, analisa:
 *      se abortar, EVENT_ENSAIO_ABORTAR_CALIBRACAO
 *      se ensaio calibração, EVENT_NONE_CALIBRACAO
 *          responder com valores atuais
 *      se outro comando, EVENT_NONE_CALIBRACAO
 *          responder com ocupado
 *
 * @param cur_state
 * @param new_state
 * @param new_event evento recebido pra chegar aqui
 */
void calibra_comando_STF(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event){
    char i;

    /*condição de retorno forçada*/
    event[MachineCalibracaoNr] = EVENT_ENSAIO_NOP_CALIBRACAO;

    mensagem_Davi.overhead.head = STX_MOTOVER_DAVI;
    /*command já preenchido*/
    mensagem_Davi.overhead.format = FORMAT_HEX_DAVI;

    switch (mensagem_Davi.overhead.command) {
        case COMMAND_ABORTAR_DAVI:
            event[MachineCalibracaoNr] = EVENT_ENSAIO_ABORTAR_CALIBRACAO;
            break;
        case COMMAND_CALIBRACAO_DAVI:
            /*reinicia parâmetros na entrada de calibração*/
            if(new_event == EVENT_LOCK_CALIBRACAO){
                /*permite comunicação com termohigrômetro*/
                vInitU1(USART_BRG_9600, true);
                param_ensaio.CALIBRA.K_ENSAIO = 0;
                param_ensaio.CALIBRA.V_ENSAIO = 0;
                param_ensaio.CALIBRA.TEMPO_32 = 0;
                param_ensaio.CALIBRA.N_PULSO = 0;
                param_ensaio.CALIBRA.N_JANELAS_IN = 0;
                event[MachineCalibracaoNr] = EVENT_NONE_CALIBRACAO;
            }
            /*ajusta o gerador, se necessário*/
            calibra_pwm();
            
            /*responde com os parametros disponíveis*/
            mensagem_Davi.overhead.length = SIZE_ANSWER_COMMAND_CALIBRACAO_DAVI;
            for (i = 0; i < SIZE_ANSWER_COMMAND_CALIBRACAO_DAVI; i++)
                mensagem_Davi.payload.parameter[i] =
                    param_ensaio.CALIBRA_A6[i];
            /*corrige endianness dos parametros de 32 bits*/
            for (i = 0; i < (SIZE_ANSWER_COMMAND_CALIBRACAO_DAVI / 
                    sizeof(uint32_t)); i++) {
                mensagem_Davi.valor_long.size_32[i] =
                        endian_big_to_little(mensagem_Davi.valor_long.size_32[i]);
            }

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
    le_mensagem_davi(apaga);
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
void erro_STF_cal(enum STATE_ID_CALIBRACAO cur_state,
        enum STATE_ID_CALIBRACAO new_state, enum EVENT_ID_CALIBRACAO new_event) {
    erro_handler(MachineCalibracaoNr, &new_event, &erro_armazenado);
}

/**
 * Configura o gerador de sinais, a partir dos parâmetros recebidos
 * 
 * Recebe do PC:
 *  Pulso (descida -> subida) a receber na entrada de sinal. TEMPO DESCONHECIDO.
 *  Valor de K
 *  Valor da velocidade
 * 
 * A4 85 00 05 (1BYTE VENSAIO) + (4BYTES K_ENSAIO) CRC16
 * A6 85 00 08 (4BYTES TEMPO32) + (4BYTES N_PULSO) CRC16
 * 
 *  Se velocidade = 0, desligar interrupção para a geração de sinal.
 *  Se houve alteração nos parâmetros de configuração:
 *      reinicia medição;
 *      altera o gerador.
 *      Gera evento EVENT_NOVA_CALIBRACAO.
 *  Se a solicitação mantiver os parâmetros:
 *      não altera a medição.
 *      Gera evento  EVENT_NONE_CALIBRACAO.
 * 
 *  NÃO HÁ AJUSTE NO FATOR DE CORREÇÃO. O TEMPO DE ENTRADA NÃO É CONHECIDO O DAVI.
 *  Apresentar o valor medido, com correção no tempo de atraso. Usar fator de correção
 * na rotina de aquisição de tempo.(interrupção).
 *  Gerar velocidade a partir do valor informado de velocidade/K.
 *  Interromper gerador ao comando Abortar.
 */
void calibra_pwm(void)
{
    uint32_t k_temp;
    uint8_t v_temp;
    uint8_t i;
    
    /*armazena os valores temporariamente*/
    k_temp = param_ensaio.CALIBRA.K_ENSAIO;
    v_temp = param_ensaio.CALIBRA.V_ENSAIO;

    /*recebe e armazena os valores dos parametros recebidos pelo comando CALIBRA*/
    for (i = 0; i <  SIZE_COMMAND_CALIBRACAO_DAVI ; i++)
    param_ensaio.CALIBRA_A4[i] = mensagem_Davi.payload.parameter[i];
    
    /*coleta valor de k*/
    param_ensaio.CALIBRA.K_ENSAIO = 
            endian_shortint(param_ensaio.CALIBRA.K_ENSAIO);
    /*V_ENSAIO não precisa de correção*/

    /*converter valor de entrada, se estiver em BCD*/
    if (mensagem_Davi.overhead.format == FORMAT_BCD_DAVI) {
        param_ensaio.CALIBRA.K_ENSAIO =
                Converter_BCD_to_HEX((uint32_t)param_ensaio.CALIBRA.K_ENSAIO);
    }

    /*verifica se houve mudança nos parâmetros*/
    if ((param_ensaio.CALIBRA.V_ENSAIO != v_temp) ||
            (param_ensaio.CALIBRA.K_ENSAIO != k_temp)) {
        /*habilita a contagem de tempo e
                     redefine o valor inicial do contador de tempo*/
                    di();
                    odometro_in = 0;
                    PIE2bits.TMR3IE = false;
                    WriteTimer3(0);
                    PIR2bits.TMR3IF = false;
                    PIE2bits.TMR3IE = true;
                    ConfigPulseIn(true);/*habilita a contagem de pulsos*/
                    ei();

        /*reinicia contadores do ensaio*/
        odometro_out = 0; /*pulsos de saída total do gerador*/
        odometro_in = 0;/*número de janelas de medição*/
        flag_int.tempo_fracao = false;
        /*se velocidade > 0 prepara gerador*/
        /*se velocidade = 0 desliga gerador*/
        gera_velocidade(&param_ensaio.CALIBRA.V_ENSAIO, &param_ensaio.CALIBRA.K_ENSAIO);
    }    
}
