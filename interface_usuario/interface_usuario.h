// interface_usuario.h
// Header file para exibição de menus, telas e interação com o usuário

#ifndef INTERFACE_USUARIO_H
#define INTERFACE_USUARIO_H

#include <stdint.h>  // Para tipos de dados padrão (uint8_t)

// Funções para Controle de Interface
void exibir_tela_inicial();           // Exibe a tela inicial com status do sistema (água, grãos, saudação)
void perguntar_quantidade_xicaras();  // Pergunta ao usuário a quantidade de xícaras
void perguntar_quando_preparar();     // Pergunta se o preparo deve ser imediato ou agendado

// Funções para Monitoramento
void exibir_temperatura_umidade_ambiente(); // Exibe temperatura e umidade do sensor DHT22
void exibir_relogio();                      // Exibe o horário atual lido do RTC

// Função de callback para processar comandos do controle IR
void callback_ir(uint16_t address, uint16_t command, int type);


#endif // INTERFACE_USUARIO_H
