#include <stdio.h>
#include <pthread.h>
#include "quiz.h"
#include "db.h"

#ifndef GIOCO_H
#define GIOCO_H

//struttura dati contenente le informazioni sui quiz disponibili e sui giocatori attualmente connessi e le classifiche per i vari temi
struct gioco
{
    struct quiz *q;         //temi, domande, risposte
    struct utenti *u;       //lista utenti registrati al gioco
    struct punteggio_tema *classifica;  //classifica per ogni tema
};

//struttura dati che contiene le informazioni necessarie al thread che gestisce un giocatore connesso
struct thread_giocatore_arg
{
    struct gioco *g;
    int client_fd;      //descrittore del socket del giocatore
};

void gestoreErroriSend(int, int, int, struct gioco *, struct utente *);     //gestisce eventuali errori nella send()
void gestoreErroriRecv(int, int, int, struct gioco *, struct utente *);     //gestisce eventuali errori nella recv()
void invia(int, char *, struct gioco *, struct utente *);                   //invia una stringa
void ricevi(int, char *, struct gioco *, struct utente *);                  //riceve una stringa
void stampaInformazioniServer(struct gioco *);                              //stampa le informazioni relative ai client connessi
void * thread_giocatore(void *);                                            //thread che gestisce un giocatore connesso
void stampaClassificaTema(struct punteggio *);                              //stampa la classifica di punteggi per un tema
void stampaTemaCompletato(struct utente *, int);                            //stampa la lista di utenti che hanno completato un tema
int gestoreComandi(int, char *, struct gioco *, struct utente *);

#endif