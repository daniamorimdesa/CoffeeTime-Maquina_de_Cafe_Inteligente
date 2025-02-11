// atuadores.c
// Inclui os LEDs, os servomotores, o motor de passo e o buzzer

#include <math.h>  // Para usar fmax
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "atuadores.h"

#define LED_VERDE 7          // LED verde: indica que o sistema está ligado
#define LED_VERMELHO 12      // LED vermelho: indica que a máquina precisa ser reabastecida
#define LED_AZUL 13          // LED azul: indica que a rotina de preparo do café está ativa
// barra de LEDs para exibir a intensidade do preparo do café
const uint LED_BAR_PINS[10] = {6, 9, 15, 22, 21, 20, 19, 18, 17, 16};
// Definição dos pinos dos servos
#define SERVO1_PIN 11 // Servo 1: Comporta de grãos
#define SERVO2_PIN 10 // Servo 2: Comporta de café moído
// Definição dos pinos do motor de passo
#define DIR_PIN 2    // Pino para controle da direção
#define STEP_PIN 3   // Pino para controle dos passos
#define BUZZER_PIN 14// Buzzer: usado para notificações sonoras

// -------------------------------------------------------------------------------------------------- //
// LEDs

// Função para inicializar LEDs
void init_leds() {
  // verde: sinaliza que a máquina está ligada
  // azul: sinaliza que o café está sendo preparado
  // vermelho: sinaliza alerta para reposição de água/grãos
  gpio_init(LED_VERDE); // inicializa o GPIO do LED
  gpio_set_dir(LED_VERDE, GPIO_OUT); // define como saída
  gpio_put(LED_VERDE, 0); // LED desligado inicialmente

  gpio_init(LED_AZUL);
  gpio_set_dir(LED_AZUL, GPIO_OUT);
  gpio_put(LED_AZUL, 0);

  gpio_init(LED_VERMELHO);
  gpio_set_dir(LED_VERMELHO, GPIO_OUT);
  gpio_put(LED_VERMELHO, 0);
}

// Função para inicializar a barra de LEDs
void init_led_bar() {
  for (int i = 0; i < 10; i++) {
    gpio_init(LED_BAR_PINS[i]);
    gpio_set_dir(LED_BAR_PINS[i], GPIO_OUT);
    gpio_put(LED_BAR_PINS[i], 0); // Apaga todos os LEDs no início
  }
}

// Função para piscar barra de LEDs no fim do preparo do café
void piscar_led_bar(int vezes, int intervalo_ms) {
  for (int i = 0; i < vezes; i++) {
    // Liga todos os LEDs
    for (int j = 0; j < 10; j++) {
      gpio_put(LED_BAR_PINS[j], 1);
    }
    sleep_ms(intervalo_ms);

    // Desliga todos os LEDs
    for (int j = 0; j < 10; j++) {
      gpio_put(LED_BAR_PINS[j], 0);
    }
    sleep_ms(intervalo_ms);
  }
}

// Função para ligar barra de LEDs conforme intensidade selecionada
void atualizar_led_bar(int pressao) {
  int num_leds = (int)fmax(1, (pressao * 10) / 100); // Converte para int e pelo menos 1 LED acende

  for (int i = 0; i < 10; i++) {
    gpio_put(LED_BAR_PINS[i], (i < num_leds) ? 1 : 0);
    sleep_ms(200); // Efeito de preenchimento progressivo
  }
}

// -------------------------------------------------------------------------------------------------- //
// Servomotores

// Inicializa o PWM para os servos
void servo_init(void)
{
  // Configuração do servo 1
  gpio_set_function(SERVO1_PIN, GPIO_FUNC_PWM);
  uint slice1 = pwm_gpio_to_slice_num(SERVO1_PIN);
  pwm_set_clkdiv(slice1, 64.0f); // Ajusta o divisor de clock (50 Hz)
  pwm_set_wrap(slice1, 20000);   // Configura o período do PWM para 20ms
  pwm_set_gpio_level(SERVO1_PIN, 0);
  pwm_set_enabled(slice1, true);

  // Configuração do servo 2
  gpio_set_function(SERVO2_PIN, GPIO_FUNC_PWM);
  uint slice2 = pwm_gpio_to_slice_num(SERVO2_PIN);
  pwm_set_clkdiv(slice2, 64.0f); // Ajusta o divisor de clock (50 Hz)
  pwm_set_wrap(slice2, 20000);   // Configura o período do PWM para 20ms
  pwm_set_gpio_level(SERVO2_PIN, 0);
  pwm_set_enabled(slice2, true);
}

// Move o servo 1 para o ângulo especificado (0 a 180 graus)
void servo1_move(uint angle)
{
  if (angle > 180) angle = 180;
  uint pulse_width = 870 + (angle * 2000 / 180);
  pwm_set_gpio_level(SERVO1_PIN, pulse_width);
}

