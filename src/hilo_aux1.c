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

    while (fgets(linea, sizeof(linea), f)) {
        strcpy(lineas[total++], linea);
    }
    fclose(f);

    int i = 0;
    int encontradoLibro = 0, isbnCorrecto = 0, ejemplarEncontrado = 0, modificado = 0;
    char respuesta[256] = "";

    while (i < total) {
        char nombreLibro[100];
        int isbn, cantidad;

        if (sscanf(lineas[i], " %99[^,], %d, %d", nombreLibro, &isbn, &cantidad) == 3) {
            if (strcasecmp(nombreLibro, m.nombreLibro) == 0) {
                encontradoLibro = 1;
                if (isbn == m.isbn) {
                    isbnCorrecto = 1;

                    for (int j = 1; j <= cantidad && (i + j) < total; j++) {
                        int ejemplar;
                        char estado, fecha[20];

                        if (sscanf(lineas[i + j], "%d, %c, %s", &ejemplar, &estado, fecha) == 3) {
                            if (ejemplar == m.ejemplar) {
                                ejemplarEncontrado = 1;

                                if (estado == 'D' && m.operacion == 'P') {
                                    snprintf(lineas[i + j], sizeof(lineas[i + j]), "%d, P, 01-06-2025\n", ejemplar);
                                    snprintf(respuesta, sizeof(respuesta), "✅ Préstamo registrado para '%s', ejemplar %d.", m.nombreLibro, ejemplar);
                                    modificado = 1;
                                } else if (estado == 'P' && m.operacion == 'R') {
                                    snprintf(lineas[i + j], sizeof(lineas[i + j]), "%d, R, 01-06-2025\n", ejemplar);
                                    snprintf(respuesta, sizeof(respuesta), "✅ Renovación registrada para '%s', ejemplar %d.", m.nombreLibro, ejemplar);
                                    modificado = 1;
                                } else if ((estado == 'P' || estado == 'R') && m.operacion == 'D') {
                                    snprintf(lineas[i + j], sizeof(lineas[i + j]), "%d, D, 01-06-2025\n", ejemplar);
                                    snprintf(respuesta, sizeof(respuesta), "✅ Devolución registrada para '%s', ejemplar %d.", m.nombreLibro, ejemplar);
                                    modificado = 1;
                                } else {
                                    snprintf(respuesta, sizeof(respuesta), "⚠️ El ejemplar %d no permite la operación '%c'.", ejemplar, m.operacion);
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }
        i++;
    }

    if (!encontradoLibro) {
        snprintf(respuesta, sizeof(respuesta), "❌ El libro '%s' no existe en la base de datos.", m.nombreLibro);
    } else if (!isbnCorrecto) {
        snprintf(respuesta, sizeof(respuesta), "❌ ISBN %d incorrecto para el libro '%s'.", m.isbn, m.nombreLibro);
    } else if (!ejemplarEncontrado) {
        snprintf(respuesta, sizeof(respuesta), "❌ Ejemplar %d no encontrado en '%s'.", m.ejemplar, m.nombreLibro);
    }

    if (modificado) {
        f = fopen("base_datos.txt", "w");
        for (int i = 0; i < total; i++) {
            fputs(lineas[i], f);
        }
        fclose(f);
    }

    // Solo imprimir si verbose está activo
    if (verboseFlag) {
        printf("[Hilo1] Mensaje procesado: %s\n", respuesta);
    }

    // Enviar respuesta por pipe
    int fd = open(m.pipeRespuesta, O_WRONLY);
    if (fd != -1) {
        write(fd, respuesta, strlen(respuesta));
        close(fd);
        if (verboseFlag) {
            printf("[Verbose] Respuesta enviada a %s\n", m.pipeRespuesta);
        }
    } else if (verboseFlag) {
        perror("[Hilo1] No se pudo abrir pipeRespuesta");
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
