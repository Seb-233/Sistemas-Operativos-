#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LIBRO 100
#define MAX_PIPE 100

typedef struct {
    char operacion;
    char nombreLibro[MAX_LIBRO];
    int isbn;
    int ejemplar;
    char pipeRespuesta[MAX_PIPE];
} Mensaje;

int leerDesdeArchivo(const char *nombreArchivo, Mensaje *mensajes, int *cantidad) {
    FILE *archivo = fopen(nombreArchivo, "r");
    if (!archivo) {
        perror("Error al abrir archivo de entrada");
        return -1;
    }

    char linea[256];
    int i = 0;
    while (fgets(linea, sizeof(linea), archivo)) {
        Mensaje msg;
        if (sscanf(linea, " %c, %99[^,], %d, %d", &msg.operacion, msg.nombreLibro, &msg.isbn, &msg.ejemplar) == 4) {
            mensajes[i++] = msg;
        } else if (sscanf(linea, " %c, %99[^,], %d", &msg.operacion, msg.nombreLibro, &msg.isbn) == 3) {
            // Si el archivo no tiene 'ejemplar', se asume 1
            msg.ejemplar = 1;
            mensajes[i++] = msg;
        }
    }
    fclose(archivo);
    *cantidad = i;
    return 0;
}

int leerDesdeMenu(Mensaje *msg) {
    printf("\nIngrese operación (D: Devolver, R: Renovar, P: Prestar, Q: Salir): ");
    scanf(" %c", &msg->operacion);

    if (msg->operacion == 'Q') return 0;

    printf("Nombre del libro: ");
    scanf(" %s", msg->nombreLibro);
    printf("ISBN: ");
    scanf("%d", &msg->isbn);
    printf("Ejemplar: ");
    scanf("%d", &msg->ejemplar);

    return 1;
}

int main(int argc, char *argv[]) {
    char archivoEntrada[100] = "";
    char pipeReceptor[100] = "";

    // Procesamiento de argumentos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            strcpy(archivoEntrada, argv[++i]);
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strcpy(pipeReceptor, argv[++i]);
        } else {
            fprintf(stderr, "Uso: %s [-i archivoEntrada] -p pipeReceptor\n", argv[0]);
            return 1;
        }
    }

    if (pipeReceptor[0] == '\0') {
        fprintf(stderr, "Error: No se especificó el pipe receptor.\n");
        return 1;
    }

    Mensaje mensajes[100];
    int cantidad = 0;

    if (archivoEntrada[0] != '\0') {
        printf("[PS] Leyendo solicitudes desde archivo: %s\n", archivoEntrada);
        if (leerDesdeArchivo(archivoEntrada, mensajes, &cantidad) == -1) return 1;

        for (int i = 0; i < cantidad; i++) {
            Mensaje msg = mensajes[i];
            snprintf(msg.pipeRespuesta, sizeof(msg.pipeRespuesta), "pipeRespuesta_%d", getpid());
            mkfifo(msg.pipeRespuesta, 0666);

            int fd = open(pipeReceptor, O_WRONLY);
            if (fd != -1) {
                write(fd, &msg, sizeof(Mensaje));
                close(fd);
            }

            fd = open(msg.pipeRespuesta, O_RDONLY);
            if (fd != -1) {
                char buffer[256];
                int n = read(fd, buffer, sizeof(buffer) - 1);
                if (n > 0) {
                    buffer[n] = '\0';
                    printf("Respuesta del RP: %s\n", buffer);
                }
                close(fd);
            }
        }
    } else {
        printf("[PS] Iniciando menú interactivo...\n");
        while (1) {
            Mensaje msg;
            if (!leerDesdeMenu(&msg)) {
                // Enviar mensaje de salida
                snprintf(msg.pipeRespuesta, sizeof(msg.pipeRespuesta), "pipeRespuesta_%d", getpid());
                mkfifo(msg.pipeRespuesta, 0666);
                int fd = open(pipeReceptor, O_WRONLY);
                if (fd != -1) {
                    write(fd, &msg, sizeof(Mensaje));
                    close(fd);
                }
                printf("Comando de salida enviado. Esperando confirmación...\n");
                break;
            }

            snprintf(msg.pipeRespuesta, sizeof(msg.pipeRespuesta), "pipeRespuesta_%d", getpid());
            mkfifo(msg.pipeRespuesta, 0666);

            int fd = open(pipeReceptor, O_WRONLY);
            if (fd != -1) {
                write(fd, &msg, sizeof(Mensaje));
                close(fd);
            }

            fd = open(msg.pipeRespuesta, O_RDONLY);
            if (fd != -1) {
                char buffer[256];
                int n = read(fd, buffer, sizeof(buffer) - 1);
                if (n > 0) {
                    buffer[n] = '\0';
                    printf("Respuesta del RP: %s\n", buffer);
                }
                close(fd);
            }
        }
    }

    return 0;
}
