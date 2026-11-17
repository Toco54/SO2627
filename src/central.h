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

#define DEF_MAXESTAFETAS 10
#define DEF_PRAZOACEITA 30
#define MAX_PEDIDOS_ESPERA 30
#define LOCK_FILE "/tmp/central_so2627.lock"


typedef struct {
    int recebidos;
    int atribuidos;
    int concluidos;
    int expirados;
    int perdidos;
    float valor_total;
} Estatisticas;

typedef struct {
    int max_estafetas;
    int prazo_aceita;
    char bin_dir[256];
    char central_pipe[256];
    Estatisticas stats;
} CentralState;

typedef struct {
    char tipo[12];     
    char corpo[50];    
} Mensagem;


void verifica_instancia_unica();
void inicializa_estado(CentralState *estado);

#endif