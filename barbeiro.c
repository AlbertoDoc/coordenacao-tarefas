#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Máximo de clientes na loja
#define MAX_CLIENTES 20

// Quantidade de clientes que querem cortar o cabelo
#define QTD_CLIENTES 50

// Quantidade atual de clientes
int clientes = 0;

// Inicializacao das threads dos clientes
pthread_t thr_clientes[QTD_CLIENTES];

// Inicialização das threads dos barbeiros
pthread_t thr_barbeiros[3];

// Inicialização dos semafóros
sem_t mutex, sem_cadeiras, sem_barbeiros, sem_clientes, sem_pagamento, sem_recibo;

// Struct para as filas
typedef struct {
	sem_t primeiro;
	sem_t segundo;
} Fila;

// Inicialização das filas
Fila *salaDeEspera, *sofa;

// Criação da fila
Fila* criaFila(int n) {
	Fila* f = (Fila*) malloc(sizeof(Fila));

	sem_init(&(f->primeiro), 0, 0);
	sem_init(&(f->segundo), 0, n);

	return f;
}

// Entra na fila
void esperaFila(Fila* f) {
	sem_wait(&(f->segundo));
	sem_post(&(f->primeiro));
}


// Sai da fila
void sinalizaFila(Fila* f) {
	sem_wait(&(f->primeiro));
	sem_post(&(f->segundo));
}

void* barbearia(void* arg) {
	int n = *(int*) arg;

	sem_wait(&mutex);

	// Se a quantidade de cliente for maior que o máximo o excedente vai embora (e volta depois)
	if (clientes >= MAX_CLIENTES) {
		sem_post(&mutex);
		printf("Barbearia cheia! Cliente %d: indo embora...\n", n);
	}

	clientes += 1;

	sem_post(&mutex);

	// Entra na sala de espera
	esperaFila(salaDeEspera);

	printf("Cliente %d: entrando na fila de espera\n", n);

	// Senta no sofá
	esperaFila(sofa);

	printf("Cliente %d: sentando no sofa\n", n);

	sinalizaFila(salaDeEspera);

	// Cliente ocupa uma cadeira
	sem_wait(&sem_cadeiras);

	printf("Cliente %d: sentando na cadeira\n", n);
	sleep(3);

	// Cliente libera uma vaga no sofa
	sinalizaFila(sofa);

	//  Cliente termina de cortar cabelo
	sem_post(&sem_clientes);

	// Acorda um barbeiro
	sem_wait(&sem_barbeiros);

	printf("Cliente %d: teve o cabelo cortado\n", n);
	sleep(2);
	printf("Cliente %d: pagando\n", n);

	// Cliente paga e vai embora
	sem_post(&sem_pagamento);

	// Barbeiro recebe o dinheiro e emite o recibo
	sem_wait(&sem_recibo);

	sem_wait(&mutex);

	clientes -= 1;

	sem_post(&mutex);

	printf("Cliente %d: indo embora...\n", n);
}

void* cortar(void* arg) {
	int n = *(int*) arg;

	for (int i = 0; i < QTD_CLIENTES; i++) {
		// Cliente senta na cadeira e tem o cabelo cortado
		sem_wait(&sem_clientes);

		// Barbeiro termina de cortar o cabelo
		sem_post(&sem_barbeiros);

		printf("\tBarbeiro %d: cortando cabelo\n", n);
		sleep(3);

		// Cliente realiza o pagamento
		sem_wait(&sem_pagamento);

		printf("\tBarbeiro %d: aceitando pagamento\n", n);
		sleep(1);

		// Barbeiro emite o recibo
		sem_post(&sem_recibo);

		// Cadeira fica vaga
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
