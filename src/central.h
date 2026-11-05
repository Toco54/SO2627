#ifndef CENTRAL_H
#define CENTRAL_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

// Valores por omissão e limites (Secção 7.2 do enunciado)
#define DEF_MAXESTAFETAS 10
#define DEF_PRAZOACEITA 30
#define MAX_PEDIDOS_ESPERA 30
#define LOCK_FILE "/tmp/central_so2627.lock"

// Estrutura para as estatísticas (comando 'stats')
typedef struct {
    int recebidos;
    int atribuidos;
    int concluidos;
    int expirados;
    int perdidos;
    float valor_total;
} Estatisticas;

// Estrutura principal com o estado e configuração da Central
typedef struct {
    int max_estafetas;
    int prazo_aceita;
    char bin_dir[256];
    char central_pipe[256];
    Estatisticas stats;
} CentralState;

// Estrutura obrigatória para as mensagens trocadas no sistema (Conforme enunciado)
typedef struct {
    char tipo[12];     /* Identificador do tipo de mensagem (ex: "pedido", "RegistoEst")[cite: 256, 266] */
    char corpo[50];    /* Dados da mensagem[cite: 255, 265] */
} Mensagem;

// Protótipos das funções da central
void inicializa_estado();
void verifica_instancia_unica();

#endif