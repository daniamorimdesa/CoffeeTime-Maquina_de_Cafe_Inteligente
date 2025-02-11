// sensores.h 
// Inclui os ADC (potenciômetros lineares), o DHT22(temperatura/umidade) e o RTC (relógio de tempo real)
#ifndef SENSORES_H
#define SENSORES_H

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "controle_ir.h"
#include "lcd_i2c.h"

// Endereço do RTC
#define RTC_ADDR 0x68

// Tipos e estruturas

//Estrutura para armazenar temperatura e umidade
typedef struct {
  float humidity;
  float temp_celsius;
} dht_reading;

// Estrutura para armazenar o horário configurado
typedef struct {
    uint8_t dia;
    uint8_t mes;
    uint8_t hora;
    uint8_t minutos;
    bool horario_valido; // Indica se o horário configurado é válido
} HorarioConfigurado;

// Funções para sensores de ADC (Potenciômetros)
void init_adc();
int ler_intensidade();            // Lê a intensidade do café (0 a 100%)
float ler_temperatura_desejada(); // Lê a temperatura desejada (85°C a 95°C)
int ler_quantidade_agua();        // Lê a quantidade de água desejada (50 ml a 200 ml)

// Funções para o sensor DHT22
void read_from_dht(dht_reading *result, const uint DHT_PIN);
float convert_to_fahrenheit(float temp_celsius);
bool is_valid_reading(const dht_reading *reading);
void print_dht_reading(const dht_reading *reading);

// Funções para o RTC DS1307
void rtc_read(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *rtc_data);
void format_time(uint8_t *rtc_data, char *time_buffer, char *date_buffer);
void get_current_date(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *day, uint8_t *month, uint8_t *year);
void increment_date(uint8_t *day, uint8_t *month, uint8_t *year);
void configurar_dia(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *day, uint8_t *month, uint8_t *year, const char *key);
uint8_t read_digit(const char *key, uint32_t timeout_ms);
void configurar_hora(uint8_t *hour, const char *key);
void configurar_minutos(uint8_t *minutes, const char *key);

// Declaração da função para horário agendado
HorarioConfigurado configurar_horario(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, const char *tecla);

// Controle de Recursos
void verificar_recursos_simulado(int xicaras, int agua_por_xicara);

#endif // SENSORES_H
