/** @file functions.h
 
	@brief Libreria delle funzioni di utilità.
 
	Contiene le definizioni per la libreria functions.
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define SHMKEY 75 	/**< Chiave del buffer condiviso */
#define EMPTYKEY 65	/**< Chiave del vettore di semafori empty */
#define FULLKEY 66 	/**< Chiave del vettore di semafori full */

struct sembuf wait_b;	/**< Struttura dati per l'esecuzione dell'operazione wait su un semaforo */
struct sembuf signal_b;	/**< Struttura dati per l'esecuzione dell'operazione signal su un semaforo */
int shmid;				/**< Identificatore della memoria condivisa */
int semid_empty; 		/**< Identificatore del vettore di semafori empty */
int semid_full;			/**< Identificatore del vettore di semafori full */
char sprintf_buffer[128];/**< Buffer per la chiamata a funzione sprintf().*/

/**
	Struttura dati utilizzata nella comunicazione tra padre e figli
 */
typedef struct operazione {
    int id_sem;				/**< Identificatore del processo a cui far eseguire l'operazione */
    int num1; 				/**< Primo operando */
    char op; 				/**< Operatore:<br> + somma<br> - differenza<br> * prodotto<br> / divisione<br> */
    int num2; 				/**< Secondo operando */
    int res;				/**< Risultato calcolato dal processo figlio */
    bool res_disponibile;	/**< Flag che indica la presenza o meno di risultati da trascrivere al padre */
} dati;

/**
	Struttura dati utilizzata per il salvataggio dei risultati
 */
typedef struct risultati{
    int n1;			/**< Operando */
    char op;		/**< Operatore */
    int n2;			/**< Operando */
    int res;		/**< Risultato */
} res;

/**
 *	@brief Procedura per l'esecuzione di una signal su un semaforo
 *
 *	Esegue una signal sul semaforo indicato: ne incrementa il valore di una unità
 *  @param semid Id del vettore di semafori
 *	@param num N° del semaforo su cui eseguire l'operazione
 */
void sem_signal(int semid, int num);

/**
 *	@brief Procedura per l'esecuzione di una wait su un semaforo
 *
 *	Esegue una wait sul semaforo indicato: ne decrementa il valore
 *	di una unità se positivo, altrimenti attende bloccandosi.
 *  @param semid Id del vettore di semafori
 *	@param num N° del semaforo su cui eseguire l'operazione
 */
void sem_wait(int semid, int num);

/**
 *  @brief Funzione per la stampa di un messaggio a video.
 *
 *  Chiama la funzione write per eseguire la stampa su STDOUT (fd = 1).<br>
 *	La lunghezza della stringa è ricavata con la funzione strlen() della libreria string.h.<br>
 *	@param fd File descriptor del file su cui scrivere.
 *  @param msg Stringa contenente il messaggio da stampare a video.
 */
void my_write(int fd, char* msg);

/**
	@brief Funzione per la lettura di una linea del file fd.
 
	@param fd File descriptor del file su cui effettuare la lettura.
	@return Stringa contenente l'ultima riga letta.
 */
char* leggi_linea(int fd);

/**
	@brief Funzione che esegue la divisione in token di una stringa
	passata come parametro.
 
	@param line Stringa letta dalla funzione leggi_linea dal file di configurazione
	che deve essere tokenizzata.
	@return Struttura dati contenente le variabili da passare alla routine del
	processo figlio per l'esecuzione dei calcoli.
 */
dati* tokenize(char* line);

/**
	@brief Funzione che libera lo spazio allocato alle risorse create
 
	@param buffer Struttura dati condivisa
	@param shmid Identificatore della memoria condivisa
	@param semid_e Identificatore del vettore di semafori empty
	@param semid_f Identificatore del vettore di semafori full
 */
void free_resources(dati* buffer, int shmid, int semid_e, int semid_f);

#endif