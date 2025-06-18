#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <math.h>

// Variáveis globais
sem_t slotCheio, slotVazio;  // semáforos para sincronização por condição
sem_t mutexGeral; // semáforo geral de sincronização por exclusão mútua

int *Buffer, M, N, C;

int numeros_gerados = 0;
int primos_encontrados = 0;
int *contagem_primos_por_thread;
int thread_vencedora = 0;

int ehPrimo(long long int n) {
    int i;
    if (n <= 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (i = 3; i < sqrt(n) + 1; i += 2)
        if (n % i == 0) return 0;
    return 1;
}

// Função para inserir um elemento no buffer
void Insere(int item, int id) {
    static int in = 0;
    sem_wait(&slotVazio); // aguarda slot vazio para inserir
    sem_wait(&mutexGeral); //exclusao mutua entre produtores
    Buffer[in] = item;
    in = (in + 1) % M;
    numeros_gerados++;
    printf("Produtor: inseriu %d (total: %d/%d)\n", item, numeros_gerados, N);
    sem_post(&mutexGeral);
    sem_post(&slotCheio); // sinaliza um slot cheio
}

// Função para retirar um elemento do buffer
int Retira(int id) {
    int item;
    static int out = 0;
    sem_wait(&slotCheio); // aguarda slot cheio para retirar
    sem_wait(&mutexGeral); //exclusao mutua entre consumidores
    item = Buffer[out];
    Buffer[out] = 0;
    out = (out + 1) % M;
    printf("Consumidor[%d]: retirou %d\n", id, item);
    sem_post(&mutexGeral);
    sem_post(&slotVazio); // sinaliza um slot vazio
    return item;
}

void *produtor(void *arg) {
    for (int i = 1; i <= N; i++) {
        Insere(i, 0);
    }
    
    for (int i = 0; i < C; i++) {
        Insere(-1, 0);
    }
    
    pthread_exit(NULL);
}

void *consumidor(void *arg) {
    int id = *(int *)arg;
    free(arg);
    int item;
    int primos_locais = 0;
    while (1) {
        item = Retira(id);
        
        if (item == -1) break;
        
        if (ehPrimo(item)) {
            primos_locais++;
            printf("Consumidor[%d]: %d eh primo!", id, item);
        }
    }
    
    sem_wait(&mutexGeral);
    contagem_primos_por_thread[id-1] = primos_locais;
    primos_encontrados += primos_locais;
    
    if (primos_locais > contagem_primos_por_thread[thread_vencedora]) {
        thread_vencedora = id-1;
    }
    sem_post(&mutexGeral);
    
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Digite ./ex6 <qtd_numeros> <tam_buffer> <qtd_threads>\n");
        return 1;
    }
    
    N = atoi(argv[1]);
    M = atoi(argv[2]);
    C = atoi(argv[3]);
    
    pthread_t tid[C+1];
    Buffer = (int *)malloc(M * sizeof(int));
    contagem_primos_por_thread = (int *) calloc(C, sizeof(int));
    
    sem_init(&mutexGeral, 0, 1);
    sem_init(&slotCheio, 0, 0);
    sem_init(&slotVazio, 0, M);
    
    for (int i = 0; i < M; i++) {
        Buffer[i] = 0;
    }
    
    if (pthread_create(&tid[0], NULL, produtor, NULL)) {
        printf("Erro na criação do thread produtor\n");
        exit(1);
    }
    
    for (int i = 1; i <= C; i++) {
        int *id = (int *)malloc(sizeof(int));
        *id = i;
        if (pthread_create(&tid[i], NULL, consumidor, (void *)id)) {
            printf("Erro na criação do thread consumidor %d\n", i);
            exit(1);
        }
    }
    
    for (int i = 0; i <= C; i++) {
        pthread_join(tid[i], NULL);
    }
    
    printf("Total de primos: %d\n", primos_encontrados);
    printf("Thread vencedora: Consumidor %d (encontrou %d primos)\n", 
           thread_vencedora + 1, contagem_primos_por_thread[thread_vencedora]);
    
    free(Buffer);
    free(contagem_primos_por_thread);
    sem_destroy(&mutexGeral);
    sem_destroy(&slotCheio);
    sem_destroy(&slotVazio);
    
    return 0;
}