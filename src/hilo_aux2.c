#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hilo_aux2.h"

extern char archivoSalida[];
extern int verboseFlag;

#define MAX_LIBRO 100
#define MAX_ISBN 20
#define MAX_FECHA 20
#define MAX_REGISTROS 1000

typedef struct {
    char tipo; // 'P', 'R', 'D'
    char nombreLibro[MAX_LIBRO];
    char isbn[MAX_ISBN];
    int ejemplar;
    char fecha[MAX_FECHA];
} RegistroOperacion;

extern RegistroOperacion historial[MAX_REGISTROS];
extern int total_registros;

void generarReporte() {
    printf("\n--- Reporte de operaciones realizadas ---\n");
    printf("Tipo, Nombre del Libro, ISBN, Ejemplar, Fecha\n");

    for (int i = 0; i < total_registros; i++) {
        RegistroOperacion r = historial[i];
        printf("%c, %s, %s, %d, %s\n", r.tipo, r.nombreLibro, r.isbn, r.ejemplar, r.fecha);
    }

    if (total_registros == 0) {
        printf("(Sin operaciones registradas)\n");
    }

    printf("------------------------------------------\n");
}

void guardarEstadoFinal(const char *archivoSalida); // ya implementado en receptor.c

void *hiloAuxiliar2(void *arg) {
    char comando[10];

    while (1) {
        if (verboseFlag)
            printf("\n[Hilo2] Ingrese comando ('r' para reporte, 's' para salir): ");
        else
            printf("> ");

        fflush(stdout);
        fgets(comando, sizeof(comando), stdin);

        if (strncmp(comando, "r", 1) == 0) {
            generarReporte();
        } else if (strncmp(comando, "s", 1) == 0) {
            if (archivoSalida[0] != '\0') {
                guardarEstadoFinal(archivoSalida);
            }
            if (verboseFlag)
                printf("[Hilo2] Finalizando proceso receptor...\n");
            exit(0);
        } else {
            printf("Comando desconocido. Use 'r' o 's'.\n");
        }
    }

    return NULL;
}
