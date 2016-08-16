/** @file routine.c
 * 	@brief Programma che implementa la routine di calcolo dei processi figli.
 *
 *	Il programma è chiamato per ogni processo figlio creato in elab2 e riceve come argomenti
 *		- il numero del processore associato
 * 		- il numero di processi totali creato
 *
 *	La routine da eseguire:<br>
 *		- Attesa su un semaforo per eseguire un calcolo.<br>
 *		- Ricezione dei dati e dell'operazione da eseguire dal padre<br>
 *			- se ricevo 'K' termino l'esecuzione.<br>
 *		- Esecuzione del calcolo.<br>
 *		- Invio del risultato al padre e segnalazione della terminazione dei calcoli.<br>
 */
#include "functions.h"

int num1_figlio;				/**< Intero per la lettura del primo operando dal buffer */
int num2_figlio; 				/**< Intero per la lettura del secondo operando dal buffer */
char op_figlio;					/**< Char per la lettura dell'operatore dal buffer */
dati* buffer_comune; 			/**< Buffer per la memoria condivisa */
int numero_processi;  			/**< Numero di processi creati dal padre */
int id; 						/**< Id del processo */

int main(int argc, char *argv[]){
    
    /**
     Inizializzo le variabili numero_processi e id con i valori passati come argomento.
     */
    id=atoi(argv[1]);
    numero_processi=atoi(argv[2]);
    
    /**
     *	Recupero il vettore di semafori empty creato dal padre per coordinare
     *	le operazioni tra processi.
     */
    if((semid_empty = semget(EMPTYKEY, numero_processi, 0777)) == -1){
        my_write(1, "						ERRORE: Recupero del vettore di semafori emptys\n");
        exit(1);
    }
    
    /**
     *	Recupero il vettore di semafori full creato dal padre per coordinare
     *	le operazioni tra processi.
     */
    if((semid_full = semget(FULLKEY, numero_processi, 0777)) == -1){
        my_write(1, "						ERRORE: Recupero del vettore di semafori full\n");
        exit(1);
    }
    
    /**
     *	Recupero il segmento di memoria condivisa creato dal padre.
     */
    if((shmid = shmget(SHMKEY, sizeof(dati) * numero_processi, 0777)) == -1){
        my_write(1, "						ERRORE: Recupero del segmento di memoria condivisa\n");
        exit(1);
    }
    
    /**
     *	Mappo la memoria condivisa nell'area dati del figlio a partire dal
     *	primo indirizzo disponibile.
     */
    if((buffer_comune = (dati *) shmat(shmid, 0, 0666)) == (dati *) -1){
        my_write(1, "						ERRORE: Mappatura memoria condivisa nell'area dati del figlio\n");
        exit(1);
    }
    
    /**
     Entro in un ciclo infinito per l'esecuzione della routine.
     */
    while(1){
        /**
         Attendo su un semaforo che il padre mi comunichi la presenza di operandi
         nel buffer.
         */
        sem_wait(semid_full, id);
        
        /**
         Quando sono disponibili operandi nel buffer eseguo la lettura
         e salvo i valori nelle variabili create appositamente.
         */
        num1_figlio=(buffer_comune+id)->num1;
        op_figlio=(buffer_comune+id)->op;
        num2_figlio=(buffer_comune+id)->num2;
        
        /**
         Se il padre non ha inviato il segnale di terminazione 'K' stampo
         un messaggio di notifica della ricezione dei valori.<br>
         */
        if(op_figlio != 'K') {
            sprintf(sprintf_buffer, "						#%d: Ho letto %d %c %d\n", id+1, num1_figlio, op_figlio, num2_figlio);
            my_write(1, sprintf_buffer);
            /**
             Eseguo uno switch sull'operatore per poter riconoscere l'operazione
             da svolgere e salvo il risultato ottenuto nel buffer comune.
             */
            switch(op_figlio){
                case '+':
                    (buffer_comune+id)->res=num1_figlio + num2_figlio;
                    break;
                case '-':
                    (buffer_comune+id)->res=num1_figlio - num2_figlio;
                    break;
                case '*':
                    (buffer_comune+id)->res=num1_figlio * num2_figlio;
                    break;
                case '/':
                    (buffer_comune+id)->res=num1_figlio / num2_figlio;
                    break;
                default:
                    break;
            }
        }
        /**
         Se il padre ha inviato il segnale di terminazione stampo un messaggio
         di notifica terminazione, scollego la memoria condivisa dall'area dati
         del figlio e termino l'esecuzione della routine.
         */
        else {
            sprintf(sprintf_buffer, "						#%d: Ho letto 'K' -> Termino esecuzione\n", id+1);
            my_write(1, sprintf_buffer);
            
            shmdt(buffer_comune);
            exit(1);
        }
        
        /**
         Alzo la flag res_disponibile per segnalare al padre la presenza di un
         risultato nel buffer del processo id.
         */
        (buffer_comune+id)->res_disponibile = true;
        
        /**
         Segnalo al padre che ho terminato l'esecuzione dei calcoli
         */
        sem_signal(semid_empty, id);
    }
}