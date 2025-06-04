#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "../include/utils.h"

//funzione che controlla che l'operazione di malloc() sia andata a buon fine
void checkMalloc(void *ptr)
{
    if(ptr == NULL){
        free(ptr);
        perror("Si è verificato un errore nella malloc()");
        exit(EXIT_FAILURE);
    }
}

//funzione che trasforma le lettere maiuscole di una stringa in minuscole
void toLowercase(char *string)
{
    while(*string != '\0'){
        *string = tolower(*string);
        string++;
    }
}