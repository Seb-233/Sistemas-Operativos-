#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hilo_aux2.h"

// Declaraciones externas (la estructura y las variables están en receptor.c)
extern int verboseFlag;
extern char archivoSalida[];

#define MAX_LIBRO 100
#define MAX_ISBN 20
#define MAX_FECHA 20
#define MAX_REGISTROS 1000

typedef struct {
    char tipo;
    char nombreLibro[MAX_LIBRO];
    char isbn[MAX_ISBN];
    int ejemplar;
    char fecha[MAX_FECHA];
} RegistroOperacion;

extern RegistroOperacion historial[MAX_REGISTROS];
extern int total_registros;

// Declaración de función definida en receptor.c
void guardarEstadoFinal(const char *archivo);

void generarReporte() {
    printf("\n--- Reporte de operaciones realizadas ---\n");
    printf("Tipo | Libro | ISBN | Ejemplar | Fecha\n");
    printf("----------------------------------------\n");

    for (int i = 0; i < total_registros; i++) {
        RegistroOperacion r = historial[i];
        printf("  %c  | %s | %s | %d | %s\n",
               r.tipo, r.nombreLibro, r.isbn, r.ejemplar, r.fecha);
    }

    if (total_registros == 0) {
        printf("(No se han registrado operaciones)\n");
    }

    printf("----------------------------------------\n\n");
}

void *hiloAuxiliar2(void *arg) {
    char comando[10];

    printf("Use 'r' para reporte o 's' para salir.\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(comando, sizeof(comando), stdin)) {
            continue;
        }

        if (strncmp(comando, "r", 1) == 0) {
            generarReporte();
        } else if (strncmp(comando, "s", 1) == 0) {
            if (archivoSalida[0] != '\0') {
                if (verboseFlag) {
                    printf("[Hilo2] Guardando base de datos en '%s'\n", archivoSalida);
                }
                guardarEstadoFinal(archivoSalida);
            } else if (verboseFlag) {
                printf("[Hilo2] Terminando sin guardar base de datos.\n");
            }
            exit(0);
        } else {
            if (verboseFlag) {
                printf("[Hilo2] Comando no reconocido: '%s'\n", comando);
            }
        }
    }

    return NULL;
}
