#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h> // Necessário para detetar o erro EEXIST

// Valores por omissão (conforme a secção 7.2 do enunciado)
#define DEF_MAXESTAFETAS 10
#define DEF_PRAZOACEITA 30
#define MAX_PEDIDOS_ESPERA 30
#define LOCK_FILE "/tmp/central_so2627.lock"

// Estrutura para guardar as estatísticas (comando 'stats')
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

// Variável global para ser mais fácil aceder ao estado a partir de funções auxiliares ou sinais
CentralState estado_central;



void inicializa_estado() {
    // 1. Inicializar estatísticas a zero
    memset(&estado_central.stats, 0, sizeof(Estatisticas));

    // 2. Tentar ler variáveis de ambiente
    char* env_max = getenv("MAXESTAFETAS");
    char* env_prazo = getenv("PRAZOACEITA");
    char* env_bindir = getenv("BINDIR");
    char* env_pipe = getenv("CENTRAL_PIPE");

    // 3. Atribuir valores lidos ou os valores por omissão
    estado_central.max_estafetas = (env_max) ? atoi(env_max) : DEF_MAXESTAFETAS;
    estado_central.prazo_aceita = (env_prazo) ? atoi(env_prazo) : DEF_PRAZOACEITA;

    if (env_bindir) {
        strncpy(estado_central.bin_dir, env_bindir, sizeof(estado_central.bin_dir) - 1);
    }
    else {
        strcpy(estado_central.bin_dir, "."); // Diretoria atual por omissão
    }

    if (env_pipe) {
        strncpy(estado_central.central_pipe, env_pipe, sizeof(estado_central.central_pipe) - 1);
    }
    else {
        fprintf(stderr, "ERRO: Variavel de ambiente CENTRAL_PIPE e obrigatoria!\n");
        exit(EXIT_FAILURE);
    }
}


void verifica_instancia_unica() {
    // Tenta criar o ficheiro de lock em modo exclusivo
    int fd = open(LOCK_FILE, O_CREAT | O_EXCL | O_RDWR, 0666);
    
    if (fd < 0) {
        if (errno == EEXIST) {
                perror("oi");
            fprintf(stderr, "ERRO: Ja existe uma instancia da Central em execucao!\n");
            exit(EXIT_FAILURE);
        } else {
            perror("ERRO ao criar ficheiro de lock");
            exit(EXIT_FAILURE);
        }
    }
    
    // Se chegou aqui, o ficheiro foi criado com sucesso (somos a primeira instância)
    close(fd); 
}


int main(int argc, char* argv[]) {
    // Carrega o que está nas variáveis de ambiente primeiro
    inicializa_estado();

    // Lê os argumentos passados na linha de comandos (que têm prioridade)
    int opt;
    while ((opt = getopt(argc, argv, "m:p:")) != -1) {
        switch (opt) {
        case 'm':
            estado_central.max_estafetas = atoi(optarg);
            break;
        case 'p':
            estado_central.prazo_aceita = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Uso: %s [-m max_estafetas] [-p prazo_aceita]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Verifica se já há outra Central a correr (Adicionado aqui!)
    verifica_instancia_unica();

    // Apenas para debug: testar se leu bem
    printf("--- CONFIGURACOES DA CENTRAL ---\n");
    printf("Max Estafetas: %d\n", estado_central.max_estafetas);
    printf("Prazo Aceitacao: %d seg\n", estado_central.prazo_aceita);
    printf("Diretoria Bin: %s\n", estado_central.bin_dir);
    printf("Pipe Central: %s\n", estado_central.central_pipe);
    printf("--------------------------------\n");

    // Remove o ficheiro de lock antes de sair, para não trancar execuções futuras.
    // (Mais à frente, isto vai passar para a função de encerramento real)
    unlink(LOCK_FILE);

    return 0;
}