#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include "include/parametri.h"
#include "include/utils.h"

struct tema
{
    char nome[MAX_INPUT_LENGTH];
    bool giocato;
};

struct temi
{
    int numTemi;
    struct tema *t;
};

int client_fd;

//ridefinizione della gestione dei segnali SIGHUP e SIGINT in modo che il socket del server venga chiuso
void signal_handler(int sig)
{
    if(sig == SIGHUP || sig == SIGINT){
        printf("Ricevuto segnale: %s\n", (sig == SIGHUP ? "SIGHUP" : "SIGINT"));
        if(client_fd != -1){
            close(client_fd);
        }
        exit(EXIT_SUCCESS);
    }
}

//funzione che effettua la gestione di eventuali errori dovuti ad una send()
void gestoreErroriSend(int ret, int len, int client_fd)
{
    if(ret == -1 && (errno == ECONNRESET || errno == EPIPE)){   //il server di è disconnesso
        printf("Il server si è disconnesso.\n");
        close(client_fd);
        exit(EXIT_FAILURE);     //termino il processo
    } else if(ret != len){
        //si è verificato un errore nell'invio
        perror("Errore durante l'invio del messaggio");
        close(client_fd);
        exit(EXIT_FAILURE);     //termino il processo
    }
}

//funzione che effettua la gestione di eventuali errori dovuti ad una recv()
void gestoreErroriRecv(int ret, int len, int client_fd)
{
    if(ret == 0 || (ret == -1 && errno == ECONNRESET)){       //il server ha chiuso la connessione
        printf("Il server si è disconnesso.\n");
        close(client_fd);
        exit(EXIT_FAILURE);     //termino il processo
    } else if(ret != len){
        //si è verificato un errore nella ricezione
        perror("Errore nella ricezione.");
        close(client_fd);
        exit(EXIT_FAILURE);     //termino il processo
    }
}

//funzione che invia tramite il socket client_fd il contenuto di buf ed effettua la gestione di eventuali errori
void invia(int client_fd, char *buf)
{
    int ret;
    int numBytes = strlen(buf);
    
    numBytes = htonl(numBytes);
    ret = send(client_fd, &numBytes, sizeof(numBytes), MSG_NOSIGNAL);
    gestoreErroriSend(ret, sizeof(numBytes), client_fd);

    numBytes = ntohl(numBytes);
    ret = send(client_fd, buf, numBytes, MSG_NOSIGNAL);
    gestoreErroriSend(ret, numBytes, client_fd);
}

//funzione che riceve una stringa dal socket client_fd e ne inserisce il contenuto all'interno di buf. Vengono gestiti evetnuali errori di ricezione
void ricevi(int client_fd, char *buf)
{
    int ret, numbytes;

    ret = recv(client_fd, &numbytes, sizeof(numbytes), 0);
    gestoreErroriRecv(ret, sizeof(numbytes), client_fd);

    numbytes = ntohl(numbytes);
    ret = recv(client_fd, buf, numbytes, 0);
    gestoreErroriRecv(ret, numbytes, client_fd);

    buf[numbytes] = '\0';       //aggiunta del terminatore di stringa in fondo al messaggio
}

//funzione che effettua la gestione dei comandi 'show score' ed 'endquiz' lanciati dal client
int gestoreComandi(int client_fd, char *bufComando)
{
    char buf[MAX_LENGTH];
    if(strcmp(bufComando, "show score") == 0){  //gestione comando 'show score'
        invia(client_fd, bufComando);

        ricevi(client_fd, buf);

        printf("%s\n", buf);

        return 1;
    } else if(strcmp(bufComando, "endquiz") == 0){  //gestione comando 'endquiz'
        invia(client_fd, bufComando);

        return 2;
    }

    return 0;
}

//funzione che restituisce TRUE se esiste almeno un tema non giocato, FALSE altrimenti
bool temiDisponibili(struct tema *t, int dim)
{
    for(int i = 0; i < dim; i++){
        if(!t[i].giocato)
            return true;    //c'è almeno un tema 
    }

    return false;   //tutti i temi disponibili sono già stati giocati
}

