#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include "buffer.h"
#include "hilo_aux2.h"
#include "hilo_aux1.h"

#define MAX_LIBRO 100
#define MAX_PIPE 100

pthread_t hilo1, hilo2;
BufferCircular bufferGlobal;
int sistemaActivo = 1;  // bandera global para controlar salida

void enviarRespuesta(const char *pipeRespuesta, const char *mensaje) {
    int fdResp = open(pipeRespuesta, O_WRONLY);
    if (fdResp != -1) {
        write(fdResp, mensaje, strlen(mensaje));
        close(fdResp);
    } else {
        perror("No se pudo abrir el pipe de respuesta");
    }
}

int main(int argc, char *argv[]) {
    char rutaPipe[100] = "";
pthread_create(&hilo1, NULL, hiloAuxiliar1, (void *)&bufferGlobal);
pthread_create(&hilo2, NULL, hiloAuxiliar2, NULL);

    if (argc < 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Uso: %s -p pipeReceptor\n", argv[0]);
        return 1;
    }

    strcpy(rutaPipe, argv[2]);

    if (mkfifo(rutaPipe, 0666) == -1 && errno != EEXIST) {
        perror("Error al crear el pipe receptor");
        return 1;
    }

    printf("Receptor escuchando en pipe: %s\n", rutaPipe);

    int fd = open(rutaPipe, O_RDONLY);
    if (fd == -1) {
        perror("No se pudo abrir el pipe receptor");
        return 1;
    }

    inicializarBuffer(&bufferGlobal);

    if (pthread_create(&hilo1, NULL, hiloAuxiliar1, (void *)&bufferGlobal) != 0) {
        perror("Error al crear el hilo auxiliar 1");
        return 1;
    }

    if (pthread_create(&hilo2, NULL, hiloAuxiliar2, NULL) != 0) {
        perror("Error al crear el hilo auxiliar 2");
        return 1;
    }

    Mensaje msg;
    while (1) {
        ssize_t n = read(fd, &msg, sizeof(Mensaje));
        if (n == sizeof(Mensaje)) {
            printf("\nSolicitud recibida:\n");
            printf("Operación: %c\n", msg.operacion);
            printf("Libro: %s\n", msg.nombreLibro);
            printf("ISBN: %d\n", msg.isbn);
            printf("Responder a: %s\n", msg.pipeRespuesta);

            switch (msg.operacion) {
                case 'P':
                    enviarRespuesta(msg.pipeRespuesta, "Prestamo procesado (simulado).\n");
                    break;
                case 'D':
                case 'R':
                    insertarBuffer(&bufferGlobal, msg);
                    break;
                case 'Q':
                    enviarRespuesta(msg.pipeRespuesta, "Finalizando sesión del solicitante.\n");
                    break;
                default:
                    enviarRespuesta(msg.pipeRespuesta, "Operación no reconocida.\n");
                    break;
            }
        } else if (n == 0) {
            printf("Pipe cerrado por el otro extremo. Esperando nuevos solicitantes...\n");
            close(fd);
            fd = open(rutaPipe, O_RDONLY);
        } else {
            perror("Error al leer del pipe");
        }
    }

    close(fd);
    return 0;
}
