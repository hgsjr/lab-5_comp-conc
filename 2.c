#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<pthread.h>

#define THREADS_DE_LEITURA 2 // quantidade de threads de leitura
#define THREADS_DE_ESCRITA 2 // quantidade de threads de escrita

int vet[5]; // vet de 5 elementos para uma melhor exibição
int ler = 0, escr = 0; // variáveis de estado (variam entre zero e um)
int filaEscrita = 0; // conta quantos escritores estão esperando


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
    printf("Media de vet = %f\n", media);
}

// função que faz a escr em vet
void escritaVetor(int id){

    vet[0] = id;
    vet[4] = id;
    for(int i=1; i<4; i++){

        vet[i] = 2 * id;
    }
    printf("E[%d] - vet modificado", id);
    for(int i=0; i<5; i++) // imprime vet após a ler

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
    while(filaEscrita > 0 || escr > 0){ // se existir algum escritor na fila ou algum escreve, a ler não começa

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
    if(filaEscrita > 0) // se existir alguma thread de escr bloqueada, ela é liberada

        pthread_cond_signal(&cond_escr);

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
    while((ler > 0) || (escr > 0)){ // caso exista alguma ler ou escr ativada, a thread é bloqueada

        printf("E[%d] entrou para a fila de escr\n", id);
        filaEscrita++;
        pthread_cond_wait(&cond_escr, &lock);
        filaEscrita--;
        printf("E[%d] desbloqueou\n", id);
    }
    escr++;
    pthread_mutex_unlock(&lock);
}

void saiEscrita(int id){

    pthread_mutex_lock(&lock);
    printf("E[%d] terminou de escrever\n", id);
    escr--;
    if(filaEscrita > 0) // se existirem escritores bloqueados libera apenas um escritor

        pthread_cond_signal(&cond_escr); //sinaliza um escritor
    else // só desbloqueia as leitoras se não existir nenhum escritor bloqueado

        pthread_cond_broadcast(&cond_ler); // desbloqueia todas as leitoras
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

    //inicializa as variaveis de sincronizacao
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