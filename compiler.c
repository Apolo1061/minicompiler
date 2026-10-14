#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

typedef enum{
    TOKEN_RETURN,       // return
    TOKEN_INT,          // int
    TOKEN_SEMI          // ;
}tokentype;

struct token{
    tokentype tipo;
    int valor;
};

char *suki_file_open(const char *ruta){
    FILE *archivo = fopen(ruta,  "rb");

    if(archivo == NULL){
        fprintf(stderr, "error de lectura de archivo. %s", ruta);
        return NULL;
    }

    fseek(archivo, 0, SEEK_END);
    long tam = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    char *buffer = malloc(tam + 1);
    if(buffer == NULL){
        perror("error de asignacion de memoria");
        fclose(archivo);
        return NULL;
    }

    size_t leido = fread(buffer, 1, tam, archivo);
    if(leido != (size_t)tam){
        fprintf(stderr, "Error: se leyeron %zu de %ld bytes \n", leido, tam);
        fclose(archivo);
        free(buffer);
        return NULL;
    }

    buffer[leido] = '\0';

    fclose(archivo);
    return buffer;
}

int suki_lexer(const char *contenido, struct token **salida){
    struct token *Token = malloc((strlen(contenido) + 1) * sizeof(struct token));
    if(Token == NULL){
        return -1;
    }

    int recorrido = 0;
    size_t poss = 0;

    while(contenido[poss] != '\0'){
        if(isspace((unsigned char)contenido[poss])){
            poss++;
            continue;
        }

        if(isalpha((unsigned char)contenido[poss])){
            char tmp[64];
            int pos = 0;
            while(isalnum((unsigned char)contenido[poss]) && pos < 63){
                tmp[pos] = contenido[poss];
                ++pos;
                ++poss;
            }
            tmp[pos] = '\0';

            if(strcmp(tmp, "return") == 0){
                Token[recorrido].tipo = TOKEN_RETURN;
                Token[recorrido].valor = 0;
                recorrido++;
            }else{
                fprintf(stderr, "Error: '%s' es desconocido para el lexer\n", tmp);
                free(Token);
                return -1;
            }
            continue;
        }

        if(isdigit((unsigned char)contenido[poss])){
            int var = 0;
            while(isdigit((unsigned char)contenido[poss])){
                var = var * 10 + (contenido[poss] - '0');
                poss++;
            }
            Token[recorrido].tipo = TOKEN_INT;
            Token[recorrido].valor = var;

            recorrido++;
            continue;
        }

        if(contenido[poss] == ';'){
            Token[recorrido].tipo = TOKEN_SEMI;
            Token[recorrido].valor = 0;

            recorrido++;
            ++poss;
            continue;
        }

        fprintf(stderr, "Error: '%c' lexer no lo conoce\n", contenido[poss]);
        free(Token);
        return -1;
    }

    *salida = Token;

    return recorrido;
}

int token_a_asm(const struct token *Token, int total){
    FILE *asmfile = fopen("code.s", "w");
    if(asmfile == NULL){
        perror("no se pudo crear code.s");
        return -1;
    }

    fprintf(asmfile, "global _start\n\n_start:\n");

    for(int i = 0; i < total; ++i){
        if(Token[i].tipo == TOKEN_RETURN){
            if(i + 1 < total && Token[i + 1].tipo == TOKEN_INT){
                fprintf(asmfile, "    mov rax, 60\n");
                fprintf(asmfile, "    mov rdi, %d\n", Token[i + 1].valor);
                fprintf(asmfile, "    syscall\n");
                i++;
            }else{
                fprintf(stderr, "\nError. falta el numero despues del return\n");
                fclose(asmfile);
                return -1;
            }
        }
    }

    fclose(asmfile);
    return 0;
}

int main(int argumento, char *argumentos[]){
    if(argumento != 2){
        fprintf(stderr, "\nError no se a encontrado el argumento");
        fprintf(stderr, "\nuso: itsuki <main.su>\n");
        return -1;
    }

    char *ruta =  argumentos[1];

    char *contenido = suki_file_open(ruta);
    if(contenido == NULL){
        fprintf(stderr, "Error de apertura de archivo %s\n", ruta);
        return -1;
    }

    struct token *Token = NULL;

    int total = suki_lexer(contenido, &Token);
    if(total < 0){
        fprintf(stderr, "Error de creacion de tokens\n");
        free(contenido);
        return -1;
    }

    if(token_a_asm(Token, total) != 0){
        free(Token);
        free(contenido);
        return -1;
    }

    fprintf(stdout, "Codigo asm generado en code.s\n");

    free(Token);
    free(contenido);
    return 0;
}
