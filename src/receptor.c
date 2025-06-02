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
RegistroOperacion historial[MAX_REGISTROS];
int total_registros = 0;


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

    if (verboseFlag)
        printf("Receptor escuchando en pipe: %s\n", rutaPipe);

    // ⬇ ABRIR PIPE EN LECTURA Y EN ESCRITURA PARA EVITAR BLOQUEO
    int fd = open(rutaPipe, O_RDONLY);
    int dummy = open(rutaPipe, O_WRONLY); // mantiene el otro extremo abierto

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
                printf("\nSolicitud recibida:\n");
                printf("Operación: %c\n", msg.operacion);
                printf("Libro: %s\n", msg.nombreLibro);
                printf("ISBN: %d\n", msg.isbn);
                printf("Ejemplar: %d\n", msg.ejemplar);
                printf("Responder a: %s\n", msg.pipeRespuesta);
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
            // 🔁 PIPE CERRADO: REABRIR AMBOS EXTREMOS
            if (verboseFlag)
                printf("Pipe cerrado por el otro extremo. Esperando nuevos solicitantes...\n");

            close(fd);
            close(dummy);

            fd = open(rutaPipe, O_RDONLY);
            dummy = open(rutaPipe, O_WRONLY);
        } else {
            perror("Error al leer del pipe");
        }
    }

    close(fd);
    close(dummy);
    return 0;

}
