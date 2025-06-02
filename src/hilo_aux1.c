#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "hilo_aux1.h"
#include "buffer.h"

extern void cargarBaseDatos(const char *archivo);
extern void guardarBaseDatos(const char *archivo);
extern BufferCircular bufferGlobal;
extern char archivoSalida[];
extern int verboseFlag;

void procesarSolicitud(Mensaje m) {
    FILE *f = fopen("base_datos.txt", "r");
    if (!f) {
        perror("[Hilo1] No se pudo abrir base_datos.txt");
        return;
    }

    char linea[256];
    char lineas[100][256];
    int total = 0;

    // Leer todas las líneas
    while (fgets(linea, sizeof(linea), f)) {
        strcpy(lineas[total++], linea);
    }
    fclose(f);

    int modificado = 0;

    // Buscar ejemplar exacto y modificar según operación
    for (int i = 1; i < total; i++) {
        int ejemplar;
        char estado, fecha[20];

        if (sscanf(lineas[i], "%d, %c, %s", &ejemplar, &estado, fecha) == 3) {
            if (ejemplar == m.ejemplar) {
                if (estado == 'D' && m.operacion == 'P') {
                    snprintf(lineas[i], sizeof(lineas[i]), "%d, P, 01-06-2024\n", ejemplar);
                    modificado = 1;
                    break;
                } else if (estado == 'P' && m.operacion == 'R') {
                    snprintf(lineas[i], sizeof(lineas[i]), "%d, R, 02-06-2024\n", ejemplar);
                    modificado = 1;
                    break;
                } else if ((estado == 'P' || estado == 'R') && m.operacion == 'D') {
                    snprintf(lineas[i], sizeof(lineas[i]), "%d, D, 03-06-2024\n", ejemplar);
                    modificado = 1;
                    break;
                }
            }
        }
    }

    // Guardar archivo
    f = fopen("base_datos.txt", "w");
    for (int i = 0; i < total; i++) {
        fputs(lineas[i], f);
    }
    fclose(f);

    if (modificado) {
        printf("[Hilo1] Solicitud %c procesada para '%s' (ejemplar %d).\n", m.operacion, m.nombreLibro, m.ejemplar);
        if (verboseFlag) {
            printf("[Verbose] Estado de base_datos.txt actualizado tras operación %c.\n", m.operacion);
        }
    } else {
        printf("[Hilo1] El ejemplar %d no se encontró o no está disponible para la operación '%c'.\n", m.ejemplar, m.operacion);
    }
}

void *hiloAuxiliar1(void *arg) {
    BufferCircular *buffer = (BufferCircular *)arg;
    while (1) {
        Mensaje m = sacarBuffer(buffer);
        printf("[Hilo1] Procesando solicitud %c para libro '%s' (ISBN %d) ejemplar %d\n", m.operacion, m.nombreLibro, m.isbn, m.ejemplar);

        if (verboseFlag) {
            printf("[Verbose] Extrayendo solicitud del buffer para operación %c.\n", m.operacion);
        }

        procesarSolicitud(m);

        int fd = open(m.pipeRespuesta, O_WRONLY);
        if (fd != -1) {
            char respuesta[150];
            snprintf(respuesta, sizeof(respuesta), "Solicitud %c procesada para el libro '%s'.\n", m.operacion, m.nombreLibro);
            write(fd, respuesta, strlen(respuesta));
            close(fd);

            if (verboseFlag) {
                printf("[Verbose] Respuesta enviada a %s\n", m.pipeRespuesta);
            }
        } else {
            perror("[Hilo1] No se pudo abrir pipeRespuesta");
        }
    }

    return NULL;
}
