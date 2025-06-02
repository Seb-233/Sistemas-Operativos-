#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>
#include <semaphore.h>

#define TAM_BUFFER 10
#define MAX_LIBRO 100
#define MAX_PIPE 100

typedef struct {
    char operacion;
    char nombreLibro[MAX_LIBRO];
    int isbn;
    int ejemplar; // ✅ NUEVO
    char pipeRespuesta[MAX_PIPE];
} Mensaje;


typedef struct {
    Mensaje datos[TAM_BUFFER];
    int entrada;
    int salida;
    sem_t lleno;
    sem_t vacio;
    pthread_mutex_t mutex;
} BufferCircular;

// Declaraciones
void inicializarBuffer(BufferCircular *buffer);
void insertarBuffer(BufferCircular *buffer, Mensaje mensaje);
Mensaje sacarBuffer(BufferCircular *buffer);

#endif

