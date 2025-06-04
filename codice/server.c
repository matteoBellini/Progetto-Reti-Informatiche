#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include "include/parametri.h"
#include "include/quiz.h"
#include "include/db.h"
#include "include/utils.h"
#include "include/gioco.h"

int server_fd;

//ridefinizione della gestione dei segnali SIGHUP e SIGINT in modo che il socket del server venga chiuso
void signal_handler(int sig)
{
    if(sig == SIGHUP || sig == SIGINT){
        printf("Ricevuto segnale: %s\n", (sig == SIGHUP ? "SIGHUP" : "SIGINT"));
        if(server_fd != -1){
            close(server_fd);
        }
        exit(EXIT_SUCCESS);
    }
}

int main()
{
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct quiz q;
    struct gioco g;

    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);

    //carico in memoria le informazioni dei quiz disponibili
    parse(&q);

    //inizializzazione struttura dati gioco utilizzata dai thread che gestiscono i giocatori connessi
    g.q = &q;
    g.u = (struct utenti *)malloc(sizeof(struct utenti));
    checkMalloc(g.u);
    g.u->root = NULL;
    g.u->numUtenti = 0;
    if(pthread_mutex_init(&g.u->mutex, NULL)){
        perror("Errore nell'inizializzazione del mutex (utenti)");
        exit(EXIT_FAILURE);
    }
    g.classifica = (struct punteggio_tema *)malloc(g.q->numero_temi * sizeof(struct punteggio_tema));
    checkMalloc(g.classifica);
    for(int i = 0; i < g.q->numero_temi; i++){
        g.classifica[i].p = NULL;
        if(pthread_mutex_init(&g.classifica[i].mutex, NULL)){
            perror("Errore nell'inizializzazione del mutex (classifica)");
            exit(EXIT_FAILURE);
        }
    }

    stampaInformazioniServer(&g);

    //creazione socket del server
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        perror("Errore nella bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, BACKLOG) == -1){
        perror("Errore nella listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while(1){
        if((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) == -1){
            perror("Errore nella accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        //creo il thread che si occupa di gestire il nuovo giocatore connesso
        pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
        checkMalloc(thread);

        //argomento da passare al thread contenente tutte le informazioni di cui questo ha bisogno
        struct thread_giocatore_arg *arg = (struct thread_giocatore_arg *)malloc(sizeof(struct thread_giocatore_arg));
        checkMalloc(arg);

        arg->g = &g;
        arg->client_fd = client_fd;

        pthread_create(thread, NULL, thread_giocatore, (void *)arg);
    }

    return 0;
}