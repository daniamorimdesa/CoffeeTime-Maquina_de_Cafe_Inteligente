// estado.h
#ifndef ESTADO_H
#define ESTADO_H

#include <stdint.h>
#include <stdbool.h>

// Estados da máquina de café
typedef enum {
  ESTADO_TELA_INICIAL,         // exibe a saudação inicial, monitoramento de ambiente, nível de recursos e relógio com horário atual
  ESTADO_QUANTIDADE_XICARAS,   // permite o usuário selecionar quantas xícaras deseja preparar
  ESTADO_QUANDO_PREPARAR,      // usuário define horário de preparo imediato ou agendado
  ESTADO_PREPARANDO,           // sistema inicia a rotina de preparo verificando recursos e seguindo para extração do café
  ESTADO_PROGRAMANDO,          // usuário define horário agendado para ínicio do preparo
  ESTADO_AGUARDANDO            // sistema aguarda o horário atual coincidir com o horário agendado de preparo
} Estado;

// Função para gerenciar os estados
void gerenciar_estado(void);

#endif // ESTADO_H