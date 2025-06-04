#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include "../include/utils.h"
#include "../include/gioco.h"
#include "../include/db.h"

//funzione che effettua la gestione di errori in seguito ad una send()
void gestoreErroriSend(int ret, int len, int client_fd, struct gioco *g, struct utente *usr)
{
    if(ret == -1 && (errno == ECONNRESET || errno == EPIPE)){       //la connessione è stata chiusa dal client
        if(g)       //in caso di chiusura della connessione rimuovo il giocatore
            rimuoviUtente(g, usr);
        stampaInformazioniServer(g);
        printf("Il client %s si è disconnesso.\n", usr->username);
        close(client_fd);
        pthread_exit(NULL);     //termino il thread che si occupava di gestire il client
    } else if(ret != len) {
        //si è verificato un errore nell'invio
        perror("Errore durante l'invio del messaggio");
        close(client_fd);
        exit(EXIT_FAILURE);     //termino il server
    }
}

//funzione che effettua la gestione di errori in seguito a recv()
void gestoreErroriRecv(int ret, int len, int client_fd, struct gioco *g, struct utente *usr)
{
    //il client ha chiuso la connessione
    if(ret == 0 || (ret == -1 && errno == ECONNRESET)){
        if(g)       //in caso di chiusura della connessione rimuovo il giocatore
            rimuoviUtente(g, usr);
        stampaInformazioniServer(g);
        printf("Il client %s si è disconnesso.\n", usr->username);
        close(client_fd);
        pthread_exit(NULL);     //termino il thread che si occupava di gestire il client
    } else if(ret != len){      //altrimenti si è verificato un errore durante la ricezione del messaggio
        perror("Errore nella ricezione del messaggio");
        close(client_fd);
        exit(EXIT_FAILURE);     //termino il server
    }
}

//funzione che invia tramite il socket fd il contenuto di buf ed effettua la gestione di eventuali errori
void invia(int fd, char *buf, struct gioco *g, struct utente *u)
{
    int ret;
    int numBytes = strlen(buf);
    
    numBytes = htonl(numBytes);
    ret = send(fd, &numBytes, sizeof(numBytes), MSG_NOSIGNAL);
    gestoreErroriSend(ret, sizeof(numBytes), fd, g, u);

    numBytes = ntohl(numBytes);
    ret = send(fd, buf, numBytes, MSG_NOSIGNAL);
    gestoreErroriSend(ret, numBytes, fd, g, u);
}

//funzione che riceve una stringa dal socket fd e ne inserisce il contenuto all'interno di buf. Vengono gestiti evetnuali errori di ricezione
void ricevi(int fd, char *buf, struct gioco *g, struct utente *u)
{
    int ret, numbytes;

    ret = recv(fd, &numbytes, sizeof(numbytes), 0);
    gestoreErroriRecv(ret, sizeof(numbytes), fd, g, u);

    numbytes = ntohl(numbytes);
    ret = recv(fd, buf, numbytes, 0);
    gestoreErroriRecv(ret, numbytes, fd, g, u);

    buf[numbytes] = '\0';       //aggiunta del terminatore di stringa in fondo al messaggio
}

//funzione che stampa la classifica dei punteggi per un tema
void stampaClassificaTema(struct punteggio *p)
{
    if(p == NULL){
        printf("---\n");
    } else {
        while(p != NULL){
            printf("- %s %d\n", p->nome, p->punti);
            p = p->next;
        }
    }
}

//funzione che stampa la lista di utenti che hanno risposto a tutte le domande di un tema
void stampaTemaCompletato(struct utente *u, int indiceTema)
{
    bool separatore = true;

    while(u != NULL){
        if(u->temi_completati[indiceTema]){
            printf("- %s\n", u->username);
            separatore = false;
        }
        u = u->next;
    }

    if(separatore){
        printf("---\n");
    }
}

