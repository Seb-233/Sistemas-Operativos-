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

#define MAX_LIBRO 100
#define MAX_PIPE 100

pthread_t hilo1, hilo2;
BufferCircular bufferGlobal;
int sistemaActivo = 1;
char archivoSalida[100] = "";
int verboseFlag = 0;

void *hiloAuxiliar1(void *arg);
void procesarSolicitud(Mensaje m);

void guardarEstadoFinal(const char *archivoSalida) {
    if (archivoSalida[0] == '\0') return;

    FILE *origen = fopen("base_datos.txt", "r");
    FILE *destino = fopen(archivoSalida, "w");

    if (!origen || !destino) {
        perror("[RP] No se pudo guardar el estado final");
        return;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), origen)) {
        fputs(linea, destino);
    }

    fclose(origen);
    fclose(destino);
    printf("[RP] Estado final guardado en: %s\n", archivoSalida);
}

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
    char nombreArchivoDatos[100] = "base_datos.txt";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strcpy(rutaPipe, argv[++i]);
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            strcpy(nombreArchivoDatos, argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            strcpy(archivoSalida, argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0) {
            verboseFlag = 1;
        }
    }

    if (rutaPipe[0] == '\0') {
        fprintf(stderr, "Uso: %s -p pipeReceptor [-f archivo] [-s salida] [-v]\n", argv[0]);
        return 1;
    }

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
            if (verboseFlag) {
                printf("[Verbose] Solicitud %c recibida para '%s' (ISBN %d)\n", msg.operacion, msg.nombreLibro, msg.isbn);
            }

            switch (msg.operacion) {
                case 'P':
                case 'D':
                case 'R':
                    if (verboseFlag) printf("[Verbose] Insertando en buffer...\n");
                    insertarBuffer(&bufferGlobal, msg);
                    if (verboseFlag) printf("[Verbose] Mensaje insertado correctamente.\n");
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
