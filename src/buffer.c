#include "buffer.h"
#include <string.h>
#include <stdio.h>

void inicializarBuffer(BufferCircular *buffer) {
    buffer->entrada = 0;
    buffer->salida = 0;
    sem_init(&buffer->lleno, 0, 0);
    sem_init(&buffer->vacio, 0, TAM_BUFFER);
    pthread_mutex_init(&buffer->mutex, NULL);
}

void insertarBuffer(BufferCircular *buffer, Mensaje mensaje) {
    sem_wait(&buffer->vacio);  // Espera si está lleno
    pthread_mutex_lock(&buffer->mutex);

    buffer->datos[buffer->entrada] = mensaje;
    buffer->entrada = (buffer->entrada + 1) % TAM_BUFFER;

    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&buffer->lleno); // Avisa que hay un nuevo dato
}

Mensaje sacarBuffer(BufferCircular *buffer) {
    Mensaje mensaje;
    sem_wait(&buffer->lleno);  // Espera si está vacío
    pthread_mutex_lock(&buffer->mutex);

    mensaje = buffer->datos[buffer->salida];
    buffer->salida = (buffer->salida + 1) % TAM_BUFFER;

    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&buffer->vacio); // Avisa que hay un espacio libre

    return mensaje;
}
