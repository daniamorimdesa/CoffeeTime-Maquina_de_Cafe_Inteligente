// processos_internos.h
// Configuração inicial e operações internas da máquina de café

#ifndef PROCESSOS_INTERNOS_H
#define PROCESSOS_INTERNOS_H

void setup_machine();                     // Configura a máquina ao iniciar
void preparar_cafe(int xicaras);           // Simula o preparo do café
void simular_aquecimento_automatico(float temp_desejada); // Simula o aquecimento da água
const char* determinar_intensidade(int pressao);          // Determina a intensidade do café
const char* determinar_nivel_temperatura(float temperatura); // Determina a temperatura do café

#endif // PROCESSOS_INTERNOS_H
