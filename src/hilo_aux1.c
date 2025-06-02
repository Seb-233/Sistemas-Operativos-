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
    char lineas[1000][256];
    int total = 0;

    // Leer todo el archivo
    while (fgets(linea, sizeof(linea), f)) {
        strcpy(lineas[total++], linea);
    }
    fclose(f);

    int i = 0, modificado = 0, encontrado = 0;
    while (i < total) {
        char nombreLibro[100];
        int isbn, cantidad;

        if (sscanf(lineas[i], " %99[^,], %d, %d", nombreLibro, &isbn, &cantidad) == 3) {
            if (strcmp(nombreLibro, m.nombreLibro) == 0 && isbn == m.isbn) {
                encontrado = 1;
                // Buscar entre las siguientes 'cantidad' líneas
                for (int j = 1; j <= cantidad && (i + j) < total; j++) {
                    int ejemplar;
                    char estado, fecha[20];

                    if (sscanf(lineas[i + j], "%d, %c, %s", &ejemplar, &estado, fecha) == 3) {
                        if (ejemplar == m.ejemplar) {
                            // Verificar operación y estado
                            if (estado == 'D' && m.operacion == 'P') {
                                snprintf(lineas[i + j], sizeof(lineas[i + j]), "%d, P, 01-06-2025\n", ejemplar);
                                modificado = 1;
                            } else if (estado == 'P' && m.operacion == 'R') {
                                snprintf(lineas[i + j], sizeof(lineas[i + j]), "%d, R, 01-06-2025\n", ejemplar);
                                modificado = 1;
                            } else if ((estado == 'P' || estado == 'R') && m.operacion == 'D') {
                                snprintf(lineas[i + j], sizeof(lineas[i + j]), "%d, D, 01-06-2025\n", ejemplar);
                                modificado = 1;
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
        i++;
    }

    if (!encontrado) {
        printf("[Hilo1] El libro '%s' con ISBN %d no existe en la base de datos.\n", m.nombreLibro, m.isbn);
    } else if (!modificado) {
        printf("[Hilo1] El ejemplar %d no se encontró o no se pudo modificar por estado inválido.\n", m.ejemplar);
    }

    // Guardar cambios
    f = fopen("base_datos.txt", "w");
    for (int i = 0; i < total; i++) {
        fputs(lineas[i], f);
    }
    fclose(f);

    if (modificado) {
        printf("[Hilo1] Solicitud %c procesada para '%s' (ISBN %d), ejemplar %d.\n", m.operacion, m.nombreLibro, m.isbn, m.ejemplar);
        if (verboseFlag)
            printf("[Verbose] Estado de base_datos.txt actualizado correctamente.\n");
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