// Move o servo 2 para o ângulo especificado (0 a 180 graus)
void servo2_move(uint angle)
{
  if (angle > 180) angle = 180;
  uint pulse_width = 870 + (angle * 2000 / 180);
  pwm_set_gpio_level(SERVO2_PIN, pulse_width);
}

// Simula o ciclo de movimento para liberação de grãos
void servo1_movimento(void)
{
  servo1_move(0);
  servo2_move(0);
  sleep_ms(500);

  //printf("Abrindo comporta de grãos...\n");
  servo1_move(90);
  sleep_ms(1000);
  servo1_move(180);
  sleep_ms(1000);
  servo1_move(0);
  sleep_ms(100);
}

// Simula o ciclo de movimento para liberação de café moído
void servo2_movimento(void)
{
 // printf("Abrindo comporta de café moído...\n");
  servo2_move(90);
  sleep_ms(1000);
  servo2_move(180);
  sleep_ms(1000);
  servo2_move(0);
  sleep_ms(100);
}

// -------------------------------------------------------------------------------------------------- //
// Motor de passo 

// Inicializa os pinos do motor de passo
void stepper_init(void) 
{
    gpio_init(STEP_PIN);
    gpio_set_dir(STEP_PIN, GPIO_OUT);
    gpio_put(STEP_PIN, 0);

    gpio_init(DIR_PIN);
    gpio_set_dir(DIR_PIN, GPIO_OUT);
    gpio_put(DIR_PIN, 0);
}

// Gira o motor continuamente por um tempo (em ms) no sentido especificado
void stepper_rotate(bool direction, uint32_t duration_ms, uint32_t step_delay_ms) 
{
    gpio_put(DIR_PIN, direction); // Define a direção
    uint32_t steps = duration_ms / step_delay_ms; // Calcula o número de passos
    for (uint32_t i = 0; i < steps; i++) 
    {
        gpio_put(STEP_PIN, 1); // Pulso alto
        sleep_ms(step_delay_ms / 2); // Meio ciclo
        gpio_put(STEP_PIN, 0); // Pulso baixo
        sleep_ms(step_delay_ms / 2); // Meio ciclo
    }
}

// -------------------------------------------------------------------------------------------------- //
//Buzzer

void setup_pwm(uint pin, uint freq, float duty_cycle) {
  gpio_set_function(pin, GPIO_FUNC_PWM); // Configura o pino para PWM

  uint slice_num = pwm_gpio_to_slice_num(pin);
  uint channel = pwm_gpio_to_channel(pin);

  uint32_t clock = 125000000; // Frequência do clock do Pico (125 MHz)
  uint32_t divider16 = clock / freq / 4096 + (clock % (freq * 4096) != 0);
  pwm_set_clkdiv(slice_num, divider16 / 16.0f);

  // Configura duty cycle
  uint32_t level = (uint32_t)(4095 * duty_cycle);
  pwm_set_wrap(slice_num, 4095);
  pwm_set_chan_level(slice_num, channel, level);

  pwm_set_enabled(slice_num, true); // Ativa o PWM
}

void stop_pwm(uint pin) {
  uint slice_num = pwm_gpio_to_slice_num(pin);
  uint channel = pwm_gpio_to_channel(pin);

  pwm_set_chan_level(slice_num, channel, 0); // Desativa o PWM
  pwm_set_enabled(slice_num, false);        // Desativa o slice
}

void play_tone(uint pin, uint freq, uint duration_ms, float duty_cycle) {
  setup_pwm(pin, freq, duty_cycle);
  sleep_ms(duration_ms);
  stop_pwm(pin);
}

void play_error_tone(uint pin) {
  for (int i = 0; i < 3; i++) {
    play_tone(pin, 3000, 200, 0.5); // Tom de erro com frequência de 3kHz e duração de 200ms
    sleep_ms(200);                 // Pausa entre os tons
  }
}

void play_beep_pattern(uint pin, uint freq, uint duration_ms, uint pause_ms, int repetitions, float duty_cycle) {
  for (int i = 0; i < repetitions; i++) {
    play_tone(pin, freq, duration_ms, duty_cycle);
    sleep_ms(pause_ms);
  }
}

void play_success_tone(uint pin) {
  play_tone(pin, 1000, 500, 0.5); // Tom baixo de sucesso
  sleep_ms(100);
  play_tone(pin, 2000, 500, 0.5); // Tom alto de sucesso
}

void play_coffee_ready(uint pin) {
  play_tone(pin, 262, 200, 0.5); //pino buzzer, nota da escala, duração da nota em ms, duty cycle
  sleep_ms(100); // pausa entre notas
  play_tone(pin, 294, 200, 0.5);
  sleep_ms(100);
  play_tone(pin, 330, 200, 0.5);
  sleep_ms(100);
  play_tone(pin, 349, 200, 0.5);
  sleep_ms(100);
  play_tone(pin, 392, 400, 0.5);
  sleep_ms(100);
}


