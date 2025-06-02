#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hilo_aux2.h"

extern char archivoSalida[];
extern int verboseFlag;

void guardarEstadoFinal(const char *archivoSalida); // ya implementado en receptor.c

void generarReporte() {
    printf("\n--- Reporte de operaciones ---\n");
    printf("Esta funcionalidad se puede personalizar si se desea mostrar datos reales.\n");
    printf("--------------------------------\n");
}

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
