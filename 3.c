#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<pthread.h>

#define THREADS_DE_LEITURA 2 // quantidade de threads que realizam leitura
#define THREADS_DE_ESCRITA 2 // quantidade de threads que realizam escrita

int vet[5]; // vetor de 5 elementos para uma melhor exibição
int ler = 0, escr = 0; // variáveis de estado (variam entre zero e um)

// variáveis que irão servir como filas
// as filas ativas referenciam a quantidade de escritores ou leitores que serão executados em certo momento
// as filas de espera referenciam a quantidade de escritores ou leitores esperando para serem executados
int filaAtivaEscrita = 0, filaEsperaEscrita = 0;
int filaAtivaLeitura, filaEsperaLeitura;


// variáveis de sincronização
pthread_mutex_t lock;
pthread_cond_t cond_ler, cond_escr;

// função que imprime vet e calcula sua média
void leituraVetor(){

    float media = 0;
    printf("Vetor");
    for(int i=0; i<5; i++){

        printf("%d ", vet[i]);
        media += vet[i];
    }
    printf("\n");
    printf("Media do vet = %f\n", media);
}

// função que faz a escrita em vet
void escritaVetor(int id){

    vet[0] = id;
    vet[4] = id;
    for(int i=1; i<4; i++){

        vet[i] = 2 * id;
    }
    printf("E[%d] - vet modificado", id);
    for(int i=0; i<5; i++) // imprime vet após a realização da leitura

        printf("%d ", vet[i]);
    printf("\n");
}

// função que inicializa vet com zeros
void inicializaVetor(){

    for(int i=0; i<5; i++)

        vet[i] = 0;
}

// função que inicia a leitura
void entraLeitura(int id){

    pthread_mutex_lock(&lock);
    printf("L[%d] quer ler", id);
    // caso exista alguma escrita escrevendo ou uma fila ativa de leitores
    // ou escritores, a thread leitora vai se bloquear e entrar para a fila de espera
    while(escr > 0 || filaAtivaLeitura > 0 || filaAtivaEscrita > 0){

        printf("L[%d] entrou para a fila de espera\n", id);
        filaEsperaLeitura++;
        pthread_cond_wait(&cond_ler, &lock);
        printf("L[%d] desbloqueou\n", id);
    }
    ler++;
    pthread_mutex_unlock(&lock);
}

// função que termina a ler
void saiLeitura(int id){

    pthread_mutex_lock(&lock);
    ler--;
    printf("L[%d] terminou de ler\n", id);
    if(filaAtivaLeitura > 1) // verifica se ainda existem leitoras na fila ativa, caso haja, decrementa a fila

        filaAtivaLeitura--;
    else if(filaEsperaEscrita > 0){ // se a thread for a última leitora da fila ativa, aé realizada a mudança na execução para a fila ativa de escritoras

        printf("Iniciando fila de Escritas\n");
        filaAtivaEscrita = filaEsperaEscrita;
        filaEsperaEscrita = 0;
        pthread_cond_signal(&cond_escr);
    }
    else{ // se não tiverem escritores em espera libera escritoras na fila de espera

        pthread_cond_broadcast(&cond_ler);
        filaEsperaLeitura = 0;
    }
    pthread_mutex_unlock(&lock);
}

// thread leitora
void *leitor(void *args){

    int *id = (int*)args;
    while(1){

        entraLeitura(*id);
        leituraVetor();
        saiLeitura(*id);
        sleep(1);
    }

    pthread_exit(NULL);
}

void entraEscrita(int id){

    pthread_mutex_lock(&lock);
    printf("E[%d] quer escrever\n", id);
    // caso exista alguma leitora ou escritora ativa ou uma fila ativa de escritoras ou leitoras, a thread será bloqueada
    while((ler > 0) || (escr > 0) || (filaAtivaEscrita > 0) || (filaAtivaLeitura > 0)){

        printf("E[%d] entrou para a fila de espera\n", id);
        filaEsperaEscrita++;
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


    if(filaAtivaEscrita > 1){ // caso ainda existam threads na fila ativa, desbloquear a próxima e liberar um espaço

        filaAtivaEscrita--;
        pthread_cond_signal(&cond_escr);
    }
    else if(filaEsperaLeitura > 0){ // se essa for a última thread escritora e existir uma fila de espera de leitoras, chavear para ler

        filaAtivaLeitura = filaEsperaLeitura;
        filaEsperaLeitura = 0;
        pthread_cond_broadcast(&cond_ler); // como leitoras pode executar ao mesmo tempo, liberar todas as leitoras
    }
    else{ // caso a fila ativa termine e não existam leitoras esperando liberar uma escrita

        pthread_cond_signal(&cond_escr);
        filaEsperaEscrita--;
    }
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