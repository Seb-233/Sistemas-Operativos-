#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hilo_aux2.h"

extern int sistemaActivo;

void generarReporte() {
    FILE *f = fopen("base_datos.txt", "r");
    if (!f) {
        perror("[Hilo2] No se pudo abrir base_datos.txt");
        return;
    }

    char linea[256];
    printf("\n📋 REPORTE DEL SISTEMA:\n----------------------------\n");
    while (fgets(linea, sizeof(linea), f)) {
        printf("%s", linea);
    }
    printf("----------------------------\n");
    fclose(f);
}

void *hiloAuxiliar2(void *arg) {
    char comando[10];

    while (sistemaActivo) {
        printf("\n[Hilo2] Ingrese comando ('r' para reporte, 's' para salir): ");
        fflush(stdout);

        if (fgets(comando, sizeof(comando), stdin) != NULL) {
            if (comando[0] == 'r') {
                generarReporte();
            } else if (comando[0] == 's') {
                printf("[Hilo2] Cerrando el sistema...\n");
                sistemaActivo = 0;
                exit(0);
            } else {
                printf("[Hilo2] Comando no válido.\n");
            }
        }
    }

    return NULL;
}
