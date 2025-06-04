#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/quiz.h"
#include "../include/utils.h"

//questa funzione effettua il parsing del file indice.txt e dei file contenenti le domande e le risposte per i vari temi
void parse(struct quiz *q)
{
    FILE *file;
    char line[MAX_LENGTH];
    int index = 0;  //indice del tema

    file = fopen("dati/indice.txt", "r");

    if(file == NULL){
        perror("Errore nell'apertura del file indice.txt");
        exit(EXIT_FAILURE);
    }

    fgets(line, MAX_LENGTH, file);    //lettura del numero di temi presenti

    q->numero_temi = atoi(line);
    q->temi = (struct tema*)malloc(q->numero_temi * sizeof(struct tema));   //alloco lo spazio necessario per memorizzare i vari temi
    checkMalloc(q->temi);
    
    //lettura dei nomi dei temi
    while(fgets(line, MAX_LENGTH, file) != NULL){

        line[strcspn(line, "\n")] = '\0';   //rimozione del carattere /n alla fine della riga
        strcpy(q->temi[index].nomeTema, line);

        index++;
    }

    fclose(file);   //chiusura del file indice.txt

    index = 0;

    //popolo le strutture domande e risposte per tutti i temi presenti
    while(index < q->numero_temi){
        char filepath[MAX_LENGTH];
        int indiceDomanda = 0;

        sprintf(filepath, "dati/%d.txt", index+1); //costruzione del path del file contenente le domande e le risposte del tema il cui numero è contenuto in index

        file = fopen(filepath, "r");

        if(file == NULL){
            printf("Errore nell'apertura del file %d.txt\n", index);
            exit(EXIT_FAILURE);
        }

        while(fgets(line, MAX_LENGTH, file) != NULL && indiceDomanda < NUM_DOMANDE){
            char delim1[2] = "|";   //separatore tra domanda e risposte
            char delim2[2] = "~";   //separatore delle risposte
            char *token;
            char *risposte;

            q->temi[index].domande[indiceDomanda].risposte = NULL;  //inizializzo la lista delle risposte alla domanda

            line[strcspn(line, "\n")] = '\0';   //rimozione del carattere \n dalla linea appena letta

            token = strtok(line, delim1);   //ottengo la domanda dalla linea letta
            
            if(token != NULL){
                strcpy(q->temi[index].domande[indiceDomanda].valore, token);    //la domanda viene copiata all'interno del tema
            }

            risposte = strtok(NULL, delim1);

            if(risposte != NULL){
                token = strtok(risposte, delim2);   //ottengo la prima risposta alla domanda

                //aggiungo tutte le risposte presenti alla domanda
                while(token != NULL){
                    struct risposta *r = (struct risposta*)malloc(sizeof(struct risposta));
                    checkMalloc(r);
                    r->next = q->temi[index].domande[indiceDomanda].risposte;
                    q->temi[index].domande[indiceDomanda].risposte = r;
                    strcpy(r->valore, token);

                    token = strtok(NULL, delim2);   //passo alla prossima risposta
                }
            }

            indiceDomanda++;
        }
        fclose(file);
        index++;
    }
}

//funzione che elimina tutte le risorse che sono state allocate
void free_quiz(struct quiz *q)
{
    if(q == NULL)
        return;

    for(int i = 0; i < q->numero_temi; i++){
        struct tema *t = &q->temi[i];

        for(int j = 0; j < NUM_DOMANDE; j++){
            struct domanda *d = &t->domande[j];
            struct risposta *r = d->risposte;

            //libero lo spazio allocato per le risposte alla domanda;
            while(r != NULL){
                struct risposta *temp = r;
                r = r->next;

                free(temp);
            }
        }
    }

    //libero lo spazio allocato per i temi
    free(q->temi);
    //libero la struttura principale
    free(q);
}

//funzione che verifica se la risposta fornita è tra quelle memorizzate per una domanda. TRUE se la risposta è corretta, FALSE altrimenti
bool verificaRisposta(struct risposta *r, char *risposta)
{
    while(r != NULL){
        if(strcmp(risposta, r->valore) == 0)
            return true;
        r = r->next;
    }

    return false;
}