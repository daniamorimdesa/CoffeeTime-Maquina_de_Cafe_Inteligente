// processos_internos.c

#include "processos_internos.h"
#include "sensores.h"
#include "atuadores.h"
#include "lcd_i2c.h"
#include "interface_usuario.h"  
#include "estado.h"            
#include <stdio.h>
#include "pico/stdlib.h"

#define DHT_PIN 8      // DHT22 para monitorar temperatura/umidade ambiente
#define BUZZER_PIN 14  // Buzzer para notificações sonoras
#define LED_AZUL 13          // LED azul: indica que a rotina de preparo do café está ativa

extern float agua_ml;
extern float graos_g;
extern bool play_apertado;
extern Estado estado_atual;

void setup_machine() {
  stdio_init_all();
  init_leds();
  init_led_bar();
  init_i2c_lcd();
  servo_init();
  stepper_init();
  gpio_init(DHT_PIN);
  init_adc();
  play_success_tone(BUZZER_PIN);

  printf("INSTRUÇÕES DE USO DA MÁQUINA DE CAFÉ\n");
  printf("=====================================================================================\n");
  printf(">> Ajuste a bebida conforme desejado: intensidade, temperatura e quantidade de água.\n");
  printf(">> Use o controle IR para navegar. Aperte PLAY para iniciar.\n");
  printf(">> Use o sensor DHT22 para monitorar temperatura/umidade ambiente.\n");
  printf(">> Caso agende o preparo, a máquina aguardará o horário marcado.\n");
  printf(">> Durante o preparo, a barra de LEDs indica a força do café.\n");
  printf(">> A tela inicial atualiza os valores conforme o uso.\n");
}

// Função para determinar a temperatura da bebida
void simular_aquecimento_automatico(float temp_desejada) {
  float temperatura_atual = 25.0;
  lcd_clear();
  lcd_set_cursor(1, 2);
  lcd_print("HEATING WATER...");

  while (temperatura_atual <= temp_desejada) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "TEMP: %.1f C", temperatura_atual);
    lcd_set_cursor(2, 4);
    lcd_print(buffer);
    temperatura_atual += 2.5;
    sleep_ms(400);
  }

  lcd_clear();
  type_effect("   WATER READY!", 1, 50);
  sleep_ms(500);
}

// Função para determinar a intensidade da bebida com base na pressão
const char* determinar_intensidade(int pressao) {
  if (pressao <= 33) return "MILD";
  else if (pressao <= 66) return "MEDIUM";
  else return "STRONG";
}

// Função para determinar a temperatura da bebida
const char* determinar_nivel_temperatura(float temperatura) {
  if (temperatura < 90) return "WARM";
  else if (temperatura < 94) return "HOT";
  else return "HOT++";
}

// Função principal para simular o preparo do café:
// Função principal para simular o preparo do café:
// 1. Verifica recursos
// 2. Acende a barra de LEDs conforme a força do café
// 3. Simula o aquecimento da água conforme ajuste do usuário
// 4. Movimenta os servomotores e o motor de passo
// 5. Finaliza o preparo e atualiza os recursos
void preparar_cafe(int xicaras) {
  int pressao = ler_intensidade(); // Intensidade do café (pressão da extração)
  float temperatura_desejada = ler_temperatura_desejada(); // Temperatura da bebida
  int agua_por_xicara = ler_quantidade_agua(); // Quantidade de água por xícara
  const char* intensidade = determinar_intensidade(pressao); // intensidade do café
  const char* nivel_temperatura = determinar_nivel_temperatura(temperatura_desejada); //temperatura do café

  verificar_recursos_simulado(xicaras, agua_por_xicara); // Verifica com a rotina simulada

  gpio_put(LED_AZUL, 1); // Acende o LED azul para indicar preparo
  play_tone(BUZZER_PIN, 500, 600, 0.8);  // Som início do preparo
  sleep_ms(1000);

  lcd_clear();
  lcd_set_cursor(1, 0);
  lcd_print("STARTING PROCESS ..."); //máquina iniciando o preparo
  for (int i = 0; i <= 80; i += 10) {
    progress_bar(i, 2);
    sleep_ms(300);
  }

  // Atualiza a barra de LEDs com a intensidade do café
  atualizar_led_bar(pressao);

  // Ajusta o aquecimento conforme escolha do usuário
  simular_aquecimento_automatico(temperatura_desejada);
  // Ajusta a quantidade total de água
  int agua_total = xicaras * agua_por_xicara;

  // Movimento do primeiro servo (grãos liberados para a moagem)
  lcd_clear();
  lcd_set_cursor(1, 1);
  lcd_print("RELEASING BEANS...");
  servo1_movimento();

  // Movimento do motor de passo (moagem dos grãos)
  lcd_clear();
  lcd_set_cursor(1, 4);
  lcd_print("GRINDING ...");
  stepper_rotate(true, 5000, 5);
  sleep_ms(500);

  // Início da extração do café
  // Tempo de brewing ajustado pela pressão (quanto maior a pressão, menor o tempo)
  int tempo_brewing = 5000 - (pressao * 20); // Tempo base reduzido pela pressão
  lcd_clear();

  char buffer_temperatura[21]; // nível de temperatura escolhida
  snprintf(buffer_temperatura, sizeof(buffer_temperatura), "BREWING COFFEE:%s", nivel_temperatura);
  lcd_set_cursor(0, 0);
  lcd_print(buffer_temperatura);

  char buffer_agua[21]; // quantidade preparada
  if (xicaras == 1) {
    snprintf(buffer_agua, sizeof(buffer_agua), "1 CUP OF %d ML", agua_por_xicara);
  } else {
    snprintf(buffer_agua, sizeof(buffer_agua), "%d CUPS OF %d ML", xicaras, agua_por_xicara);
  }
  lcd_set_cursor(2, 0);
  lcd_print(buffer_agua);

  char buffer_intensidade[21]; // nível de intensidade do café
  snprintf(buffer_intensidade, sizeof(buffer_intensidade), "INTENSITY: %s", intensidade);
  lcd_set_cursor(3, 0);
  lcd_print(buffer_intensidade);

  servo2_move(45);
  sleep_ms(tempo_brewing); // Simula o tempo de brewing proporcional à pressão da água

  // Atualiza os níveis de água e grãos de café
  agua_ml -= agua_total;
  graos_g -= xicaras * 10;

  // Mensagem final no display
  servo2_movimento();
  lcd_clear();
  fade_text("  COFFEE IS READY!", "      GRAB IT!", 1, 1000);
  play_coffee_ready(BUZZER_PIN);      // toca som para indicar que o café está pronto para retirar
  // Efeito especial: Piscada na barra de LEDs e desligamento em seguida
  piscar_led_bar(3, 300);
  gpio_put(LED_AZUL, 0); // Desliga o LED azul pois finalizou
  sleep_ms(2000);


  exibir_tela_inicial();
  estado_atual = ESTADO_TELA_INICIAL; // Volta à tela inicial
}

