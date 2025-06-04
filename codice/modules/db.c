#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/utils.h"
#include "../include/db.h"
#include "../include/gioco.h"

//funzione che verifica se c'è già un utente registrato con l'username fornito. TRUE se l'username viene trovato, FALSE altrimenti
bool cercaUtente(struct utente *root, char *username)
{
    struct utente *tmp = root;
    while(tmp != NULL){
        if(strcmp(tmp->username, username) == 0)
            return true;
        else
            tmp = tmp->next;
    }

    return false;
}

/*funzione che permette di registrare un nuovo utente con il suo username.
La registrazione va a buon fine solo se l'username scelto non è già in uso*/
bool inserimentoUtente(struct utente **root, char *username)
{
    //caso in cui non ci sono utenti registrati
    if(*root == NULL){
        struct utente *new = (struct utente *)malloc(sizeof(struct utente));
        checkMalloc(new);
        strcpy(new->username, username);
        new->endquiz = false;
        new->next = NULL;
        *root = new;
        return true;
    }

    //ci sono già utenti registrati
    //verifico che non ci sia già un utente con questo username
    if(cercaUtente(*root, username)){
        return false;
    }

    struct utente *new = (struct utente *)malloc(sizeof(struct utente));
    checkMalloc(new);
    strcpy(new->username, username);
    new->endquiz = false;
    new->next = *root;
    *root = new;
    return true;
}

//funzione che permette di stamapare la lista di utenti attualmente connessi
void stampaUtenti(struct utente *root){
    struct utente *tmp = root;

    while(tmp != NULL){
        if(!tmp->endquiz)   //l'utente viene visualizzato solo se è attivo
            printf("- %s\n", tmp->username);
        tmp = tmp->next;
    }
}

//funzione che verifica se sono presenti temi non ancora giocati
bool temaDisponibile(bool *temi_giocati, int dim)
{
    for(int i = 0; i < dim; i++){
        if(!temi_giocati[i])
            return true;
    }

    return false;
}

//funzione che aggiunge un nuovo punteggio alla lista dei punteggi di un tema. Il punteggio inizialmente sarà 0 (inserimento in fondo)
void aggiungiPunteggio(struct punteggio **p, char *usr)
{
    struct punteggio *punt = (struct punteggio *)malloc(sizeof(struct punteggio));
    checkMalloc(punt);

    strcpy(punt->nome, usr);
    punt->punti = 0;
    punt->next = NULL;

    if(*p == NULL){
        *p = punt;
        return;
    }

    struct punteggio *temp = *p;
    while(temp->next != NULL){
        temp = temp->next;
    }

    temp->next = punt;
}

//funzione che incrementa il punteggio di un utente per un tema e riordina la lista dei punteggi in ordine decrescente
void incrementaPunteggio(struct punteggio **p, char *usr)
{
    if(*p == NULL)      //la lista fornita è vuota
        return;

    struct punteggio *curr = *p;
    struct punteggio *prev = NULL;
    struct punteggio *target = NULL;

    //ricerca del punteggio dell'utente all'interno della lista
    while(curr != NULL){
        if(strcmp(curr->nome, usr) == 0){
            target = curr;
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    if(target == NULL)  //caso utente non trovato
        return;

    target->punti++;

    if(prev == NULL){   //l'utente aveva già il punteggio massimo (non devo riordinare)
        return;
    } else {
        prev->next = target->next;     //rimuovo il nodo corrispondente all'utente dalla lista
    }

    prev = NULL;
    curr = *p;

    while(curr != NULL && curr->punti >= target->punti){    //cerco la posizione dove reinserire il nodo
        prev = curr;
        curr = curr->next;
    }

    //inserisco il nodo in modo che la lista sia ordinata per punteggi decrescenti
    if(prev == NULL){       //l'utente è diventato il primo in classifica
        target->next = *p;
        *p = target;
    } else {
        target->next = curr;
        prev->next = target;
    }
}

//funzione che effettua la rimozione di un utente dalle classifiche e dalle liste dei temi completati
void rimuoviUtente(struct gioco *g, struct utente *usr)
{
    if(g == NULL || usr == NULL)
        return;

    pthread_mutex_lock(&g->u->mutex);
    g->u->numUtenti--;
    pthread_mutex_unlock(&g->u->mutex);

    memset(usr->temi_completati, 0, g->q->numero_temi * sizeof(bool));  //rimuovo il giocatore dai temi completati
    usr->endquiz = true;    //l'utente viene considerato come inattivo (non viene più visualizzato: nei client connessi, nelle classifiche e nei temi completati)

    //rimozione del giocatore dalle classifiche dei temi giocati
    for(int i = 0; i < g->q->numero_temi; i++){
        if(usr->temi_giocati[i]){
            struct punteggio_tema *pt;
            pt = &g->classifica[i];
            pthread_mutex_lock(&pt->mutex);
            if(pt->p != NULL){
                struct punteggio *prev = NULL;
                struct punteggio *curr = pt->p;
                while(strcmp(curr->nome, usr->username) != 0){
                    prev = curr;
                    curr = curr->next;
                }

                if(prev == NULL){   //il giocatore era in cima alla classifica
                    g->classifica[i].p = curr->next;
                } else {
                    prev->next = curr->next;
                }
                free(curr);     //libero la memoria allocata precedentemente
            }
            pthread_mutex_unlock(&pt->mutex);
        }
    }
}