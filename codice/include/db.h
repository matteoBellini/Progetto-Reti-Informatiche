#include "parametri.h"
#include <pthread.h>
#include <stdbool.h>

#ifndef DB_H
#define DB_H

//struttura dati utilizzata per memorizzare il punteggio di un utente
struct punteggio
{
    char nome[MAX_INPUT_LENGTH];     //nome con cui si è registrato l'utente
    int punti;                      //punteggio ottenuto per il tema
    struct punteggio *next;         //puntatore al prossimo punteggio
};

//struttura dati utilizzata per memorizzare i punteggi degli utenti che hanno partecipato ad un tema
struct punteggio_tema
{
    struct punteggio *p;        //lista di punteggi
    pthread_mutex_t mutex;      //semaforo utilizzato per garantire l'accesso in mutua esclusione alla lista
};

//struttura dati che contiene le informazioni dell'utente
struct utente
{
    char username[MAX_INPUT_LENGTH];
    bool *temi_completati;
    bool *temi_giocati;
    bool endquiz;
    struct utente *next;
};

//struttura dati che contiene la lista degli utenti registrati
struct utenti
{
    struct utente *root;    //utenti registrati organizzati sotto forma di lista
    int numUtenti;
    pthread_mutex_t mutex;  //necessario per gestire la mutua esclusione
};

struct gioco;

bool cercaUtente(struct utente *, char *);     //verifica se è già registrato un utente con l'username fornito
bool inserimentoUtente(struct utente **, char *);    //aggiunta di un nuovo utente
void stampaUtenti(struct utente *);                  //stampa la lista di utenti attualmente connessi
bool temaDisponibile(bool *, int);              //verifica se sono presenti temi non giocati
void aggiungiPunteggio(struct punteggio **, char *);  //aggiunge un nuovo punteggio alla lista dei punteggi di un tema
void incrementaPunteggio(struct punteggio **, char *);   //incrementa il punteggio di un utente in un tema
void rimuoviUtente(struct gioco *, struct utente *);                           //rimuove un utente dalle classifiche e dalle liste dei temi completati

#endif