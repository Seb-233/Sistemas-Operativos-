#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "hilo_aux1.h"
#include "buffer.h"
#include <ctype.h>
#include <strings.h>  // para strcasecmp()

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

    int i = 0;
    int encontradoLibro = 0, isbnCorrecto = 0, ejemplarEncontrado = 0, modificado = 0;

    while (i < total) {
        char nombreLibro[100];
        int isbn, cantidad;

        if (sscanf(lineas[i], " %99[^,], %d, %d", nombreLibro, &isbn, &cantidad) == 3) {
            if (strcasecmp(nombreLibro, m.nombreLibro) == 0) {
                encontradoLibro = 1;
                if (isbn == m.isbn) {
                    isbnCorrecto = 1;

                    // Buscar entre las siguientes 'cantidad' líneas
                    for (int j = 1; j <= cantidad && (i + j) < total; j++) {
                        int ejemplar;
                        char estado, fecha[20];

                        if (sscanf(lineas[i + j], "%d, %c, %s", &ejemplar, &estado, fecha) == 3) {
                            if (ejemplar == m.ejemplar) {
                                ejemplarEncontrado = 1;

                                // Validar estado y aplicar cambio
                                if (estado == 'D' && m.operacion == 'P') {
                                    snprintf(lineas[i + j], sizeof(lineas[i + j]), "%d, P, 01-06-2025\n", ejemplar);
                                    modificado = 1;
                                } else if (estado == 'P' && m.operacion == 'R') {
                                    snprintf(lineas[i + j], sizeof(lineas[i + j]), "%d, R, 01-06-2025\n", ejemplar);
                                    modificado = 1;
                                } else if ((estado == 'P' || estado == 'R') && m.operacion == 'D') {
                                    snprintf(lineas[i + j], sizeof(lineas[i + j]), "%d, D, 01-06-2025\n", ejemplar);
                                    modificado = 1;
                                } else {
                                    printf("[Hilo1] El estado del ejemplar %d no permite aplicar la operación '%c'.\n", ejemplar, m.operacion);
                                }
                                break;
                            }
                        }
                    }

                    break;  // Ya encontramos el bloque que nos interesa
                }
            }
        }
        i++;
    }

    // Guardar cambios si fue modificado
    if (modificado) {
        f = fopen("base_datos.txt", "w");
        for (int i = 0; i < total; i++) {
            fputs(lineas[i], f);
        }
        fclose(f);
        printf("[Hilo1] ✅ Solicitud %c procesada para '%s' (ISBN %d), ejemplar %d.\n",
               m.operacion, m.nombreLibro, m.isbn, m.ejemplar);
        if (verboseFlag)
            printf("[Verbose] Estado actualizado correctamente.\n");
    } else {
        // Mensajes de error
        if (!encontradoLibro) {
            printf("[Hilo1] ❌ Libro '%s' no encontrado (nombre incorrecto).\n", m.nombreLibro);
        } else if (!isbnCorrecto) {
            printf("[Hilo1] ❌ ISBN %d incorrecto para el libro '%s'.\n", m.isbn, m.nombreLibro);
        } else if (!ejemplarEncontrado) {
            printf("[Hilo1] ❌ El ejemplar %d no se encontró dentro de '%s' (ISBN %d).\n", m.ejemplar, m.nombreLibro, m.isbn);
        }
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
