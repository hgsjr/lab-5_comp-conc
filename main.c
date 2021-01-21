#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<pthread.h>

#define THREADS_DE_LEITURA 2 // quantidade de threads leitoras
#define THREADS_DE_ESCRITA 2 // quantidade de threads escritoras

int vet[5]; // vet de 5 elementos para uma melhor exibição
int ler = 0, escr = 0; // variáveis de estado (variam entre zero e um)


// variáveis de sincronização
pthread_mutex_t lock;
pthread_cond_t cond_ler, cond_escr;

// função que imprime o vet e calcula a média
void leituraVetor(int id){

    float media = 0;
    printf("L[%d]Vetor", id);
    for(int i=0; i<5; i++){

        printf("%d ", vet[i]);
        media += vet[i];
    }
    printf("\n");
    printf("Média de vet = %f\n", media);
}

// função que faz a escr no vet
void escritaVetor(int id){

    vet[0] = id;
    vet[4] = id;
    for(int i=1; i<4; i++){

        vet[i] = 2 * id;
    }
    printf("E[%d] - vet modificado", id);
    for(int i=0; i<5; i++) // imprime o vet após a ler

        printf("%d ", vet[i]);
    printf("\n");
}

// função que inicializa o vet com zeros
void inicializaVetor(){

    for(int i=0; i<5; i++)

        vet[i] = 0;
}

// função que inicia a ler
void entraLeitura(int id){

    pthread_mutex_lock(&lock);
    printf("L[%d] quer ler", id);
    while(escr > 0){ // se escr for maior que zero, há alguém escrevendo, portanto, a ler precisa se bloquear

        printf("L[%d] bloqueou\n", id);
        pthread_cond_wait(&cond_ler, &lock);
        printf("L[%d] desbloqueou\n", id);
    }
    ler++;
    pthread_mutex_unlock(&lock);
}

// função que termina a ler
void saiLeitura(int id){

    pthread_mutex_lock(&lock);
    printf("L[%d] terminou de ler\n", id);
    ler--;
    if(ler == 0) // se ler for zero quer dizer que é a última leitora, logo, libera uma escritora

        pthread_cond_signal(&cond_escr); // libera uma escr
    pthread_mutex_unlock(&lock);
}

// thread leitora
void *leitor(void *args){

    int *id = (int*)args;
    while(1){

        entraLeitura(*id);
        leituraVetor(*id);
        saiLeitura(*id);
        sleep(1);
    }

    pthread_exit(NULL);
}

void entraEscrita(int id){

    pthread_mutex_lock(&lock);
    printf("E[%d] quer escrever\n", id);
    while((ler > 0) || (escr > 0)){ // nesse caso não pode ter ninguém lendo nem escrevendo

        printf("E[%d] bloqueou\n", id);
        pthread_cond_wait(&cond_escr, &lock);
        printf("E[%d] desbloqueou\n", id);
    }
    escr++;
    pthread_mutex_unlock(&lock);
}

void saiEscrita(int id){

    pthread_mutex_lock(&lock);
    printf("E[%d] terminou de escrever\n", id);
    escr--;
    pthread_cond_signal(&cond_escr); //sinaliza um escritor
    pthread_cond_broadcast(&cond_ler); // sinaliza todos os leitores na fila
    pthread_mutex_unlock(&lock);
}

// thread escritora
void *escritor(void *args){

    int *id = (int*)args;
    while(1){

        entraEscrita(*id);
        escritaVetor(*id);
        saiEscrita(*id);
        sleep(1);
    }

    pthread_exit(NULL);
}


int main(int argc, char *argv[]){

    pthread_t tid[THREADS_DE_LEITURA + THREADS_DE_ESCRITA];
    int id[THREADS_DE_LEITURA + THREADS_DE_ESCRITA];

    //inicializa as variáveis de sincronização
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond_ler, NULL);
    pthread_cond_init(&cond_escr, NULL);


    for(int i=0; i < THREADS_DE_LEITURA; i++){

        id[i] = i + 1;
        if(pthread_create(&tid[i], NULL, leitor, (void*)&id[i])){

            printf("Erro ao executar pthread_create()");
            return 1;
        }
    }

    for(int i=0; i < THREADS_DE_ESCRITA; i++){

        id[i + THREADS_DE_LEITURA] = i + 1;
        if(pthread_create(&tid[i + THREADS_DE_LEITURA], NULL, leitor, (void*)&id[i + THREADS_DE_LEITURA])){

            printf("Erro ao executar pthread_create()");
            return 1;
        }
    }

    pthread_exit(NULL);
    return 0;
}