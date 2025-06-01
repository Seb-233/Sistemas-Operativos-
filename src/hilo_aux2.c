#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hilo_aux2.h"

extern int sistemaActivo;
extern char archivoSalida[];
void guardarEstadoFinal(const char *archivoSalida);

void generarReporte() {
    FILE *f = fopen("base_datos.txt", "r");
    if (!f) {
        perror("[Hilo2] No se pudo abrir base_datos.txt");
        return;
    }

    char linea[256];
    char nombreLibro[50];
    int isbn, total = 0;

    printf("\n📋 REPORTE DEL SISTEMA:\n----------------------------\n");

    // Primera línea: nombre, isbn, total
    if (fgets(linea, sizeof(linea), f)) {
        sscanf(linea, "%[^,], %d, %d", nombreLibro, &isbn, &total);
        printf("Libro: %s\nISBN: %d\nTotal ejemplares: %d\n", nombreLibro, isbn, total);
    }

    // Resto: ejemplares
    int ejemplar;
    char estado, fecha[20];
    while (fgets(linea, sizeof(linea), f)) {
        if (sscanf(linea, "%d, %c, %s", &ejemplar, &estado, fecha) == 3) {
            printf("Ejemplar %d → Estado: %c → Fecha: %s\n", ejemplar, estado, fecha);
        }
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
                guardarEstadoFinal(archivoSalida);
                sistemaActivo = 0;
                exit(0);
            } else {
                printf("[Hilo2] Comando no válido.\n");
            }
        }
    }

    return NULL;
}
