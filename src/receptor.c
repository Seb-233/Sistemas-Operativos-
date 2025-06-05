#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include "buffer.h"
#include "hilo_aux2.h"

#define MAX_LIBRO 100
#define MAX_ISBN 20
#define MAX_FECHA 20
#define MAX_REGISTROS 1000

typedef struct {
    char tipo;
    char nombreLibro[MAX_LIBRO];
    char isbn[MAX_ISBN];
    int ejemplar;
    char fecha[MAX_FECHA];
} RegistroOperacion;

RegistroOperacion historial[MAX_REGISTROS];
int total_registros = 0;

char archivoSalida[100] = "";
int verboseFlag = 0;

extern void *hiloAuxiliar1(void *arg);
extern void *hiloAuxiliar2(void *arg);

BufferCircular bufferGlobal;
pthread_t hilo1, hilo2;

void guardarEstadoFinal(const char *archivo) {
    FILE *f = fopen(archivo, "w");
    if (!f) {
        perror("[RP] No se pudo guardar el estado final");
        return;
    }

    FILE *original = fopen("base_datos.txt", "r");
    if (!original) {
        perror("[RP] No se pudo abrir base_datos.txt");
        fclose(f);
        return;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), original)) {
        fputs(linea, f);
    }

    fclose(original);
    fclose(f);
    printf("[RP] Estado final guardado en: %s\n", archivo);
}

void enviarRespuesta(const char *pipeRespuesta, const char *mensaje) {
    int fd = open(pipeRespuesta, O_WRONLY);
    if (fd != -1) {
        write(fd, mensaje, strlen(mensaje));
        close(fd);
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

    int fd = open(rutaPipe, O_RDONLY);
    int dummy = open(rutaPipe, O_WRONLY); // para evitar bloqueo

    inicializarBuffer(&bufferGlobal);

    pthread_create(&hilo1, NULL, hiloAuxiliar1, (void *)&bufferGlobal);
    pthread_create(&hilo2, NULL, hiloAuxiliar2, NULL);

    Mensaje msg;
    while (1) {
    ssize_t n = read(fd, &msg, sizeof(Mensaje));

    // Si read() devuelve 0, significa que ningún PS está escribiendo
    if (n == 0) {
        if (verboseFlag) {
            printf("[RP] No hay PS escribiendo. Reabriendo pipe...\n");
        }
        close(fd);
        close(dummy);

        // Reabrir el pipe (espera bloqueante hasta que un PS se conecte)
        fd = open(rutaPipe, O_RDONLY);
        dummy = open(rutaPipe, O_WRONLY);
        continue;
    }

    // Si read() falla por otro motivo
    if (n == -1) {
        perror("[RP] Error al leer del pipe");
        continue;
    }

    // Procesar mensaje válido
    if (verboseFlag) {
        printf("[RP] Mensaje recibido: %c - %s\n", msg.operacion, msg.nombreLibro);
    }

    switch (msg.operacion) {
        case 'P':
        case 'D':
        case 'R':
            insertarBuffer(&bufferGlobal, msg);
            break;
        case 'Q':
            enviarRespuesta(msg.pipeRespuesta, "Fin de sesión. Gracias.");
            break;
        default:
            enviarRespuesta(msg.pipeRespuesta, "Operación no reconocida.");
            break;
    }
}


    close(fd);
    close(dummy);
    return 0;
}
