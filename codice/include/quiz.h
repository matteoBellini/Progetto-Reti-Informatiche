#include <stdbool.h>
#include "parametri.h"

#ifndef QUIZ_H
#define QUIZ_H

//struttura che rappresenta una risposta ad una domanda; nel caso ci siano più risposte queste vengono gestite come una lista
struct risposta
{
    char valore[MAX_LENGTH];    //testo della risposta
    struct risposta *next;     //putatore alla risposta successiva
};

//struttura che rappresenta una domanda del quiz
struct domanda
{
    char valore[MAX_LENGTH];    //testo della domanda
    struct risposta *risposte;   //risposte alla domanda
};

//struttura che rappresenta un tema del quiz, contiene al suo interno un vettore di domande;
struct tema
{
    char nomeTema[MAX_LENGTH];
    struct domanda domande[NUM_DOMANDE];
};

//struttura che rappresenta il quiz (il numero di temi è dinamico e dipende da quanti ne sono indicati in indice.txt)
struct quiz
{
    int numero_temi;
    struct tema *temi;
};


void parse(struct quiz *);   //effettua il parsing dei vari temi del quiz
void free_quiz(struct quiz *);   //elimina le risorse allocate
bool verificaRisposta(struct risposta *, char *);   //verifica se una risposta è all'interno della lista delle risposte ad una domanda

#endif