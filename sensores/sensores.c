// sensores.c
// Inclui os ADC (potenciômetros lineares), o DHT22(temperatura/umidade) e o RTC (relógio de tempo real)
// Também é feita a verificação de recursos na máquina (futuramente com sensores reais)

#include "sensores.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include "lcd_i2c.h"
#include "controle_ir.h"
#include "pico/time.h"
#include "atuadores.h" 

#define LED_VERMELHO 12 // LED vermelho: indica que a máquina precisa ser reabastecida
#define BUZZER_PIN 14   // Buzzer: usado para notificações sonoras
extern float agua_ml;
extern float graos_g;
extern bool play_apertado;
const uint MAX_TIMINGS = 85;

typedef enum {
  ESTADO_CONFIG_DIA,
  ESTADO_CONFIG_HORA,
  ESTADO_CONFIG_MINUTOS,
  ESTADO_VALIDACAO,
  ESTADO_FINALIZADO,
  ESTADO_INVALIDO,
} EstadoHorario;

// ---------------------------------- ADC (Potenciômetros) ---------------------------------- //
// Inicializa o ADC e configura os pinos dos potenciômetros
void init_adc() {
  adc_init();
  adc_gpio_init(26); // INTENSITY_POT_PIN
  adc_gpio_init(27); // TEMP_WATER_PIN
  adc_gpio_init(28); // WATER_AMOUNT_PIN
}

// Lê o potenciômetro de intensidade (0 a 100%)
int ler_intensidade() {
  adc_select_input(0); // ADC0 (GPIO26)
  sleep_us(500);       // Aguarda estabilização
  adc_read();          // Descartar primeira leitura
  uint16_t raw_value = adc_read();
  return (raw_value * 100) / 4095; // Converte para percentual
}

// Lê o potenciômetro de temperatura (85°C a 95°C)
float ler_temperatura_desejada() {
  adc_select_input(1); // ADC1 (GPIO27)
  sleep_us(500);
  adc_read();          // Descartar primeira leitura
  uint16_t raw_value = adc_read();

  float percentual = (raw_value * 100.0) / 4095.0;
  return 85.0 + ((percentual * 10.0) / 100.0); // Mapeia para 85°C - 95°C
}

// Lê o potenciômetro de quantidade de água (50 ml a 200 ml)
int ler_quantidade_agua() {
  adc_select_input(2); // ADC2 (GPIO28)
  sleep_us(500);
  adc_read();          // Descartar primeira leitura
  uint16_t raw_value = adc_read();
  return 50 + ((raw_value * 150) / 4095); // Mapeia para 50 ml - 200 ml
}

// ---------------------------------- DHT22 (Temperatura e Umidade) ---------------------------------- //
void read_from_dht(dht_reading *result, const uint DHT_PIN) {
  int data[5] = {0, 0, 0, 0, 0};
  uint last = 1;
  uint j = 0;

  // Pulso de inicialização
  gpio_set_dir(DHT_PIN, GPIO_OUT);
  gpio_put(DHT_PIN, 0);
  sleep_ms(20);
  gpio_set_dir(DHT_PIN, GPIO_IN);

  // Leitura do sensor
  for (uint i = 0; i < MAX_TIMINGS; i++) {
    uint count = 0;
    while (gpio_get(DHT_PIN) == last) {
      count++;
      sleep_us(1);
      if (count == 255) break;
    }
    last = gpio_get(DHT_PIN);
    if (count == 255) break;

    if ((i >= 4) && (i % 2 == 0)) {
      data[j / 8] <<= 1;
      if (count > 50) { // Ajuste fino do timing
        data[j / 8] |= 1;
      }
      j++;
    }
  }

  // Validação dos dados
  if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
    result->humidity = (float)((data[0] << 8) + data[1]) / 10;
    if (result->humidity > 100) {
      result->humidity = data[0];
    }
    result->temp_celsius = (float)(((data[2] & 0x7F) << 8) + data[3]) / 10;
    if (result->temp_celsius > 125) {
      result->temp_celsius = data[2];
    }
    if (data[2] & 0x80) {
      result->temp_celsius = -result->temp_celsius;
    }
  } else {
    printf("Dados inválidos do DHT22\n");
    result->humidity = -1; // Valor de erro
    result->temp_celsius = -1; // Valor de erro
  }
}

