/////////////////////////////////////////////////////////////////////////////////////////////////////////
// COFFEE TIME - Máquina de café inteligente utilizando a Raspberry Pi Pico W
// Elaborado por: Daniela Amorim de Sá
////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include <ctype.h>
// Bibliotecas desenvolvidas para modular o projeto
#include "lcd_i2c.h"
#include "controle_ir.h"
#include "sensores.h"
#include "atuadores.h"
#include "interface_usuario.h"
#include "processos_internos.h"
#include "estado.h"

#define IR_SENSOR_GPIO_PIN 1 // controle remoto IR para o usuário enviar comandos para a máquina

int main() {

  setup_machine();
  init_ir_irq_receiver(IR_SENSOR_GPIO_PIN, &callback_ir);

  while (true) {
    gerenciar_estado();  // Delegação do controle para o estado atual
    sleep_ms(200);
  }
  return 0;
}