//funzione che effettua la gestione dei comandi 'show score' ed 'endquiz' inviati da un client
int gestoreComandi(int client_fd, char *buf, struct gioco *g, struct utente *usr)
{
    if(strcmp(buf, "show score") == 0){
        char temp[MAX_LENGTH];  //buffer temporaneo
        buf[0] = '\0';          //svuoto il buffer

        //scrivo la classifica all'interno del buffer
        for(int i = 0; i < g->q->numero_temi; i++){
            snprintf(temp, MAX_LENGTH, "Punteggio tema %d\n", i+1);
            strcat(buf, temp);

            struct punteggio *p = g->classifica[i].p;

            pthread_mutex_lock(&g->classifica[i].mutex);
            if(p == NULL)
                strcat(buf, "---\n\n");
            else {
                while(p != NULL){
                    snprintf(temp, MAX_LENGTH, "- %s %d\n", p->nome, p->punti);
                    strcat(buf, temp);
                    p = p->next;
                }
                strcat(buf, "\n");
            }
            pthread_mutex_unlock(&g->classifica[i].mutex);
        }

        //invio la classifica al client
        invia(client_fd, buf, g, usr);
        return 1;
    } else if(strcmp(buf, "endquiz") == 0){
        return 2;
    }

    return 0;
}


/*funzione che effettua la stampa dell informazioni relative al server:
- lista dei temi disponibili
- lista dei giocatori connessi
- punteggio dei giocatori per ogni tema
- quali giocatori hanno completato un determinato tema
*/
void stampaInformazioniServer(struct gioco *g)
{
    system("clear");
    printf("Trivia Quiz\n");
    printf("+++++++++++++++++++++++++++++++++++\n");
    //stampa della lista dei temi presenti
    printf("Temi:\n");
    for(int i = 0; i < g->q->numero_temi; i++){
        printf("%d - %s\n", i+1, g->q->temi[i].nomeTema);
    }
    printf("+++++++++++++++++++++++++++++++++++\n\n");

    //stampa della lista dei giocatori registrati
    pthread_mutex_lock(&g->u->mutex);
    printf("Partecipanti (%d):\n", g->u->numUtenti);
    if(g->u->root == NULL)
        printf("---\n");
    else
        stampaUtenti(g->u->root);
    pthread_mutex_unlock(&g->u->mutex);

    //stampa punteggi temi
    for(int i = 0; i < g->q->numero_temi; i++){
        printf("\nPunteggio Tema %d:\n", i+1);

        pthread_mutex_lock(&g->classifica[i].mutex);
        stampaClassificaTema(g->classifica[i].p);
        pthread_mutex_unlock(&g->classifica[i].mutex);
    }

    //stampa temi completati
    for(int i = 0; i < g->q->numero_temi; i++){
        printf("\nQuiz Tema %i completato:\n", i+1);

        pthread_mutex_lock(&g->u->mutex);
        stampaTemaCompletato(g->u->root, i);
        pthread_mutex_unlock(&g->u->mutex);
    }
}

