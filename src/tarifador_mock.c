#include <stdio.h>
#include <stdlib.h>

int main() {
    char origem[50], destino[50];
    float peso;

    while (scanf("%49s %49s %f", origem, destino, &peso) == 3) {

        printf("3.1 4.85\n");
        fflush(stdout); // Muito importante para os pipes não ficarem bloqueados!
    }

    return 0;
}