float convert_to_fahrenheit(float temp_celsius) {
  return (temp_celsius * 9 / 5) + 32;
}

bool is_valid_reading(const dht_reading *reading) {
  return reading->humidity > 0 && reading->temp_celsius > -40 && reading->temp_celsius < 125;
}

void print_dht_reading(const dht_reading *reading) {
  if (is_valid_reading(reading)) {
    float fahrenheit = convert_to_fahrenheit(reading->temp_celsius);
    printf("Umidade: %.1f%%, Temperatura: %.1f°C (%.1f°F)\n",
           reading->humidity, reading->temp_celsius, fahrenheit);
  } else {
    printf("Erro na leitura do DHT22. Tente novamente.\n");
  }
}


// ---------------------------------- RTC (Relógio de Tempo Real) ---------------------------------- //
// Função para ler dados do RTC
void rtc_read(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *rtc_data) {
  // Configure os pinos I2C para o RTC
  gpio_set_function(sda_pin, GPIO_FUNC_I2C);
  gpio_set_function(scl_pin, GPIO_FUNC_I2C);
  gpio_pull_up(sda_pin);
  gpio_pull_up(scl_pin);

  // Leitura dos dados do RTC
  uint8_t reg = 0x00;
  int ret = i2c_write_blocking(i2c, RTC_ADDR, &reg, 1, true);
  if (ret < 0) {
    printf("Erro ao escrever no RTC\n");
    return;
  }
  ret = i2c_read_blocking(i2c, RTC_ADDR, rtc_data, 7, false);
  if (ret < 0) {
    printf("Erro ao ler do RTC\n");
    return;
  }
}

// Função para formatar os dados do RTC
void format_time(uint8_t *rtc_data, char *time_buffer, char *date_buffer) {
  const char *months[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };

  uint8_t seconds = (rtc_data[0] & 0x0F) + ((rtc_data[0] >> 4) * 10);
  uint8_t minutes = (rtc_data[1] & 0x0F) + ((rtc_data[1] >> 4) * 10);
  uint8_t hours = (rtc_data[2] & 0x0F) + ((rtc_data[2] >> 4) * 10);
  uint8_t date = (rtc_data[4] & 0x0F) + ((rtc_data[4] >> 4) * 10);
  uint8_t month = (rtc_data[5] & 0x0F) + ((rtc_data[5] >> 4) * 10);
  uint16_t year = 2000 + (rtc_data[6] & 0x0F) + ((rtc_data[6] >> 4) * 10);

  snprintf(time_buffer, 64, "%02d:%02d", hours, minutes);
  snprintf(date_buffer, 64, "%02d %s %04d", date, months[month - 1], year);
}

// Função para obter a data atual do RTC
void get_current_date(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *day, uint8_t *month, uint8_t *year) {
  uint8_t rtc_data[7];
  rtc_read(i2c, sda_pin, scl_pin, rtc_data);

  *day = (rtc_data[4] & 0x0F) + ((rtc_data[4] >> 4) * 10);
  *month = (rtc_data[5] & 0x0F) + ((rtc_data[5] >> 4) * 10);
  *year = (rtc_data[6] & 0x0F) + ((rtc_data[6] >> 4) * 10);
}

// Função para incrementar a data
void increment_date(uint8_t *day, uint8_t *month, uint8_t *year) {
  const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint8_t max_days = days_in_month[*month - 1];

  // Verifica ano bissexto para fevereiro
  if (*month == 2 && ((*year % 4 == 0 && *year % 100 != 0) || (*year % 400 == 0))) {
    max_days = 29;
  }

  if (*day < max_days) {
    (*day)++;
  } else {
    *day = 1;
    if (*month < 12) {
      (*month)++;
    } else {
      *month = 1;
      (*year)++;
    }
  }
}


void configurar_dia(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *day, uint8_t *month, uint8_t *year, const char *key) {
  uint8_t current_day, current_month, current_year;
  get_current_date(i2c, sda_pin, scl_pin, &current_day, &current_month, &current_year);

  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("SCHEDULE FOR:");
  lcd_set_cursor(2, 0);
  lcd_print("+ : TOMORROW");
  lcd_set_cursor(3, 0);
  lcd_print("- : TODAY");

  uint32_t start_time = to_ms_since_boot(get_absolute_time());

  while (to_ms_since_boot(get_absolute_time()) - start_time < 30000) { // 30 segundos timeout
    if (strcmp(key, "+") == 0) {
      increment_date(&current_day, &current_month, &current_year); // Incrementa para amanhã
      *day = current_day;
      *month = current_month;
      // *year = 2000 + current_year; // Aplica o offset de 2000 para o ano correto
      break;
    } else if (strcmp(key, "-") == 0) {
      *day = current_day;   // Mantém o dia atual
      *month = current_month;
      //*year = 2000 + current_year; // Aplica o offset de 2000 para o ano correto
      break;
    }
  }

  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("DATE CONFIRMED!");
  sleep_ms(1000);
}




