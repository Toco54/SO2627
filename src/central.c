#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h> // Necessário para detetar o erro EEXIST
#include <signal.h>
#include "central.h"

// Variável global para ser mais fácil aceder ao estado a partir de funções auxiliares ou sinais
// CentralState estado_central;
static pid_t pid_tarifador = -1;
static int pipe_c2t[2] = {-1, -1};
static int pipe_t2c[2] = {-1, -1};


void inicializa_estado(CentralState *estado) {
    // 1. Inicializar estatísticas a zero
    memset(&estado->stats, 0, sizeof(Estatisticas));

    // 2. Tentar ler variáveis de ambiente
    char* env_max = getenv("MAXESTAFETAS");
    char* env_prazo = getenv("PRAZOACEITA");
    char* env_bindir = getenv("BINDIR");
    char* env_pipe = getenv("CENTRAL_PIPE");

    // 3. Atribuir valores lidos ou os valores por omissão
    estado->max_estafetas = (env_max) ? atoi(env_max) : DEF_MAXESTAFETAS;
    estado->prazo_aceita = (env_prazo) ? atoi(env_prazo) : DEF_PRAZOACEITA;

    if (env_bindir) {
        strncpy(estado->bin_dir, env_bindir, sizeof(estado->bin_dir) - 1);
        estado->bin_dir[sizeof(estado->bin_dir) - 1] = '\0';
    }
    else {
        strcpy(estado->bin_dir, "."); // Diretoria atual por omissão
    }

    if (env_pipe) {
        strncpy(estado->central_pipe, env_pipe, sizeof(estado->central_pipe) - 1);
        estado->central_pipe[sizeof(estado->central_pipe) - 1] = '\0';
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

void encerra_central() {
    printf("\nA encerrar a Central ordeiramente...\n");

    close(pipe_c2t[1]);
    close(pipe_t2c[0]);

    // 2. Esperar que o processo do tarifador termine (evita que fique um processo zombie)
    if (pid_tarifador > 0) {
        printf("A aguardar terminacao do tarifador (PID %d)...\n", pid_tarifador);
        waitpid(pid_tarifador, NULL, 0);
    }

    // 3. Apagar o ficheiro de lock para libertar a instância única
    unlink(LOCK_FILE);
    
    printf("Recursos libertados com sucesso. Obrigado!\n");
    exit(EXIT_SUCCESS);
}

// Função que é chamada automaticamente se o utilizador fizer Ctrl+C
void trata_sigint(int signum) {
    (void)signum;
    encerra_central();
}


int main(int argc, char* argv[]) {
    // Carrega o que está nas variáveis de ambiente primeiro
    CentralState estado_central;
    inicializa_estado(&estado_central);
    signal(SIGINT, trata_sigint);

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

    if (estado_central.max_estafetas <= 0 || estado_central.max_estafetas > 20) {
        fprintf(stderr, "ERRO: MAXESTAFETAS deve estar entre 1 e 20.\n");
        exit(EXIT_FAILURE);
    }
    if (estado_central.prazo_aceita <= 0) {
        fprintf(stderr, "ERRO: PRAZOACEITA deve ser superior a 0.\n");
        exit(EXIT_FAILURE);
    }
    
    // Verifica se já há outra Central a correr (Adicionado aqui!)
    verifica_instancia_unica();

    // ==========================================
    // TASK 2: INICIALIZAÇÃO DE PIPES ANÓNIMOS
    // ==========================================
    
    // int pipe_c2t[2]; // Pipe Central -> Tarifador
    // int pipe_t2c[2]; // Pipe Tarifador -> Central

    // Criação do primeiro pipe
    if (pipe(pipe_c2t) == -1) {
        perror("ERRO: Falha ao criar pipe Central->Tarifador");
        unlink(LOCK_FILE); // Limpa o lock antes de abortar
        exit(EXIT_FAILURE);
    }

    // Criação do segundo pipe
    if (pipe(pipe_t2c) == -1) {
        perror("ERRO: Falha ao criar pipe Tarifador->Central");
        unlink(LOCK_FILE);
        exit(EXIT_FAILURE);
    }
    
    printf("DEBUG: Pipes criados com sucesso.\n"); // Podes remover isto mais tarde

    // ==========================================


    // ==========================================
    // TASK 3: CRIAÇÃO DO PROCESSO-FILHO (TARIFADOR)
    // ==========================================
    
    pid_tarifador = fork();

    if (pid_tarifador < 0) {
        perror("ERRO: Falha ao executar o fork");
        unlink(LOCK_FILE);
        exit(EXIT_FAILURE);
    }

    if (pid_tarifador == 0) {
        // --- CÓDIGO DO PROCESSO-FILHO (TARIFADOR) ---
        
        // 1. Fechar as pontas dos tubos que o filho não vai usar
        close(pipe_c2t[1]); // O filho não escreve no tubo de ida
        close(pipe_t2c[0]); // O filho não lê do tubo de volta

        // 2. Redirecionar stdin (0) e stdout (1) para os tubos
        dup2(pipe_c2t[0], STDIN_FILENO);   // O stdin passa a ler do pipe_c2t
        dup2(pipe_t2c[1], STDOUT_FILENO);  // O stdout passa a escrever no pipe_t2c

        // 3. Fechar os descritores originais que já foram duplicados
        close(pipe_c2t[0]);
        close(pipe_t2c[1]);

        // 4. Construir o caminho para o executável e lançar com exec()
        char caminho_tarifador[512];
        snprintf(caminho_tarifador, sizeof(caminho_tarifador), "%s/tarifador", estado_central.bin_dir);
        
        execl(caminho_tarifador, "tarifador", NULL);
        
        // Se o código chegar aqui, significa que o execl() falhou
        perror("ERRO: Falha ao iniciar o executavel do tarifador (verifique a BINDIR)");
        exit(EXIT_FAILURE);
    }

    // --- CÓDIGO DO PROCESSO-PAI (CENTRAL) ---
    // Fechar as pontas que a Central não vai usar
    close(pipe_c2t[0]); // A Central não lê do tubo que envia
    close(pipe_t2c[1]); // A Central não escreve no tubo que recebe

    printf("DEBUG: Tarifador arrancou com PID %d.\n", pid_tarifador);
    // ==========================================


    // Apenas para debug: testar se leu bem
    printf("--- CONFIGURACOES DA CENTRAL ---\n");
    printf("Max Estafetas: %d\n", estado_central.max_estafetas);
    printf("Prazo Aceitacao: %d seg\n", estado_central.prazo_aceita);
    printf("Diretoria Bin: %s\n", estado_central.bin_dir);
    printf("Pipe Central: %s\n", estado_central.central_pipe);
    printf("--------------------------------\n");


// ==========================================
    // TASK 4: CONSOLA DE ADMINISTRAÇÃO
    // ==========================================
    char linha[256];
    char *comando;
    
    printf("\n--- Consola de Administracao Pronta ---\n");
    
while (1) {
        printf("Central > ");
        fflush(stdout); // Força o prompt a aparecer imediatamente
        
        // Lê a linha do terminal
        if (fgets(linha, sizeof(linha), stdin) == NULL) {
            encerra_central(); // Sai em caso de erro ou EOF (Ctrl+D)
        }
        
        // Remove o parágrafo '\n' que o fgets apanha no final
        linha[strcspn(linha, "\n")] = 0;
        
        // Extrai a primeira palavra (o comando real)
        comando = strtok(linha, " ");
        
        if (comando == NULL) {
            continue; // Se o utilizador só der Enter, volta ao início do ciclo
        }
        
        // Verifica qual foi o comando introduzido
        if (strcmp(comando, "encerra") == 0) {
            if (strtok(NULL, " ") != NULL) {
                printf("ERRO: O comando 'encerra' nao aceita parametros.\n");
            } else {
                encerra_central();
            }
        } 
        else if (strcmp(comando, "stats") == 0) {
            if (strtok(NULL, " ") != NULL) {
                printf("ERRO: O comando 'stats' nao aceita parametros.\n");
            } else {
                printf("\n--- ESTATISTICAS DA CENTRAL ---\n");
                printf("Pedidos recebidos:  %d\n", estado_central.stats.recebidos);
                printf("Pedidos atribuidos: %d\n", estado_central.stats.atribuidos);
                printf("Pedidos concluidos: %d\n", estado_central.stats.concluidos);
                printf("Pedidos expirados:  %d\n", estado_central.stats.expirados);
                printf("Pedidos perdidos:   %d\n", estado_central.stats.perdidos);
                printf("Valor total:        %.2f euros\n", estado_central.stats.valor_total);
                printf("-------------------------------\n");
            }
        }
        else if (strcmp(comando, "pedido") == 0) {
            // Extrair os argumentos seguintes usando strtok
            char *origem = strtok(NULL, " ");
            char *destino = strtok(NULL, " ");
            char *peso_str = strtok(NULL, " ");
            char *extra = strtok(NULL, " ");

            // Validar se o utilizador escreveu exatamente os 3 argumentos
            if (origem == NULL || destino == NULL || peso_str == NULL || extra != NULL) {
                printf("ERRO: Formato incorreto. Uso: pedido <origem> <destino> <peso>\n");
            } else {
                char *endptr;
                float peso = strtof(peso_str, &endptr);

                if (*endptr != '\0' || peso <= 0.0f) {
                    printf("ERRO: Peso invalido. Deve ser um valor numerico positivo.\n");
                } else if (strlen(origem) > 15 || strlen(destino) > 15) {
                    printf("ERRO: Origem e destino nao podem exceder 15 caracteres.\n");
                } else {
                    char mensagem_tarifador[256];
                    snprintf(mensagem_tarifador, sizeof(mensagem_tarifador), "%s %s %s\n", origem, destino, peso_str);

                    // Enviar para o pipe do tarifador (Central -> Tarifador)
                    if (write(pipe_c2t[1], mensagem_tarifador, strlen(mensagem_tarifador)) == -1) {
                        perror("ERRO ao escrever para o tarifador");
                    } else {
                        // Ler a resposta do tarifador (Tarifador -> Central)
                        char resposta_tarifador[256];
                        ssize_t bytes_lidos = read(pipe_t2c[0], resposta_tarifador, sizeof(resposta_tarifador) - 1);

                        if (bytes_lidos > 0) {
                            resposta_tarifador[bytes_lidos] = '\0';
                            resposta_tarifador[strcspn(resposta_tarifador, "\n")] = 0;

                            printf("PEDIDO ACEITE/TARIFADO -> Resposta do Tarifador: %s\n", resposta_tarifador);
                            estado_central.stats.recebidos++;
                        } else {
                            printf("ERRO ao ler a resposta do tarifador\n");
                        }
                    }
                }
            }
        }
        else {
            printf("ERRO: Comando desconhecido '%s'\n", comando);
        }
    }

    return 0;
}