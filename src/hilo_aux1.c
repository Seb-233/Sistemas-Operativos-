#include "buffer.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#define MAX_EJEMPLARES 100
#define MAX_LIBROS 100

typedef struct {
    int numero;
    char estado;
    char fecha[11]; // dd-mm-aaaa
} Ejemplar;

typedef struct {
    char nombre[MAX_LIBRO];
    int isbn;
    int cantidad;
    Ejemplar ejemplares[MAX_EJEMPLARES];
} Libro;

Libro libros[MAX_LIBROS];
int totalLibros = 0;

void cargarBaseDatos(const char *archivo) {
    FILE *f = fopen(archivo, "r");
    if (!f) {
        perror("[Hilo1] Error al abrir base_datos.txt");
        return;
    }

    totalLibros = 0;
    while (!feof(f)) {
        Libro l;
        char linea[256];

        if (!fgets(linea, sizeof(linea), f)) break;
        if (sscanf(linea, "%[^,], %d, %d", l.nombre, &l.isbn, &l.cantidad) != 3) continue;

        for (int i = 0; i < l.cantidad; i++) {
            if (!fgets(linea, sizeof(linea), f)) break;
            sscanf(linea, "%d, %c, %10s", &l.ejemplares[i].numero,
                                          &l.ejemplares[i].estado,
                                          l.ejemplares[i].fecha);
        }

        libros[totalLibros++] = l;
    }

    fclose(f);
}

void guardarBaseDatos(const char *archivo) {
    FILE *f = fopen(archivo, "w");
    if (!f) {
        perror("[Hilo1] Error al escribir base_datos.txt");
        return;
    }

    for (int i = 0; i < totalLibros; i++) {
        fprintf(f, "%s, %d, %d\n", libros[i].nombre, libros[i].isbn, libros[i].cantidad);
        for (int j = 0; j < libros[i].cantidad; j++) {
            fprintf(f, "%d, %c, %s\n", libros[i].ejemplares[j].numero,
                                       libros[i].ejemplares[j].estado,
                                       libros[i].ejemplares[j].fecha);
        }
    }

    fclose(f);
}

void obtenerFechaActual(char *dest) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(dest, "%02d-%02d-%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}

void procesarSolicitud(Mensaje m) {
    char fechaHoy[11];
    obtenerFechaActual(fechaHoy);

    int encontrado = 0;
    for (int i = 0; i < totalLibros; i++) {
        if (strcmp(libros[i].nombre, m.nombreLibro) == 0 || libros[i].isbn == m.isbn) {
            encontrado = 1;

            if (m.operacion == 'D') {
                for (int j = 0; j < libros[i].cantidad; j++) {
                    if (libros[i].ejemplares[j].estado == 'P') {
                        libros[i].ejemplares[j].estado = 'D';
                        strcpy(libros[i].ejemplares[j].fecha, fechaHoy);
                        printf("[Hilo1] Libro '%s': ejemplar %d marcado como disponible\n",
                               libros[i].nombre, libros[i].ejemplares[j].numero);
                        break;
                    }
                }
            } else if (m.operacion == 'R') {
                for (int j = 0; j < libros[i].cantidad; j++) {
                    if (libros[i].ejemplares[j].estado == 'P') {
                        strcpy(libros[i].ejemplares[j].fecha, fechaHoy);
                        printf("[Hilo1] Libro '%s': ejemplar %d renovado\n",
                               libros[i].nombre, libros[i].ejemplares[j].numero);
                        break;
                    }
                }
            }

            break;
        }
    }

    if (!encontrado) {
        printf("[Hilo1] Libro '%s' no encontrado para ISBN %d\n", m.nombreLibro, m.isbn);
    }
}

void *hiloAuxiliar1(void *arg) {
    BufferCircular *buffer = (BufferCircular *)arg;

    while (1) {
        Mensaje m = sacarBuffer(buffer);
        printf("[Hilo1] Procesando solicitud %c para libro '%s' (ISBN %d)\n",
               m.operacion, m.nombreLibro, m.isbn);

        cargarBaseDatos("base_datos.txt");
        procesarSolicitud(m);
        guardarBaseDatos("base_datos.txt");

        int fd = open(m.pipeRespuesta, O_WRONLY);
        if (fd != -1) {
            char respuesta[150];
            snprintf(respuesta, sizeof(respuesta),
                     "Solicitud %c procesada para el libro '%s'.\n",
                     m.operacion, m.nombreLibro);
            write(fd, respuesta, strlen(respuesta));
            close(fd);
        } else {
            perror("[Hilo1] No se pudo abrir pipeRespuesta");
        }
    }

    return NULL;
}