uint8_t read_digit(const char *key, uint32_t timeout_ms) {
  uint32_t start_time = to_ms_since_boot(get_absolute_time());
  uint8_t digit = 0xFF; // Valor inválido por padrão

  while (to_ms_since_boot(get_absolute_time()) - start_time < timeout_ms) {
    if (strlen(key) == 1 && isdigit(key[0])) {
      digit = key[0] - '0'; // Converte o caractere para número
      break;
    }
  }
  sleep_ms(100);
  return digit;
}

void configurar_hora(uint8_t *hour, const char *key) {
  uint8_t first_digit, second_digit;
  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("SET HOURS:");
  lcd_set_cursor(2, 2);
  lcd_print(":");

  // Leia o primeiro dígito
  first_digit = read_digit(key, 30000); // 30 segundos de timeout
  if (first_digit > 2) first_digit = 0; // Validação inicial
  lcd_set_cursor(2, 0);
  char buffer[2] = {first_digit + '0', '\0'};
  lcd_print(buffer);
  sleep_ms(1000);

  // Leia o segundo dígito
  second_digit = read_digit(key, 30000); // 30 segundos de timeout
  if (first_digit == 2 && second_digit > 3) second_digit = 0; // Valida até 23h
  lcd_set_cursor(2, 1);
  char buffer1[2] = {second_digit + '0', '\0'};
  lcd_print(buffer1);
  sleep_ms(2000);

  *hour = (first_digit * 10) + second_digit;// Combine os dois dígitos

  lcd_set_cursor(0, 0);
  lcd_print("HOURS OK!         ");
  printf("Hora configurada para %02d\n", *hour);
  sleep_ms(2000);
}

void configurar_minutos(uint8_t *minutes, const char *key) {
  uint8_t first_digit, second_digit;
  // lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("SET MINUTES:");

  // Leia o primeiro dígito
  sleep_ms(1500);
  first_digit = read_digit(key, 30000); // 30 segundos de timeout
  if (first_digit > 5) first_digit = 0; // Valida minutos
  lcd_set_cursor(2, 3);
  char buffer[2] = {first_digit + '0', '\0'};
  lcd_print(buffer);
  sleep_ms(1000);

  // Leia o segundo dígito
  second_digit = read_digit(key, 30000); // 30 segundos de timeout
  lcd_set_cursor(2, 4);
  char buffer1[2] = {second_digit + '0', '\0'};
  lcd_print(buffer1);
  sleep_ms(2000);

  *minutes = first_digit * 10 + second_digit; // Calcular os minutos completos

  lcd_clear();
  lcd_print("MIN CONFIRMED!");
  printf("Minutos configurados para %02d\n", *minutes);
  sleep_ms(1000);
}