//funzione utilizzata per effettuare gli inserimenti delle risposte del client, vengono gestiti i casi input vuoto e input che supera la dimensione del buffer
void inserimento(char *buf, int lenght)
{
    while(1){
        if(fgets(buf, lenght, stdin) != NULL){
            int len = strlen(buf);
            if(strcmp(buf, "\n") == 0){     //caso in cui si è inserita una stringa vuota
                continue;
            } else if(buf[len-1] == '\n'){ //la stringa inserita è dimensionalmente corretta
                buf[len-1] = '\0';
                break;
            } else {    //la stringa inserita ha dimensione maggiore del buffer
                buf[lenght-1] = '\0';   //aggiungo il terminatore di stringa nell'ultima posizione del buffer
                int ch;
                while((ch = getchar()) != '\n' && ch != EOF);
                break;
            }
        }
    }
}


int main(int argc, char* argv[])
{
    int porta;
    struct sockaddr_in server_addr;
    int sceltaMenu;
    char buf[MAX_LENGTH] = {0};
    int ret;
    int numBytes;
    struct temi *temi;

    //controllo che il client sia stato avviato correttamente
    if(argc != 2){
        printf("Il numero di argomenti inseriti non è valido. Utilizzare ./client <porta>\n");
        exit(EXIT_FAILURE);
    } else {
        porta = atoi(argv[1]);
    }

    //gestione dei segnali SIGHUP e SIGINT per chiudere il socket se questo è stato aperto in modo da evitare errori
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);

    endquiz:;

    while(1){
        //stampa del menù iniziale
        system("clear");
        printf("Trivia Quiz\n");
        printf("+++++++++++++++++++++++++\n");
        printf("Menù:\n");
        printf("1 - Comincia una sessione di Trivia\n");
        printf("2 - Esci\n");
        printf("+++++++++++++++++++++++++\n");
        printf("La tua scelta: ");
        //l'utente sceglie una tra le opzioni disponibili

        inserimento(buf, MAX_INPUT_LENGTH);
        sceltaMenu = atoi(buf);

        if(sceltaMenu == 1 || sceltaMenu == 2){
            break;
        } else {
            printf("\nScegli una tra le opzioni disponibili\n\n");
        }
    }

    //l'utente ha scelto di uscire
    if(sceltaMenu == 2){
        exit(EXIT_SUCCESS);
    }

    //l'utente vuole giocare
    server_addr.sin_family = AF_INET;
    //conversione indirizzo del server da formato presentazione a network
    if(inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr.s_addr) <= 0){
        perror("Il formato dell'indirizzo del server non è corretto");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_port = htons(porta);

    if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    //connessione al server
    if(connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        perror("Errore nella connessione al server");
        close(client_fd);
        exit(EXIT_FAILURE);
    }


    //REGISTRAZIONE UTENTE
    while(1){
        ricevi(client_fd, buf);

        //controllo se la registrazione è andata a buon fine
        if(strcmp(buf, "OK") == 0){
            break;
        }

        printf("%s", buf);    //stampa del messaggio ricevuto
        inserimento(buf, MAX_INPUT_LENGTH);  //l'utente inserisce la risposta al messaggio

        invia(client_fd, buf);
    }

    //RICEZIONE LISTA DEI TEMI DISPONIBILI
    temi = (struct temi *)malloc(sizeof(struct temi));
    temi->numTemi = 0;
    ret = recv(client_fd, &temi->numTemi, sizeof(numBytes), 0);     //ricevo il numero di temi disponibili
    gestoreErroriRecv(ret, sizeof(numBytes), client_fd);

    temi->numTemi = ntohl(temi->numTemi);
    temi->t = (struct tema *)malloc(temi->numTemi * sizeof(struct tema));   //alloco lo spazio necessario a memorizzare i temi disponibili
    checkMalloc(temi->t);
    memset(temi->t, 0, temi->numTemi * sizeof(struct tema));

    //ottengo i nomi dei temi disponibili dal server
    for(int i = 0; i < temi->numTemi; i++){
        ricevi(client_fd, temi->t[i].nome);
    }

    //SCELTA TEMA E RISPOSTA ALLE DOMANDE
    //si esce quando non ci sono più temi oppure se il client digita il comando endquiz
    do {
        int temaScelto = 0;
        system("clear");
        printf("Quiz disponibili\n");
        printf("+++++++++++++++++++++++++++++++++++\n");
        for(int i = 0; i < temi->numTemi; i++){
            if(!temi->t[i].giocato)     //propongo il tema solo se il giocatore non lo ha già giocato
                printf("%d - %s\n", i+1, temi->t[i].nome);
        }
        printf("+++++++++++++++++++++++++++++++++++");

        //scelta del tema
        while(true){
            printf("\nLa tua scelta: ");
            inserimento(buf, MAX_INPUT_LENGTH);

            ret = gestoreComandi(client_fd, buf);

            if(ret == 1){       //è stato digitato il comando 'show score'
                continue;
            } else if(ret == 2){    //è stato digitato il comando 'endquiz'
                close(client_fd);

                //dealloco lo spazio allocato precedentemente
                free(temi->t);
                free(temi);

                goto endquiz;
            } else if((temaScelto = atoi(buf)) <= 0 || temaScelto > temi->numTemi || temi->t[temaScelto-1].giocato){     //gestione caso numero tema non valido
                printf("\nNumero tema non valido\n");
            } else {
                //comunico al server il tema scelto
                snprintf(buf, MAX_LENGTH, "%d", temaScelto-1);
                invia(client_fd, buf);

                break;
            }
        }

        temi->t[temaScelto - 1].giocato = true;
        printf("\nQuiz - %s\n", temi->t[temaScelto -1].nome);

        //RICEZIONE DOMANDA E INVIO RISPOSTA
        //si esce al termine delle domande o se il client digita il comando endquiz
        int contatore = 0;
        while(contatore < NUM_DOMANDE){
            char bufIns[MAX_INPUT_LENGTH] = {0};     //buffer utilizzato per contenere ciò che l'utente digita
            printf("+++++++++++++++++++++++++++++++++++\n");
            //ricezione domanda
            ricevi(client_fd, buf);

            while(true){
                printf("%s\n", buf);      //stampa della domanda
                printf("\nRisposta: ");
                inserimento(bufIns, MAX_INPUT_LENGTH);   //inserimento della risposta o di un comando

                ret = gestoreComandi(client_fd, bufIns);

                if(ret == 1){   //è stato digitato il comando 'show score'
                    continue;
                } else if(ret == 2){    //è stato digitato il comando 'endquiz'
                    close(client_fd);
                    goto endquiz;
                } else {    //è stata digitata la risposta alla domanda
                    break;
                }
            }

            //invio risposta
            invia(client_fd, bufIns);

            //ricezione esito della risposta
            ricevi(client_fd, buf);
            printf("%s\n", buf);    //stampa esito ricevuto

            contatore++;
        }
    } while(temiDisponibili(temi->t, temi->numTemi));

    //TUTTI I QUIZ SONO STATI COMPLETATI
    while(true){
        printf("Hai terminato tutti i quiz disponibili. Puoi continuare a visualizzare i punteggi con il comando 'show score' oppure puoi terminare la sessione con il comando 'endquiz'\n\nLa tua scelta: ");
        inserimento(buf, MAX_INPUT_LENGTH);

        ret = gestoreComandi(client_fd, buf);

        if(ret == 1)
            continue;
        else if(ret == 2){
            close(client_fd);

            //dealloco lo spazio allocato precendentemente
            free(temi->t);
            free(temi);

            goto endquiz;
        } else
            printf("Comando non valido\n");
    }

    return 0;
}