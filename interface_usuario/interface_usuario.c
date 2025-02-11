// interface_usuario.c
// Exibição de menus, telas e interação com o usuário

#include "interface_usuario.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>
#include "lcd_i2c.h"
#include "sensores.h"
#include "atuadores.h"
#include "controle_ir.h"
#include "estado.h"

#define DHT_PIN 8 // DHT22 usado para monitorar temperatura/umidade ambiente
#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5
#define BUZZER_PIN 14 // Buzzer para notificações sonoras

extern float agua_ml;
extern float graos_g;
extern Estado estado_atual;
extern int xicaras;
extern bool play_apertado;
extern bool tecla_pressionada;
extern bool preparo_agora;
extern char tecla[16];

// -------------------------------------------------------------------------------------------------- //
// Funções de Tela e Menu

void perguntar_quantidade_xicaras() {
  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("HOW MANY CUPS?");
  lcd_set_cursor(2, 0);
  lcd_print("- FROM 1 TO 5");
  lcd_set_cursor(3, 0);
  lcd_print("- 0 TO EXIT");
}

void perguntar_quando_preparar() {
  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("START TIME:");
  lcd_set_cursor(2, 0);
  lcd_print("1-NOW");
  lcd_set_cursor(3, 0);
  lcd_print("2-SCHEDULE");
}

// -------------------------------------------------------------------------------------------------- //
// Tela Inicial e Monitoramento

// Função que exibe a tela inicial com dados de B(beans = grãos de café) e W (water = água) atualizados
void exibir_tela_inicial() {
  gpio_put(7, 1); // Acende o LED verde para indicar que a máquina está ligada
  static float last_agua_ml = -1.0;
  static float last_graos_g = -1.0;

  lcd_clear();
  type_effect(" IT'S COFFEE TIME!", 0, 50);
  sleep_ms(500);

  if (agua_ml != last_agua_ml || graos_g != last_graos_g) {
    char status[32];
    snprintf(status, sizeof(status), "B:%.0fg|W:%.2fL", graos_g, agua_ml / 1000);
    type_effect(status, 2, 100);
    last_agua_ml = agua_ml;
    last_graos_g = graos_g;
  }
}

// Função que exibe as condições ambientes atualizadas na tela inicial
void exibir_temperatura_umidade_ambiente() {
  dht_reading reading;
  read_from_dht(&reading, DHT_PIN);

  lcd_set_cursor(3, 0);
  if (is_valid_reading(&reading)) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.1fC|H:%.1f%%", reading.temp_celsius, reading.humidity);
    lcd_print(buffer);
  } else {
    lcd_print("Error!");
    play_error_tone(BUZZER_PIN);
  }
  sleep_ms(300);
}

// Função que exibe o relógio HH:MM na tela inicial
void exibir_relogio() {
  uint8_t rtc_data[7];
  char time_buffer[6];
  rtc_read(I2C_PORT, SDA_PIN, SCL_PIN, rtc_data);

  uint8_t hours = (rtc_data[2] & 0x0F) + ((rtc_data[2] >> 4) * 10);
  uint8_t minutes = (rtc_data[1] & 0x0F) + ((rtc_data[1] >> 4) * 10);

  snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d", hours, minutes);
  lcd_set_cursor(3, 15);
  lcd_print(time_buffer);
  sleep_ms(300);
}

// -------------------------------------------------------------------------------------------------- //
// Função de callback para processar comandos do controle IR
// Mapeia botões do controle para ações específicas, como iniciar preparo, definir horário, etc
void callback_ir(uint16_t address, uint16_t command, int type) {
  const char* key = get_key_name(command);

  // Verifica se uma tecla válida foi pressionada
  if (strlen(key) > 0) {
    tecla_pressionada = true;
    strncpy(tecla, key, sizeof(tecla));
    tecla[sizeof(tecla) - 1] = '\0'; // Garante null-terminator
  }

  if (strcmp(key, "PLAY") == 0) {
    play_apertado = true; // Marca que o PLAY foi pressionado
  } else if (estado_atual == ESTADO_QUANTIDADE_XICARAS) {
    if (strcmp(key, "0") == 0) { // se o 0 for pressionado retorna ao ínicio
      lcd_clear();
      exibir_tela_inicial();
      estado_atual = ESTADO_TELA_INICIAL; // Retorna à tela inicial
    } else if (strcmp(key, "1") == 0 || strcmp(key, "2") == 0 ||
               strcmp(key, "3") == 0 || strcmp(key, "4") == 0 || strcmp(key, "5") == 0) {
      xicaras = key[0] - '0'; // Converte a tecla para número de xícaras desejadas
      estado_atual = ESTADO_QUANDO_PREPARAR;
    } else {
      lcd_clear();
      lcd_set_cursor(0, 0);
      lcd_print("INVALID KEY"); // caso o usuário aperte uma tecla diferente
      lcd_set_cursor(2, 0);
      lcd_print("PLEASE SELECT 1 TO 5");
      sleep_ms(1000);
    }
  } else if (estado_atual == ESTADO_QUANDO_PREPARAR) { // escolha do usuário de preparar logo ou agendar
    if (strcmp(key, "1") == 0) {
      preparo_agora = true;
      estado_atual = ESTADO_PREPARANDO;
    } else if (strcmp(key, "2") == 0)
    {
      preparo_agora = false;
      estado_atual = ESTADO_PROGRAMANDO;
    }
  }
}