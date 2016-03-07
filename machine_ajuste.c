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
#include "machine_ajuste.h"

/******************************************************************************/
/* Constants initializing                                                     */
/******************************************************************************/
static int erro_armazenado = 0;

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/
/**
 * Monitora se:
 *  alguma mensagem válida foi recebida e gera o evento correspondente
 *
 * @param state
 */
void idle_SMF_aju(enum STATE_ID_AJUSTE state) {

    if (current_exclusion[MachineAjusteNr] == OWNED) {
        event[MachineAjusteNr] = EVENT_AUTO_AJUSTE;
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
void idle_STF_aju(enum STATE_ID_AJUSTE cur_state,
        enum STATE_ID_AJUSTE new_state, enum EVENT_ID_AJUSTE new_event){

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
void erro_STF_aju(enum STATE_ID_AJUSTE cur_state,
        enum STATE_ID_AJUSTE new_state, enum EVENT_ID_AJUSTE new_event) {
    erro_handler(MachineAjusteNr, &new_event, &erro_armazenado);
}

/*TODO colocar este código na inicialização*/
//posição de memoria para guardar o valor de calibração
//#define Recarga_Tmr1_ADDRESS 0x3FF //endereço na EEPROM interna, para calibração de frequencia
//
//#rom 0xF00000={2011} //senha para acesso
//#rom int8 (0xF00000+Recarga_Tmr1_ADDRESS) = {46} //valor de calibração inicial
//int Recarga_Tmr1;	//valor de calibração do instrumento
//long int k;			//k do instrumento
//int vel_ptr = 0;	//ponteiro de velocidades do cronotacometro
//
//Recarga_Tmr1=read_eeprom(Recarga_Tmr1_ADDRESS);
//	//caso chip novo, ainda não possui valor de calibração
//	if(Recarga_Tmr1==0xFF || Recarga_Tmr1==0x00)
//		Recarga_Tmr1=46;
//
//void periodo(int vel_ptr);
//void ajusta_vel(void);


//32ciclos até chegar aqui + 5 ciclos p/ reescrever tmr1
//	set_timer1(recarga_tmr1);
    
/**
 * Função de ajuste do DAVI
 * 
 * Recebe do PC:
 *  Valor de tempo (em décimo de us) a receber na entrada de sinal.
 *  Valor de K
 *  Valor da velocidade
 * 
 *  Corrigir o valor medido, com correção no tempo de atraso. Usar fator de correção
 * na rotina de aquisição de tempo.(interrupção).
 *  Gerar velocidade a partir do valor informado de velocidade/K.
 *  Interromper gerador ao comando Abortar.
 * 
 * Retorna ao PC:
 *  Valor de tempo medido entre as bordas (descida -> subida) do sinal de entrada.
 * Esta resposta não necessita de solicitação anterior. A solicitação é o próprio
 * sinal de entrada.
 * 
 * Avaliar no PC através do valor coletado de referência qual a correção no ajuste
 * de medição de tempo. Esta diferença é proporcional ao desvio da fonte de clock
 */
void ajusta_vel(void)
{
	/**************************************************************************
	*		Rotina para ajustar a velocidade, durante ativação do instrumento
	*
	*	void ajusta_vel(void)
	*
	*		Ajusta valor a ser usado no tratamento de CCP, para que a velocidade
	*		gerada esteja dentro do limite de tolerância.
	*		medir a frequencia dos pulsos, usando k=65535 e v=220km/h
	*		que são os limites de utilização do instrumento
	*
	*	argumentos:
	*
	*	retorna:
	*		void;
	*
	**************************************************************************/

	long int temp;
	//lê o valor atual de ajuste, gravado na eeprom
//	Recarga_Tmr1=read_eeprom(Recarga_Tmr1_ADDRESS);
//	k=65535;
//	
//	//rotina de iteração com usuário, para achar valor ótimo de ajuste
//	do
//	{
//		printf(lcd_putc,"\f\Cal. Reload:\n%3U\nSair:0 Grava:255\n",Recarga_Tmr1);
//		//captura valor de recarga do timer1
//		temp=get_int();
//		if((temp>0)&&(temp<255))
//		{
//			//transfere recarga temporariamente,se estiver na faixa válida
//			Recarga_Tmr1=temp;
//			//gera frequencia correspondente à velocidade de 220km/h
//			periodo(11);
//			//CCP_1=k;
//		}
//	//para sair, faça temp=0 ou 255
//	}while(temp>0&&temp<255);
//	
//	//salva ultimo valor de ajuste na eeprom
//	if (temp)
//		write_eeprom(Recarga_Tmr1_ADDRESS,Recarga_Tmr1);
//	//interrompe pulsos
//	gera_velocidade(0);
}

void periodo(int vel_ptr)
{
	/**************************************************************************
	*		Rotina para gerar velocidades do ensaio de cronotacômetro
	*
	*	void periodo(int vel_ptr)
	*
	*	Gera pulsos de calibração de W, correspondente a distancia de 50m
	*	Gera velocidades de 30 a 120km/h
	*
	*	argumentos:
	*		vel_ptr: velocidade do ensaio
	*		para indice 0, são gerados pulsos equivalentes a 50m, para calibrar W
	*		para indices de 1 a 10, as velocidades estão na tabela abaixo:
	*		{30,40,50,60,70,80,90,100,110,120}
	*
	*	retorna:
	*		void;
	*
	**************************************************************************/

	long i,pulsos;
	//aviso para o usuário
//	bip(200);
//	if(!vel_ptr)
//	{
//		//gera 400 pulsos para simular 50m percorridos
//		disable_interrupts(GLOBAL);
//		setup_ccp1(CCP_OFF);
//		
//		// calcula o valor correspondente a 50m
//		//pulsos=400 => 50m @ k=8000
//		pulsos = k/20;
//		printf(lcd_putc,"\fGerando Pulsos..");
//		for (i=1;i<=pulsos;++i)
//		{
//			output_high(pulso);
//			delay_us(2500);
//			output_low(pulso);
//			delay_us(2500);
//		}
//		//bip longo de fim de execução
//		bip(1000);
//	}
//	else
//	{
//		//ajusta ciclo para velocidade
//		set_timer1(0);
//		gera_velocidade(vel_lcd[vel_ptr]);
//		printf(lcd_putc,"\fVeloc= %4U km/h\nk=%5LUpulsos/km",vel_lcd[vel_ptr],k);
//	}
}

