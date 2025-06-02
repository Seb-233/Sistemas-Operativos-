RECEPTOR = receptor
SOLICITANTE = solicitante

RECEPTOR_SRC = receptor.c buffer.c hilo_aux1.c hilo_aux2.c
SOLICITANTE_SRC = solicitante.c

CC = gcc
CFLAGS = -Wall -g
PTHREAD = -lpthread

$(RECEPTOR): $(RECEPTOR_SRC)
	$(CC) $(CFLAGS) $(RECEPTOR_SRC) -o $(RECEPTOR) $(PTHREAD)

$(SOLICITANTE): $(SOLICITANTE_SRC)
	$(CC) $(CFLAGS) $(SOLICITANTE_SRC) -o $(SOLICITANTE)

all: $(RECEPTOR) $(SOLICITANTE)

clean:
	rm -f $(RECEPTOR) $(SOLICITANTE) pipeRP pipeRespuesta_* salida.txt
