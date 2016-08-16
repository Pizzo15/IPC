/** @file functions.c
 
	@brief Libreria delle funzioni di utilità.
 */
#include "functions.h"

/**
 *	@brief Procedura per l'esecuzione di una wait su un semaforo
 *
 *	Esegue una wait sul semaforo indicato: ne decrementa il valore
 *	di una unità se positivo, altrimenti attende bloccandosi.
 *  @param semid Id del vettore di semafori
 *	@param num N° del semaforo su cui eseguire l'operazione
 */
void sem_wait(int semid, int num){
    wait_b.sem_num = num;
    wait_b.sem_op = -1;
    wait_b.sem_flg = 0;
    
    if(semop(semid, &wait_b, 1) == -1){
        sprintf(sprintf_buffer, "ERRORE: Esecuzione wait su semaforo n°%d\n", num);
        my_write(1, sprintf_buffer);
        exit(1);
    }
}

/**
 *	@brief Procedura per l'esecuzione di una signal su un semaforo
 *
 *	Esegue una signal sul semaforo indicato: ne incrementa il valore di una unità
 *  @param semid Id del vettore di semafori
 *	@param num N° del semaforo su cui eseguire l'operazione
 */
void sem_signal(int semid, int num){
    signal_b.sem_num = num;
    signal_b.sem_op = 1;
    signal_b.sem_flg = 0;
    
    if(semop(semid, &signal_b, 1) == -1){
        sprintf(sprintf_buffer, "ERRORE: Esecuzione signal su semaforo n°%d\n", num);
        my_write(1, sprintf_buffer);
        exit(1);
    }
}

/**
	@brief Funzione per la lettura di una linea del file fd.
 
	@param fd File descriptor del file su cui effettuare la lettura.
	@return Stringa contenente l'ultima riga letta.
 */
char* leggi_linea(int fd){
    char* res;	/* Stringa per il salvataggio dei caratteri letti dal buffer */
    char c;	 	/* Buffer per la lettura del file carattere per carattere */
    int i=0;	/* Contatore */
    
    /**
     Alloco lo spazio per la stringa risultato (Max. 128 char).
     */
    res = (char *) malloc(sizeof(char)*128);
    
    /**
     Entro in un ciclo di lettura finchè ci sono caratteri da leggere.
     */
    while(read(fd, &c, 1) > 0){
        /**
         Se il carattere letto è '\n' ho raggiunto la fine della linea.
         Posso terminare il ciclo.
         */
        if(c == '\n')
            break;
        
        /**
         Altrimenti aggiungo nella posizione i-esima di res il carattere
         letto e incremento il contatore.
         */
        *(res+i) = c;
        i++;
    }
    
    /**
     Aggiungo il carattere di terminazione alla fine di res.
     */
    *(res+i+1) = '\0';
    
    return res;
}

/**
	@brief Funzione che esegue la divisione in token di una stringa
	passata come parametro.
 
	@param line Stringa letta dalla funzione leggi_linea dal file di configurazione
	che deve essere tokenizzata.
	@return Struttura dati contenente le variabili da passare alla routine del
	processo figlio per l'esecuzione dei calcoli.
 */
dati* tokenize(char* line){
    dati* res;				/* Struttura dati dove salvare i valori letti da ritornare. */
    int i=0;				/* Contatore */
    int token[4];			/* Vettore contenente i token interi letti dalla funzione strtok */
    char c;					/* Carattere contenente l'operatore letto dalla funzione strtok */
    
    /**
     Alloco lo spazio per la struttura dati res.
     */
    res = (dati *) malloc(sizeof(dati));
    
    /**
     Inizio un ciclo di lettura dei token tramite la funzione strtok().
     La funzione riceve come parametro la stringa da leggere e il carattere
     separatore come divisore tra i vari token.
     */
    token[i++] = atoi(strtok(line, " "));
    
    while(i < 4){
        if(i == 2) {
            c = strtok(NULL, " ")[0];
            i++;
        } else {
            token[i++] = atoi(strtok(NULL, " "));
        }
    }
    
    /**
     Salvo gli argomenti da ritornare nella struttura res.
     */
    res->id_sem = token[0];
    res->num1 = token[1];
    res->op = c;
    res->num2 = token[3];
    
    return res;
}

/**
 *  @brief Funzione per la stampa di un messaggio a video.
 *
 *  Chiama la funzione write per eseguire la stampa su STDOUT (fd = 1) o sul file fd.<br>
 *	La lunghezza della stringa è ricavata con la funzione strlen() della libreria string.h.
 *	@param fd File descriptor del file su cui scrivere.
 *  @param msg Stringa contenente il messaggio da stampare a video.
 */
void my_write(int fd, char* msg){
    write(fd, msg, strlen(msg));
}

/**
	@brief Funzione che libera lo spazio allocato alle risorse create
 
	@param buffer Struttura dati condivisa
	@param shmid Identificatore della memoria condivisa
	@param semid_e Identificatore del vettore di semafori empty
	@param semid_f Identificatore del vettore di semafori full
 */
void free_resources(dati* buffer, int shmid, int semid_e, int semid_f){
    shmdt(buffer);
    
    shmctl(shmid, IPC_RMID, NULL);
    
    semctl(semid_e, 0, IPC_RMID);
    
    semctl(semid_f, 0, IPC_RMID);
}