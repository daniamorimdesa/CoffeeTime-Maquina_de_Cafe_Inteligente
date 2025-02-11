#include "estado.h"
#include "interface_usuario.h"
#include "processos_internos.h"
#include "controle_ir.h"
#include "sensores.h"
#include "lcd_i2c.h"
#include <stdio.h>
#include <stdint.h>

// Definição dos pinos
// pinos compartilhados pelo LCD e RTC (seus endereços estão definidos em suas bibliotecas)
#define I2C_PORT i2c0        // comunicação i2c para o display LCD e o RTC
#define SDA_PIN 4
#define SCL_PIN 5

// Variáveis globais
float agua_ml = 1000.0;         // Reservatório inicial de 1 litro
float graos_g = 250.0;          // Reservatório inicial de 250g de grãos de café (cada xícara utiliza 10g de café)
int xicaras = 0;                // Quantidade de xícaras de café
//buffer que armazena o horário de preparo desejado
uint8_t dia_config, mes_config, hora_config, minutos_config;
bool play_apertado = false;     // Indica se o botão PLAY foi pressionado
bool saudacao_exibida = false;  // flag para exibir apenas uma vez "it's coffee time"
bool preparo_agora = false;     // flag para início de preparo da bebida
bool tecla_pressionada = false;
char tecla[16] = "";
HorarioConfigurado horario_configurado = {0, 0, 0, 0, false};
Estado estado_atual = ESTADO_TELA_INICIAL;
// garante que não haja flicker nos estados de quantidade de xícaras e quando preparar
Estado ultimo_estado_exibido = ESTADO_TELA_INICIAL;

// Monitora o estado da máquina e chama a função correspondente baseado no estado atual
void gerenciar_estado() {
  switch (estado_atual)
  {
    case ESTADO_TELA_INICIAL:
      if (!saudacao_exibida) {
        exibir_tela_inicial();
        saudacao_exibida = true;
        ultimo_estado_exibido = ESTADO_TELA_INICIAL;  // Garante que este estado foi exibido
      } else {

        exibir_relogio();                             // Atualiza o relógio continuamente
        exibir_temperatura_umidade_ambiente();        // Atualiza condições do ambiente
      }

      if (play_apertado) {
        estado_atual = ESTADO_QUANTIDADE_XICARAS;
        play_apertado = false; // Reseta a flag
        ultimo_estado_exibido = ESTADO_TELA_INICIAL; // Força a atualização no próximo estado
      }
      break;

    case ESTADO_QUANTIDADE_XICARAS:
      if (ultimo_estado_exibido != ESTADO_QUANTIDADE_XICARAS) {
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("HOW MANY CUPS?");
        lcd_set_cursor(2, 0);
        lcd_print("- FROM 1 TO 5");
        lcd_set_cursor(3, 0);
        lcd_print("- 0 TO EXIT");
        ultimo_estado_exibido = ESTADO_QUANTIDADE_XICARAS; // Atualiza o estado exibido
      }
      break;

    case ESTADO_QUANDO_PREPARAR:
      if (ultimo_estado_exibido != ESTADO_QUANDO_PREPARAR) {
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("START TIME:");
        lcd_set_cursor(2, 0);
        lcd_print("1-NOW");
        lcd_set_cursor(3, 0);
        lcd_print("2-SCHEDULE");
        ultimo_estado_exibido = ESTADO_QUANDO_PREPARAR; // Atualiza o estado exibido
      }
      break;

    case ESTADO_PREPARANDO:
      preparar_cafe(xicaras);
      break;

    case ESTADO_PROGRAMANDO: // estado para agendar o preparo do café
      horario_configurado = configurar_horario(I2C_PORT, SDA_PIN, SCL_PIN, tecla);
      if (horario_configurado.horario_valido) {
        estado_atual = ESTADO_AGUARDANDO;
      } else {
        estado_atual = ESTADO_TELA_INICIAL;
      }
      break;

    case ESTADO_AGUARDANDO: { // estado que compara o tempo atual com o tempo agendado para iniciar o preparo
        uint8_t rtc_data[7];
        rtc_read(I2C_PORT, SDA_PIN, SCL_PIN, rtc_data); // lê o tempo atual

        uint8_t current_day = (rtc_data[4] & 0x0F) + ((rtc_data[4] >> 4) * 10);
        uint8_t current_month = (rtc_data[5] & 0x0F) + ((rtc_data[5] >> 4) * 10);
        uint8_t current_hour = (rtc_data[2] & 0x0F) + ((rtc_data[2] >> 4) * 10);
        uint8_t current_minute = (rtc_data[1] & 0x0F) + ((rtc_data[1] >> 4) * 10);

        //se for igual, vai para o preparo do café
        if (current_day == horario_configurado.dia &&
            current_month == horario_configurado.mes &&
            current_hour == horario_configurado.hora &&
            current_minute == horario_configurado.minutos) {
          estado_atual = ESTADO_PREPARANDO;
        }
        break;
      }

    default:
      estado_atual = ESTADO_TELA_INICIAL;
      break;
  }
}
