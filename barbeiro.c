#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Máximo de clientes na loja
#define MAX_CLIENTES 20

// Quantidade de clientes que querem cortar o cabelo
#define QTD_CLIENTES 20

// Quantidade atual de clientes
int clientes = 0;

// Inicializacao das threads dos clientes
pthread_t thr_clientes[QTD_CLIENTES];

// Inicialização das threads dos barbeiros
pthread_t thr_barbeiros[3];

// Inicialização dos semafóros
sem_t mutex, sem_cadeiras, sem_barbeiros, sem_clientes, sem_pagamento, sem_recibo;

// Struct para as fila
typedef struct {
	sem_t primeiro;
	sem_t segundo;
} Fifo;

// Inicialização das filas
Fifo *salaDeEspera, *sofa;

// Criação da fila
Fifo* criaFila(int n) {
	Fifo* f = (Fifo*) malloc(sizeof(Fifo));

	sem_init(&(f->primeiro), 0, 0);
	sem_init(&(f->segundo), 0, n);

	return f;
}

void esperaFila(Fifo* f, int n) {
	sem_wait(&(f->segundo));
	sem_post(&(f->primeiro));
}

void sinalizaFila(Fifo* f) {
	sem_wait(&(f->primeiro));
	sem_post(&(f->segundo));
}

void* barbearia(void* arg) {
	int n = *(int*) arg;

	sem_wait(&mutex);

	if (clientes >= MAX_CLIENTES) {
		sem_post(&mutex);
		printf("Cliente %d: indo embora...\n", n);
	}

	clientes += 1;

	sem_post(&mutex);

	esperaFila(salaDeEspera, n);

	printf("Cliente %d: entrando na fila de espera\n", n);

	esperaFila(sofa, n);

	printf("Cliente %d: sentando no sofa\n", n);

	sinalizaFila(salaDeEspera);

	sem_wait(&sem_cadeiras);

	printf("Cliente %d: sentando na cadeira\n", n);
	sleep(3);
	sinalizaFila(sofa);

	sem_post(&sem_clientes);

	sem_wait(&sem_barbeiros);

	printf("Cliente %d: teve o cabelo cortado\n", n);
	sleep(2);
	printf("Cliente %d: pagando\n", n);

	sem_post(&sem_pagamento);
	sem_wait(&sem_recibo);
	sem_wait(&mutex);

	clientes -= 1;

	sem_post(&mutex);

	printf("Cliente %d: indo embora...\n", n);
}

void* cortar(void* arg) {
	int n = *(int*) arg;

	for (int i = 0; i < QTD_CLIENTES; i++) {
		sem_wait(&sem_clientes);
		sem_post(&sem_barbeiros);

		printf("\tBarbeiro %d: cortando cabelo\n", n);
		sleep(3);
		sem_wait(&sem_pagamento);

		printf("\tBarbeiro %d: aceitando pagamento\n", n);
		sleep(1);
		sem_post(&sem_recibo);

		sem_post(&sem_cadeiras);
	}
}

int main() {
	// ID dos clientes
	int id_cliente[QTD_CLIENTES];

	// Criacao dos semafóros
	sem_init(&mutex, 0, 1);
	sem_init(&sem_cadeiras, 0, 3);
	sem_init(&sem_barbeiros, 0, 0);
	sem_init(&sem_clientes, 0, 0);
	sem_init(&sem_pagamento, 0, 0);
	sem_init(&sem_recibo, 0, 0);

	// Fila de espera
	salaDeEspera = criaFila(16);

	// Fila do sofá
	sofa = criaFila(4);

	// Criacao das threads dos clientes
	for (int i = 0; i < QTD_CLIENTES; i++) {
		id_cliente[i] = i;
		if (pthread_create(&thr_clientes[i], 0, barbearia, &id_cliente[i])) {
			perror("pthread_create");
			exit(1);
		}
	}

	// Criacao das threads dos barbeiros
	for (int i = 0; i < 3; i++) {
		if (pthread_create(&thr_barbeiros[i], 0, cortar, &id_cliente[i])) {
			perror("pthread_create");
			exit(1);
		}
	}

	// Join das threads dos clientes
	for (int i = 0; i < QTD_CLIENTES; i++) {
		if (pthread_join(thr_clientes[i], NULL)) {
			perror("pthread_join");
			exit(1);
		}
	}

	// libera a área de memória alocada pelas filas
	free(sofa);
	free(salaDeEspera);

	return 0;
}
