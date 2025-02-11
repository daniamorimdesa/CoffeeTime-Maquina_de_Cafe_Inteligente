// lcd_i2c.c

#include "lcd_i2c.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>

// comunicação i2c para o display LCD e o RTC
#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5
static i2c_inst_t *i2c_instance;

static void lcd_send_command(uint8_t cmd) {
  uint8_t upper = cmd & 0xF0;
  uint8_t lower = (cmd << 4) & 0xF0;

  uint8_t data[4] = {
    upper | 0x0C, // Envia habilitação
    upper,        // Desativa habilitação
    lower | 0x0C, // Envia habilitação
    lower         // Desativa habilitação
  };

  i2c_write_blocking(i2c_instance, LCD_ADDR, data, sizeof(data), false);
}

// escrita de caractere no LCD
void lcd_send_char(char c) {
  uint8_t upper = c & 0xF0;
  uint8_t lower = (c << 4) & 0xF0;

  uint8_t data[4] = {
    upper | 0x0D, // Envia habilitação
    upper,        // Desativa habilitação
    lower | 0x0D, // Envia habilitação
    lower         // Desativa habilitação
  };

  i2c_write_blocking(i2c_instance, LCD_ADDR, data, sizeof(data), false);
}

void lcd_init(i2c_inst_t *i2c) {
  i2c_instance = i2c;

  sleep_ms(50); // Aguarda inicialização do LCD
  lcd_send_command(0x03);
  sleep_ms(5);
  lcd_send_command(0x03);
  sleep_us(150);
  lcd_send_command(0x03);
  lcd_send_command(0x02);

  // Configuração do LCD
  lcd_send_command(0x28); // Modo 4-bits, 2 linhas
  lcd_send_command(0x08); // Desliga display
  lcd_send_command(0x01); // Limpa display
  sleep_ms(2);
  lcd_send_command(0x06); // Incrementa cursor
  lcd_send_command(0x0C); // Liga display e cursor
}

// Limpa o display
void lcd_clear() {
  lcd_send_command(0x01); // Comando para limpar
  sleep_ms(2);
}


void init_i2c_lcd() {
  i2c_init(I2C_PORT, 100 * 1000); // configura o barramento I2C para 100 kHz
  // Define os pinos SDA e SCL
  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
  // habilita resistores de pull up para comunicação estável
  gpio_pull_up(SDA_PIN);
  gpio_pull_up(SCL_PIN);

  lcd_init(I2C_PORT); // inicializa o LCD
  lcd_clear();       // limpa a tela
}

// Configura a linha e a coluna do começo da escrita do buffer
void lcd_set_cursor(int row, int col) {
  const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  lcd_send_command(0x80 | (col + row_offsets[row]));
}


void lcd_print(const char *str) {
  while (*str) {
    lcd_send_char(*str++);
  }
}

void create_custom_char(int location, uint8_t charmap[]) {
  location &= 0x7; // O LCD suporta 8 caracteres (0-7)
  lcd_send_command(0x40 | (location << 3));
  for (int i = 0; i < 8; i++) {
    lcd_send_char(charmap[i]);
  }
}

void display_custom_char(int location, int row, int col) {
  lcd_set_cursor(row, col);
  lcd_send_char(location);
}

// Funções de animação

//animação de texto deslizando
void scroll_text(const char *message, int row, int delay_ms) {
  int len = strlen(message);
  char buffer[LCD_COLS + 1] = {0};

  for (int start = 0; start < len; start++) {
    strncpy(buffer, message + start, LCD_COLS);
    buffer[LCD_COLS] = '\0';

    lcd_set_cursor(row, 0);
    lcd_print(buffer);
    sleep_ms(delay_ms);

    if (start + LCD_COLS >= len) {
      start = -1; // Reinicia o ciclo
    }
  }
}
//exemplo de uso na main:
//scroll_text("Bem-vindo ao Raspberry Pi Pico! ", 0, 200);

//animação de texto digitando
void type_effect(const char *message, int row, int delay_ms) {
  lcd_set_cursor(row, 0);
  for (int i = 0; i < strlen(message); i++) {
    lcd_print((char[]) {
      message[i], '\0'
    }); // Envia um caractere por vez
    sleep_ms(delay_ms);
  }
}
//exemplo de uso na main:
//type_effect("Hello, World!", 0, 100);

//animação de barra de progresso
void progress_bar(int percentage, int row) {
  int filled = (percentage * LCD_COLS) / 100;
  lcd_set_cursor(row, 0);
  for (int i = 0; i < LCD_COLS; i++) {
    if (i < filled) {
      lcd_print("_");
    } else {
      lcd_print(" ");
    }
  }
}
//exemplo de uso na main:
//for (int i = 0; i <= 100; i += 10) {
// progress_bar(i, 1);
//  sleep_ms(500);
//}

//animação de texto piscando/alerta
void blink_text(const char *message, int row, int col, int times, int delay_ms) {
  for (int i = 0; i < times; i++) {
    lcd_set_cursor(row, col);
    lcd_print(message);
    sleep_ms(delay_ms);

    lcd_set_cursor(row, col);
    for (int j = 0; j < strlen(message); j++) {
      lcd_print(" "); // Apaga o texto
    }
    sleep_ms(delay_ms);
  }

  // Restaura o texto
  lcd_set_cursor(row, col);
  lcd_print(message);
}
//exemplo de uso na main:
//blink_text("ALERTA!", 0, 5, 5, 500);

//animação Efeito de Apagar e Escrever
void fade_text(const char *message1, const char *message2, int row, int delay_ms) {
  lcd_set_cursor(row, 0);
  lcd_print(message1);
  sleep_ms(delay_ms);

  for (int i = strlen(message1); i >= 0; i--) {
    lcd_set_cursor(row, i);
    lcd_print(" ");
    sleep_ms(50);
  }

  lcd_set_cursor(row, 0);
  lcd_print(message2);
}
//exemplo de uso na main:
//fade_text("Bem-vindo!", "Aprendendo C!", 0, 1000);

//animação relógio simples
void simple_clock() {
  for (int seconds = 0; seconds < 1000; seconds++) {
    char time[20];
    snprintf(time, sizeof(time), "Tempo: %03d seg", seconds);
    lcd_set_cursor(0, 0);
    lcd_print(time);
    sleep_ms(1000);
  }
}
//exemplo de uso na main:
//simple_clock();

