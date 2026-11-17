#ifndef ESTAFETA_H
#define ESTAFETA_H

// Constantes simbólicas específicas do estafeta e limites do sistema
#define MAX_USERNAME 32
#define MAX_LOCAL 16
#define MAX_PIPE_NAME 128
#define TIMEOUT_CENTRAL_SEG 10

// Estados possíveis de um estafeta
#define ESTADO_LIVRE 0
#define ESTADO_OCUPADO 1

// Identificadores de tipo de mensagem (campo comum char tipo[12])
#define MSG_PEDIDO "pedido"
#define MSG_REGISTO_EST "RegistoEst"
#define MSG_RESP_REGISTO "RespRegisto"
#define MSG_ANUNCIO "Anuncio"
#define MSG_ACEITA "Aceita"
#define MSG_RESP_ACEITA "RespAceita"
#define MSG_ENTREGUE "Entregue"
#define MSG_SAI "Sai"
#define MSG_PING "Ping"
#define MSG_ENCERRA "Encerra"

// Estrutura para guardar os dados da entrega atual do estafeta
typedef struct {
    int id_pedido;
    char origem[MAX_LOCAL];
    char destino[MAX_LOCAL];
    float peso;
    float distancia_km;
    float valor_euros;
} EntregaAtual;

// Estrutura principal de estado e configuração do programa estafeta
typedef struct {
    char username[MAX_USERNAME];
    char central_pipe[MAX_PIPE_NAME];
    char pipe_c2e[MAX_PIPE_NAME]; // Named pipe Central -> Estafeta
    char pipe_e2c[MAX_PIPE_NAME]; // Named pipe Estafeta -> Central
    int estado;                   // ESTADO_LIVRE ou ESTADO_OCUPADO
    int entregas_concluidas;
    float ganhos_totais;
    EntregaAtual entrega_atual;
} EstafetaState;

// Estruturas das mensagens trocadas entre Estafeta e Central
typedef struct {
    char tipo[12]; // "RegistoEst"
    char username[MAX_USERNAME];
    char pipe_c2e[MAX_PIPE_NAME];
    char pipe_e2c[MAX_PIPE_NAME];
} MsgRegistoEstafeta;

typedef struct {
    char tipo[12]; // "RespRegisto"
    int aceite;    // 1 = sucesso, 0 = erro (nome duplicado / lotação cheia)
    char motivo[64];
} MsgRespostaRegisto;

typedef struct {
    char tipo[12]; // "Anuncio"
    int id_pedido;
    char origem[MAX_LOCAL];
    char destino[MAX_LOCAL];
    float peso;
    float valor_euros;
    int tempo_restante;
} MsgAnuncioPedido;

typedef struct {
    char tipo[12]; // "Aceita" ou "Entregue" ou "Sai" ou "Ping"
    char username[MAX_USERNAME];
    int id_pedido;
} MsgAcaoEstafeta;

typedef struct {
    char tipo[12]; // "RespAceita" ou "Encerra"
    int sucesso;
    char mensagem[64];
} MsgRespostaCentral;

#endif