// Configuração de horário agendado
HorarioConfigurado configurar_horario(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, const char *tecla)
{
  HorarioConfigurado horario = {0, 0, 0, 0, false};
  EstadoHorario estado_atual = ESTADO_CONFIG_DIA;

  while (true) {
    switch (estado_atual) {
      case ESTADO_CONFIG_DIA: {
          uint8_t ano;
          configurar_dia(i2c, sda_pin, scl_pin, &horario.dia, &horario.mes, &ano, tecla);
          estado_atual = ESTADO_CONFIG_HORA;
          break;
        }

      case ESTADO_CONFIG_HORA:
        configurar_hora(&horario.hora, tecla);
        estado_atual = ESTADO_CONFIG_MINUTOS;
        break;

      case ESTADO_CONFIG_MINUTOS:
        configurar_minutos(&horario.minutos, tecla);
        estado_atual = ESTADO_VALIDACAO;
        break;

      case ESTADO_VALIDACAO: {
          uint8_t rtc_data[7];
          rtc_read(i2c, sda_pin, scl_pin, rtc_data);

          uint8_t current_day = (rtc_data[4] & 0x0F) + ((rtc_data[4] >> 4) * 10);
          uint8_t current_month = (rtc_data[5] & 0x0F) + ((rtc_data[5] >> 4) * 10);
          uint8_t current_hour = (rtc_data[2] & 0x0F) + ((rtc_data[2] >> 4) * 10);
          uint8_t current_minute = (rtc_data[1] & 0x0F) + ((rtc_data[1] >> 4) * 10);

          // Verifica se a data/hora é futura
          if (horario.mes > current_month ||
              (horario.mes == current_month && horario.dia > current_day) ||
              (horario.mes == current_month && horario.dia == current_day &&
               (horario.hora > current_hour || (horario.hora == current_hour && horario.minutos > current_minute)))) {
            horario.horario_valido = true;
            estado_atual = ESTADO_FINALIZADO;
          } else {
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Invalid Date/Time!");
            lcd_set_cursor(2, 0);
            lcd_print("PRESS PLAY TO RESET:");
            sleep_ms(3000);
            estado_atual = ESTADO_INVALIDO;
          }
          break;
        }

      case ESTADO_FINALIZADO:
        lcd_clear();
        char buffer[32];
        lcd_set_cursor(0, 0);
        lcd_print("COFFEE SCHEDULED!");
        snprintf(buffer, sizeof(buffer), "%02d/%02d", horario.dia, horario.mes);
        lcd_set_cursor(2, 0);
        lcd_print("DATE: ");
        lcd_set_cursor(2, 6);
        lcd_print(buffer);
        snprintf(buffer, sizeof(buffer), "%02d:%02d", horario.hora, horario.minutos);
        lcd_set_cursor(3, 0);
        lcd_print("TIME: ");
        lcd_set_cursor(3, 6);
        lcd_print(buffer);
        sleep_ms(3000);
        return horario;

      case ESTADO_INVALIDO:
        if (strcmp(tecla, "PLAY") == 0) {
          estado_atual = ESTADO_CONFIG_DIA; // Reinicia a configuração
        }
        break;

      default:
        break;
    }
  }
}

// Função para verificar a quantidade de água e grãos de café na máquina
// Verifica se há recursos suficientes para a quantidade de xícaras selecionadas.
// Caso os recursos sejam insuficientes, alerta o usuário para reabastecer.
void verificar_recursos_simulado(int xicaras, int agua_por_xicara) {
  float graos_necessarios = xicaras * 10; // 10g por xícara
  float agua_necessaria = xicaras * agua_por_xicara; // considera a quantidade de água escolhida
  bool precisa_reabastecer = false;

  if (agua_ml < agua_necessaria) { // Verifica se há água suficiente
    gpio_put(LED_VERMELHO, 1);     // Acende o LED vermelho
    play_beep_pattern(BUZZER_PIN, 400, 400, 300, 4, 0.8); // Som de alerta
    lcd_clear();
    blink_text("REFILL MACHINE!", 0, 2, 3, 500);
    lcd_set_cursor(2, 0);
    lcd_print("PRESS PLAY TO FILL:");
    precisa_reabastecer = true;
  }

  if (graos_g < graos_necessarios) { // Verifica se há grãos suficientes
    gpio_put(LED_VERMELHO, 1);       // Acende o LED vermelho
    play_beep_pattern(BUZZER_PIN, 400, 400, 300, 4, 0.8); // Som de alerta
    lcd_clear();
    blink_text("REFILL MACHINE!", 0, 2, 3, 500);
    lcd_set_cursor(2, 0);
    lcd_print("PRESS PLAY TO FILL:");
    precisa_reabastecer = true;
  }

  if (precisa_reabastecer) {
    while (!play_apertado) { // Aguarda o usuário pressionar PLAY
      sleep_ms(200);         // Loop de espera
    }

    // Simula reabastecimento de grãos e água
    graos_g = 250.0;           // Grãos reabastecidos
    agua_ml = 1000.0;          // Água reabastecida
    gpio_put(LED_VERMELHO, 0); // desativa LED vermelho

    // Sinaliza que a máquina está pronta novamente
    lcd_clear();
    lcd_set_cursor(1, 4);
    lcd_print("READY AGAIN!");
    play_success_tone(BUZZER_PIN);// som de máquina abastecida
    sleep_ms(2000);
    play_apertado = false; // Reseta a flag
  }
}