//funzione contenente il codice eseguito dai thread che gestiscono i giocatori connessi
void * thread_giocatore(void * arg)
{
    //ottengo le informazioni necessarie al thread (socket del client e struttura dati gioco)
    struct thread_giocatore_arg *targ = (struct thread_giocatore_arg *)arg;
    int client_fd = targ->client_fd;
    struct gioco *g = targ->g;

    bool registrato = false;
    char buf[MAX_LENGTH] = {0};
    struct utente *usr = NULL;
    int numBytes;
    int ret;

    free(arg);

    //REGISTRAZIONE DEL GIOCATORE
    while(!registrato){
        strcpy(buf, "Scegli un nickname (deve essere univoco): ");

        invia(client_fd, buf, g, usr);

        ricevi(client_fd, buf, g, usr);     //ricevo la risposta del client

        pthread_mutex_lock(&g->u->mutex);
        registrato = inserimentoUtente(&g->u->root, buf);
        pthread_mutex_unlock(&g->u->mutex);
        if(registrato){
            pthread_mutex_lock(&g->u->mutex);
            g->u->numUtenti++;  //incremento il numero di utenti registrati
            usr = g->u->root;
            pthread_mutex_unlock(&g->u->mutex);

            strcpy(buf, "OK");

            //invio messaggio di corretta registrazione all'utente
            invia(client_fd, buf, g, usr);
        }
    }

    //alloco lo spazio necessario a memorizzare a quali quiz ha giocato l'utente
    usr->temi_completati = (bool *)malloc(g->q->numero_temi * sizeof(bool));
    checkMalloc(usr->temi_completati);
    usr->temi_giocati = (bool *)malloc(g->q->numero_temi * sizeof(bool));
    checkMalloc(usr->temi_giocati);

    //inizialmente il nuovo giocatore non ha partecipato a nessun quiz
    memset(usr->temi_completati, 0, g->q->numero_temi * sizeof(bool));
    memset(usr->temi_giocati, 0, g->q->numero_temi * sizeof(bool));

    stampaInformazioniServer(g);    //stampo la lista dei giocatori connessi aggiornata

    //invio al client il numero dei temi presenti
    numBytes = htonl(g->q->numero_temi);
    ret = send(client_fd, &numBytes, sizeof(numBytes), MSG_NOSIGNAL);
    gestoreErroriSend(ret, sizeof(numBytes), client_fd, g, usr);

    //invio al client la lista dei temi presenti
    for(int i = 0; i < g->q->numero_temi; i++){
        invia(client_fd, g->q->temi[i].nomeTema, g, usr);
    }

    do {
        //ricezione numero del tema scelto
        int temaScelto = 0;
        int contatore = 0;

        while(true){
            ricevi(client_fd, buf, g, usr);

            ret = gestoreComandi(client_fd, buf, g, usr);
            if(ret == 1)    //il client ha inviato il comando 'show score'
                continue;
            else if(ret == 2){  //il client ha inviato il comando 'endquiz'
                rimuoviUtente(g, usr);
                stampaInformazioniServer(g);
                printf("Il client %s ha digitato il comando endquiz\n", usr->username);
                close(client_fd);
                pthread_exit(NULL);
            } else {    //il client ha scelto un tema a cui giocare
                temaScelto = atoi(buf);
                break;
            }
        }

        usr->temi_giocati[temaScelto] = true;

        pthread_mutex_lock(&g->classifica[temaScelto].mutex);
        aggiungiPunteggio(&g->classifica[temaScelto].p, usr->username);
        pthread_mutex_unlock(&g->classifica[temaScelto].mutex);

        stampaInformazioniServer(g);    //il client deve comparire nella lista dei punteggi del tema scelto

        while(contatore < NUM_DOMANDE){
            //invio domanda al client
            strcpy(buf, g->q->temi[temaScelto].domande[contatore].valore);
            invia(client_fd, buf, g, usr);

            while(true){
                //ricezione risposta del client
                ricevi(client_fd, buf, g, usr);

                ret = gestoreComandi(client_fd, buf, g, usr);
                if(ret == 1)        //l'utente ha inviato il comando 'show score'
                    continue;
                else if(ret == 2){  //l'utente ha inviato il comando 'endquiz'
                    rimuoviUtente(g, usr);
                    stampaInformazioniServer(g);
                    printf("Il client %s ha digitato il comando endquiz\n", usr->username);
                    close(client_fd);
                    pthread_exit(NULL);
                } else              //l'utente ha inviato la risposta alla domanda
                    break;
            }

            //verifica risposta
            toLowercase(buf);
            if(verificaRisposta(g->q->temi[temaScelto].domande[contatore].risposte, buf)){
                strcpy(buf, "Risposta Corretta");
                //aggiornamento punteggio utente per il tema, agg. classifica e agg. infoServer
                pthread_mutex_lock(&g->classifica[temaScelto].mutex);
                incrementaPunteggio(&g->classifica[temaScelto].p, usr->username);
                pthread_mutex_unlock(&g->classifica[temaScelto].mutex);

                stampaInformazioniServer(g);
            } else {
                //non è necessario aggiornare il punteggio dell'utente in quanto la risposta data era errata
                strcpy(buf, "Risposta errata"); //TODO si può aggiungere la risposta corretta
            }

            //invio esito verifica
            invia(client_fd, buf, g, usr);

            contatore++;    //passo alla domanda successiva
        }

        if(contatore == NUM_DOMANDE){
            usr->temi_completati[temaScelto] = true;    //il giocatore ha risposto a tutte le domande del tema
            stampaInformazioniServer(g);
        }
    } while(temaDisponibile(usr->temi_giocati, g->q->numero_temi));

    //il client ha terminato tutti i quiz. Può continuare a visualizzare i punteggi con il comando 'show score' o può terminare la sessione con 'endquiz'
    while(true){
        ricevi(client_fd, buf, g, usr);

        ret = gestoreComandi(client_fd, buf, g, usr);

        if(ret == 1)
            continue;
        else{
            rimuoviUtente(g, usr);
            stampaInformazioniServer(g);
            printf("Il client %s ha digitato il comando endquiz.\n", usr->username);
            close(client_fd);
            pthread_exit(NULL);
        }
    }

    close(client_fd);
    pthread_exit(NULL);
}