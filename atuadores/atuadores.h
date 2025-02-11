// atuadores.h
// Inclui os LEDs, os servomotores, o motor de passo e o buzzer

#ifndef ATUADORES_H
#define ATUADORES_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdio.h>

// Funções para controle de LEDs e barra de LEDs
void init_leds();
void init_led_bar();
void piscar_led_bar(int vezes, int intervalo_ms);
void atualizar_led_bar(int pressao);

//Funções para controle dos servomotores
void servo_init(void);        // Inicializa o PWM para os servos
void servo1_move(uint angle); // Move o servo 1 para o ângulo especificado (0 a 180 graus)
void servo2_move(uint angle); // Move o servo 2 para o ângulo especificado (0 a 180 graus)
void servo1_movimento(void);  // Simula o ciclo de movimento para liberação de grãos
void servo2_movimento(void);  // Simula o ciclo de movimento para liberação de café moído

//Funções para controle do motor de passo
void stepper_init(void); // Inicializa os pinos do motor de passo
// Gira o motor continuamente por um tempo (em ms) no sentido especificado
void stepper_rotate(bool direction, uint32_t duration_ms, uint32_t step_delay_ms);

//Funções para controle do buzzer
// Configura o PWM para o pino especificado com frequência e duty cycle
void setup_pwm(uint pin, uint freq, float duty_cycle);
// Para o PWM no pino especificado
void stop_pwm(uint pin);
// Toca um tom no pino especificado por uma duração em milissegundos
void play_tone(uint pin, uint freq, uint duration_ms, float duty_cycle);
// Reproduz um padrão de beep com frequência, duração, pausa e repetições
void play_beep_pattern(uint pin, uint freq, uint duration_ms, uint pause_ms, int repetitions, float duty_cycle);
// Reproduz um tom de erro
void play_error_tone(uint pin);
// Reproduz um tom de sucesso (frequências crescentes)
void play_success_tone(uint pin);
// Reproduz um som para sinalizar que o café está pronto
void play_coffee_ready(uint pin);

#endif // ATUADORES_H