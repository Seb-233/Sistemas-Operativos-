#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LIBRO 100

typedef struct {
    char operacion;              // 'D', 'R', 'P', 'Q'
    char nombreLibro[MAX_LIBRO];
    int isbn;
} Mensaje;

// Leer desde archivo

void leerDesdeArchivo(const char *archivo, const char *pipe) {
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
        if (sscanf(linea, " %c, %99[^,], %d", &msg.operacion, msg.nombreLibro, &msg.isbn) == 3) {
            write(fd, &msg, sizeof(Mensaje));
            printf("Enviado: %c, %s, %d\n", msg.operacion, msg.nombreLibro, msg.isbn);
        } else {
            printf("Línea malformada: %s", linea);
        }
    }

    close(fd);
    fclose(f);
}

// Leer desde menu
void leerDesdeMenu(const char *pipe) {
    int fd = open(pipe, O_WRONLY);
    if (fd == -1) {
        perror("No se pudo abrir el pipe");
        return;
    }

    Mensaje msg;

    while (1) {
        printf("\nIngrese operación (D: Devolver, R: Renovar, P: Prestar, Q: Salir): ");
        scanf(" %c", &msg.operacion);

        if (msg.operacion == 'Q') {
            strcpy(msg.nombreLibro, "Salir");
            msg.isbn = 0;
            write(fd, &msg, sizeof(Mensaje));
            printf("Comando de salida enviado.\n");
            break;
        }

        printf("Nombre del libro: ");
        scanf(" %[^\n]", msg.nombreLibro);  // Lee hasta salto de línea

        printf("ISBN: ");
        scanf("%d", &msg.isbn);

        write(fd, &msg, sizeof(Mensaje));
        printf("Enviado: %c, %s, %d\n", msg.operacion, msg.nombreLibro, msg.isbn);
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

    if (usarArchivo) {
        leerDesdeArchivo(archivoEntrada, rutaPipe);
    } else {
        leerDesdeMenu(rutaPipe);
    }

    return 0;
}
