#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#define MAX_LIBRO 100
#define MAX_PIPE 100

typedef struct {
    char operacion;                  // 'D', 'R', 'P', 'Q'
    char nombreLibro[MAX_LIBRO];
    int isbn;
    char pipeRespuesta[MAX_PIPE];   // Pipe para respuesta del RP
} Mensaje;

// Leer desde archivo
void leerDesdeArchivo(const char *archivo, const char *pipe, const char *pipeRespuesta) {
    FILE *f = fopen(archivo, "r");
    if (!f) {
        perror("No se pudo abrir el archivo de entrada");
        return;
    }

    int fd = open(pipe, O_WRONLY);
    if (fd == -1) {
        perror("No se pudo abrir el pipe");
        fclose(f);
        return;
    }

    char linea[256];
    Mensaje msg;

    while (fgets(linea, sizeof(linea), f)) {
        if (sscanf(linea, " %c, %99[^,], %d, %d", &msg.operacion, msg.nombreLibro, &msg.isbn, &msg.ejemplar) == 4){

            // VALIDACIONES NUEVAS
            if (msg.operacion != 'D' && msg.operacion != 'R' && msg.operacion != 'P' && msg.operacion != 'Q') {
                printf("Operación inválida en línea: %s", linea);
                continue;
            }

            if (msg.isbn <= 0) {
                printf("ISBN inválido (debe ser positivo): %s", linea);
                continue;
            }

            strcpy(msg.pipeRespuesta, pipeRespuesta);
            write(fd, &msg, sizeof(Mensaje));
            printf("Enviado: %c, %s, %d\n", msg.operacion, msg.nombreLibro, msg.isbn);

            // Esperar y leer respuesta
            printf("Esperando respuesta del RP...\n");
            int fdResp = open(pipeRespuesta, O_RDONLY);
            if (fdResp != -1) {
                char buffer[256];
                int n = read(fdResp, buffer, sizeof(buffer));
                buffer[n > 0 ? n : 0] = '\0';
                close(fdResp);
                printf("Respuesta del RP: %s\n", buffer);
            } else {
                perror("No se pudo leer respuesta del RP");
            }

        } else {
            printf("Línea malformada: %s", linea);
        }
    }

    close(fd);
    fclose(f);
}

// Leer desde menú
void leerDesdeMenu(const char *pipe, const char *pipeRespuesta) {
    int fd = open(pipe, O_WRONLY);
    if (fd == -1) {
        perror("No se pudo abrir el pipe");
        return;
    }

    Mensaje msg;

    while (1) {
        printf("\nIngrese operación (D: Devolver, R: Renovar, P: Prestar, Q: Salir): ");
        scanf(" %c", &msg.operacion);

        // VALIDACIÓN OPERACIÓN
        if (msg.operacion != 'D' && msg.operacion != 'R' && msg.operacion != 'P' && msg.operacion != 'Q') {
            printf("Operación inválida. Intente de nuevo.\n");
            continue;
        }

        if (msg.operacion == 'Q') {
            strcpy(msg.nombreLibro, "Salir");
            msg.isbn = 0;
            strcpy(msg.pipeRespuesta, pipeRespuesta);
            write(fd, &msg, sizeof(Mensaje));
            printf("Comando de salida enviado. Esperando confirmación...\n");

            int fdResp = open(pipeRespuesta, O_RDONLY);
            if (fdResp != -1) {
                char buffer[256];
                int n = read(fdResp, buffer, sizeof(buffer));
                buffer[n > 0 ? n : 0] = '\0';
                close(fdResp);
                printf("Respuesta del RP: %s\n", buffer);
            } else {
                perror("No se pudo leer respuesta del RP");
            }

            break;
        }

        printf("Nombre del libro: ");
        scanf(" %[^\n]", msg.nombreLibro);

        printf("ISBN: ");
        scanf("%d", &msg.isbn);

        // VALIDACIÓN ISBN
        if (msg.isbn <= 0) {
            printf("ISBN inválido. Debe ser un número positivo.\n");
            continue;
        }

        printf("Ejemplar: ");
        scanf("%d", &msg.ejemplar);


        strcpy(msg.pipeRespuesta, pipeRespuesta);
        write(fd, &msg, sizeof(Mensaje));
        printf("Enviado: %c, %s, %d\n", msg.operacion, msg.nombreLibro, msg.isbn);

        printf("Esperando respuesta del RP...\n");
        int fdResp = open(pipeRespuesta, O_RDONLY);
        if (fdResp != -1) {
            char buffer[256];
            int n = read(fdResp, buffer, sizeof(buffer));
            buffer[n > 0 ? n : 0] = '\0';
            close(fdResp);
            printf("Respuesta del RP: %s\n", buffer);
        } else {
            perror("No se pudo leer respuesta del RP");
        }
    }

    close(fd);
}

// Función principal
int main(int argc, char *argv[]) {
    char archivoEntrada[100] = "";
    char rutaPipe[100] = "";
    int usarArchivo = 0;

    if (argc < 3) {
        fprintf(stderr, "Uso: %s [-i archivo] -p pipe\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            strcpy(archivoEntrada, argv[++i]);
            usarArchivo = 1;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strcpy(rutaPipe, argv[++i]);
        }
    }

    if (strlen(rutaPipe) == 0) {
        fprintf(stderr, "Error: debe especificar -p pipe\n");
        return 1;
    }

    char pipeRespuesta[MAX_PIPE];
    snprintf(pipeRespuesta, MAX_PIPE, "pipeRespuesta_%d", getpid());
    if (mkfifo(pipeRespuesta, 0666) == -1 && errno != EEXIST) {
        perror("Error creando pipe de respuesta");
        return 1;
    }

    if (usarArchivo) {
        leerDesdeArchivo(archivoEntrada, rutaPipe, pipeRespuesta);
    } else {
        leerDesdeMenu(rutaPipe, pipeRespuesta);
    }

    return 0;
}
