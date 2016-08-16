/** @file elab2.c
 
 @brief Programma che utilizzando le system call (IPC) implementa un simulatore di
 calcolo parallelo.
 
 Il programma dovrà leggere un file di configurazione contenente:<br>
 - N° di processi di calcolo parallelo<br>
 - Dati di computazione (formato id num1 op num2)<br>
 
 Il processo padre eseguirà nel seguente modo:<br>
 - Setup della simulazione leggendo dal file di configurazione il n° di
 processori da simulare, creandone i processi relativi e creando ed inizializzando
 le eventuali strutture di supporto (semafori/memoria condivisa/array dei risultati)<br>
 - Entrata in un ciclo per ogni operazione da simulare:<br>
 - se id != 0: attende che il processo id sia libero, salva l'eventuale risultato
 del calcolo precendente nell'array dei risultati e interagisce con id
 passandogli il comando da simulare.<br>
 Non deve attendere che il figlio abbia completato la simulazione.<br>
 - se id == 0: trova il primo processo libero e interagisce con esso.<br>
 Se non ci sono processi liberi attende che almeno uno lo sia.<br>
 - Passati tutti i comandi attende l'esecuzione dei calcoli da parte dei figli.<br>
 - Salvataggio dei risultati e invio del comando di terminazione a tutti i figli.<br>
 - Attesa che tutti i figli siano terminati.<br>
 - Libera eventuali risorse.<br>
 - Esce.<br>
 */
#include "functions.h"

int fd;					/**< File descriptor del file da leggere(config.txt) e su cui scrivere output(res.txt) */
int i;					/**< Contatore per i cicli */
dati* token;			/**< Struttura dati di supporto alla tokenizzazione di una linea letta dal padre */
char c;					/**< Buffer per la lettura carattere-per-carattere */
char n_proc[3]; 		/**< Buffer per il salvataggio del n° di processi da creare */
char *temp;				/**< Stringa per il salvataggio di una singola linea del file di configurazione */
int row_count=0; 		/**< Contatore del n° di operazioni da eseguire */
dati* buffer_comune;	/**< Buffer per la memoria condivisa */
int id; 				/**< Intero per il salvataggio dell'id del processore con cui il padre deve interagire */
char* write_message;	/**< Stringa per la memorizzazione dei messaggi per le system call write */
int numero_processi;	/**< Variabile contenente il numero di processi letto dal file di configurazione */

int main(int argc, char *argv[]){
    /**
     Stampo un intestazione per il programma.
     */
    my_write(1, "**************************************************************************\n");
    my_write(1, "*			SIMULATORE CALCOLO PARALLELO			 *\n");
    my_write(1, "**************************************************************************\n");
    my_write(1, "		PADRE 					FIGLIO\n\n");
    
    /**
     *	<h2>SETUP CONFIGURAZIONE</h2>
     *	Apertura del file config.txt in sola lettura tramite la system call open().
     */
    if((fd=open("config.txt", O_RDONLY, 0444)) == -1){
        my_write(1, "ERRORE: Apertura file config.txt\n");
        exit(1);
    }
    
    /**
     * 	Lettura del n° di operazioni da salvare dal file config.txt.
     *	Eseguo un ciclo While finchè ci sono caratteri da leggere nel file e incremento
     *	un contatore ogni volta che incontro il carattere new_line.
     */
    while(read(fd, &c, 1) > 0){
        if(c == '\n')
            row_count++;
    }
    
    sprintf(sprintf_buffer, "Ci sono %d operazioni da svolgere\n\n", row_count);
    my_write(1, sprintf_buffer);
    
    /**
     *	Riposiziono il cursore all'inizio del file tramite la system call lseek().
     */
    if(lseek(fd, 0, SEEK_SET) == -1){
        my_write(1, "ERRORE: riposizionamento cursore a inizio file\n");
        exit(1);
    }
    
    i = 0;
    /**
     *	Lettura del n° di processi da simulare dal file di configurazione.<br>
     *	Tale valore è contenuto nella prima riga del file, quindi eseguo un ciclo
     *	di lettura fino al primo carattere new_line e salvo in un array di caratteri
     *	le cifre lette.
     */
    while(read(fd, &c, 1) > 0){
        if(c == '\n')
            break;
        
        n_proc[i++] = c;
    }
    
    /**
     *	Salvo in una variabile intera il n° di processi da creare.
     *	Utilizzo la funzione atoi per convertire la stringa in intero.
     */
    numero_processi=atoi(n_proc);
    
    int res_count = 0; 				/* Contatore per le scritture nella struttura dati dei risultati */
    pid_t proc[numero_processi]; 	/* Vettore contenente i pid dei processi */
    res array_risultati[row_count];	/* Array dei risultati */
    
    /**
     *	Creazione di un vettore di semafori empty per coordinare
     *	le operazioni tra processo padre e figli.<br>
     *	Creo un semaforo per ogni processo che segnala al padre
     *	la disponibilità del figlio ad eseguire operazioni.
     */
    if((semid_empty = semget(EMPTYKEY, numero_processi, IPC_CREAT | IPC_EXCL | 0777)) == -1){
        my_write(1, "ERRORE: Creazione vettore di semafori empty\n");
        exit(1);
    }
    
    /**
     *	Inizializzo i semafori empty a 1.
     */
    for(i=0; i<numero_processi; i++){
        if((semctl(semid_empty, i, SETVAL, 1)) == -1){
            sprintf(sprintf_buffer, "ERRORE: Inizializzazione semaforo empty n° %d\n", i+1);
            my_write(1, sprintf_buffer);
            exit(1);
        }
    }
    
    /**
     *	Creazione di un vettore di semafori full per coordinare
     *	le operazioni tra processo padre e figli.<br>
     *	Creo un semaforo per ogni processo che segnala al figlio
     *	la disponibilità di operandi su cui eseguire calcoli nel buffer.<br>
     *	I semafori sono inizialmente posti a 0.
     */
    if((semid_full = semget(FULLKEY, numero_processi, IPC_CREAT | IPC_EXCL | 0777)) == -1){
        my_write(1, "ERRORE: Creazione vettore di semafori full\n");
        exit(1);
    }
    
    /**
     *	Creazione di un segmento di memoria condivisa per ogni processore.
     */
    if((shmid = shmget(SHMKEY, sizeof(dati) * numero_processi, 0777 | IPC_CREAT)) == -1){
        my_write(1, "ERRORE: Creazione segmento di memoria condivisa\n");
        exit(1);
    }
    
    /**
     *	Mappo la memoria condivisa nell'area dati del padre a partire dal
     *	primo indirizzo disponibile.
     */
    if((buffer_comune = (dati *) shmat(shmid, 0, 0666)) == (dati *) -1){
        my_write(1, "ERRORE: Mappatura della memoria condivisa nell'area dati del padre\n");
        exit(1);
    }
    
    /**
     *	Inizializzo tutte le flag res_disponibile a false
     */
    for(i=0; i<numero_processi; i++){
        (buffer_comune+i)->res_disponibile = false;
    }
    
    /** <h2>CREAZIONE PROCESSI</h2>
     *	Creo un processo figlio per ogni processore da simulare.<br>
     *	Eseguo un ciclo for finchè non ho creato tutti i processi necessari
     *	e inserisco i figli nell'array di processi.
     */
    
    sprintf(sprintf_buffer, "Creazione di %d processi figli\n\n", numero_processi);
    my_write(1, sprintf_buffer);
    
    for(i=0; i<numero_processi; i++){
        proc[i] = fork();
        if(proc[i] < 0){ /* Errore */
            /**
             *	In caso di errore della fork() stampo un messaggio di notifica, libero le risorse e termino.
             */
            sprintf(sprintf_buffer, "ERRORE: Creazione processo figlio n°%d\n", i+1);
            my_write(1, sprintf_buffer);
            
            free_resources(buffer_comune, shmid, semid_empty, semid_full);
            
            exit(1);
        } else if(proc[i] == 0) { /* Figlio i */
            /** <h3>FIGLIO</h3>
             * 	Esecuzione della routine di calcolo.<br>
             * 	Salvo in un buffer il n° del processo corrente.
             */
            sprintf(sprintf_buffer, "%d", i);
            
            /** Creo un vettore di argomenti contenente:
             *		- routine da eseguire
             *		- n° del processo che esegue
             * 		- n° di processori totali
             */
            char *args[] = {"./routine", sprintf_buffer, n_proc, NULL};
            
            /**
             * 	Eseguo la routine di calcolo per il processo figlio.<br>
             *	Utilizzo la funzione execvp() a cui passo il nome dell'eseguibile
             *	e la lista degli argomenti.
             */
            execvp(args[0], args);
            
            /**
             *	Se la execvp ritorna è fallita.<br>
             *	Libero le risorse create e termino.
             */
            sprintf(sprintf_buffer, "ERRORE: Execvp routine figlio n°%d\n", i+1);
            my_write(1, sprintf_buffer);
            
            free_resources(buffer_comune, shmid, semid_empty, semid_full);
            
            exit(1);
        }
    }
    
    /**  <h3>PADRE</h3>
     *	Eseguo un ciclo di scrittura dei dati nel buffer.
     */
    while(1){
        /**
         Salvo in una stringa la linea letta dalla funzione leggi_linea().
         */
        temp=leggi_linea(fd);
        
        /**
         Se la stringa letta è vuota la lettura del file è terminata: interrompo
         il ciclo e continuo l'esecuzione del programma.
         */
        if(strncmp(temp, "", 1) == 0)
            break;
        
        /**
         Altrimenti salvo in una struttura dati la linea appena letta tokenizata
         con la funzione tokenize().
         */
        token=tokenize(temp);
        
        /**
         Se l'id letto è diverso da 0
         */
        if(token->id_sem != 0)
        {
            /**
             Devo interagire con il figlio id.<br>
             Stampo un messaggio di notifica e salvo in una variabile il n°
             del processo con cui dovrò interagire.
             */
            sprintf(sprintf_buffer, "Attendo figlio %d\n", token->id_sem);
            my_write(1, sprintf_buffer);
            id = token->id_sem - 1; /* Assegno a id il valore del semaforo con cui devo interagire */
        }
        /**
         Se l'id letto è uguale a 0 devo scorrere tra i processi per cercare il primo
         libero e interagirvi o attendere se tutti sono occupati.
         */
        else
        {
            /**
             Utilizzo un contatore per scorrere tra i vari processi e controllare
             il loro stato.
             */
            int id_count=0;
            /**
             Entro in un ciclo infinito.
             */
            while(1)
            {
                /**
                 Se il semaforo empty del processo che sto controllando è posto a 1
                 significa che il processo è libero: posso interrompere il ciclo e
                 continuare l'esecuzione.
                 */
                if(semctl(semid_empty, id_count, GETVAL) == 1)
                    break;
                
                /**
                 Se ho raggiunto la fine del vettore di processori e non ho ancora trovato
                 un processo libero azzero il contatore e ricomincio il controllo,
                 altrimenti incremento il contatore e continuo la scansione del vettore.
                 */
                if(id_count == numero_processi - 1) {
                    id_count = 0;
                   	my_write(1, "Nessun processo è libero, attendo\n");
                   	/* Simulo attesa con una sleep(). */
                   	sleep(2);
                } else
                    id_count++;
            }
            /**
             Quando trovo un processo libero stampo un messaggio di notifica
             e salvo in una variabile id il n° del processore con cui devo
             interagire.
             */
            sprintf(sprintf_buffer, "Figlio%d è libero\n", id_count + 1);
            my_write(1, sprintf_buffer);
            id = id_count;
        }
        
        /**
         Attendo che il processo id sia libero.
         */
        sem_wait(semid_empty, id);
        
        /**
         *	Controllo se è disponibile un risultato precedente nel buffer di id,
         * 	in tal caso salvo una stringa nell'array dei risultati, incremento il contatore dei
         *  risultati e pongo a false la flag per permettere ulteriori
         * 	scritture.
         */
        if((buffer_comune+id)->res_disponibile) {
            array_risultati[res_count].n1 = (buffer_comune+id)->num1;
            array_risultati[res_count].op = (buffer_comune+id)->op;
            array_risultati[res_count].n2 = (buffer_comune+id)->num2;
            array_risultati[res_count].res = (buffer_comune+id)->res;
            res_count++;
            
            sprintf(sprintf_buffer, "\nRicevuto risultato da figlio %d\n%d/%d Operazioni svolte (%.2f%%)\n\n",
                    id+1, res_count, row_count, (float) res_count / row_count * 100);
            my_write(1, sprintf_buffer);
        }
        
        /**
         Scrittura dei dati appena letti nel buffer condiviso alla
         posizione id.
         */
        *(buffer_comune+id)=*token;
        
        /**
        	Segnalo al figlio id la presenza di operandi nel buffer.
         */
        sem_signal(semid_full, id);
        
        /**
         Stampo un messaggio di notifica dell'avvenuta scrittura.
         */
        sprintf(sprintf_buffer, "Scrittura dati a figlio %d completata\n\n", id + 1);
        my_write(1, sprintf_buffer);
    }
    
    /**
     *	Lettura del file config.txt terminata: il processo padre ha letto e
     *	inviato tutte le linee del file.<br>
     *	Entro in un ciclo che scansiona tutti i processi.
     */
    for(i=0; i<numero_processi; i++){
        sprintf(sprintf_buffer, "Invio segnale terminazione a figlio %d\n", i + 1);
        my_write(1, sprintf_buffer);
        
        /**
         Attendo che il semaforo i-esimo abbia completato i calcoli
         che lo tenevano occupato.
         */
        sem_wait(semid_empty, i);
        
        /**
         Eventualmente salvo gli ultimi risultati.
         */
        if((buffer_comune+i)->res_disponibile){
            array_risultati[res_count].n1 = (buffer_comune+i)->num1;
            array_risultati[res_count].op = (buffer_comune+i)->op;
            array_risultati[res_count].n2 = (buffer_comune+i)->num2;
            array_risultati[res_count].res = (buffer_comune+i)->res;
            res_count++;
            
            sprintf(sprintf_buffer, "\nRicevuto risultato da figlio %d\n%d/%d Operazioni svolte (%.1f%%)\n\n",
                    i+1, res_count, row_count, (float) res_count / row_count * 100);
            my_write(1, sprintf_buffer);
        }
        
        /**
         Scrivo nel buffer il segnale di terminazione 'K' come operatore.
         */
        (buffer_comune+i)->op = 'K';
        
        /**
         Segnalo al figlio la presenza del segnale di terminazione.
         */
        sem_signal(semid_full, i);
        
        /**
         Attendo che il figlio i-esimo abbia letto 'K' e terminato la sua esecuzione.
         */
        wait(&proc[i]);
    }
    
    /**
     Stampo un messaggio di notifica per la terminazione dei calcoli.
     */
    my_write(1, "**************************************************************************\n");
    my_write(1, "Calcoli terminati\n");
    
    /** <h2>SCRITTURA RISULTATI</h2>
     *	Creo un file res.txt su cui scrivere i risultati dei calcoli effettuati
     *	dai processori.
     */
    if((fd=creat("res.txt", 0777)) == -1){
        my_write(1, "ERRORE: Creazione file res.txt\n");
        exit(1);
    }
    
    /**
     Stampo un intestazione nel file di output.
     */
    my_write(fd, "*************************\n");
    my_write(fd, "*      RISULTATI        *\n");
    my_write(fd, "*************************\n");
    
    /**
     Entro in un ciclo per ogni risultato da scrivere salvato nell'array
     dei risultati e scrivo sul file res.txt una stringa per ogni valore
     calcolato preceduto da un contatore e dalla descrizione dei calcoli
     effettuati.
     */
    for(i=0; i<row_count; i++){
        sprintf(sprintf_buffer, "%d. %d %c %d = %d\n", i+1, array_risultati[i].n1, array_risultati[i].op, array_risultati[i].n2, array_risultati[i].res);
        my_write(fd, sprintf_buffer);
    }
    
    my_write(1, "Scrittura risultati sul file di output 'res.txt' terminata\n");
    my_write(1, "**************************************************************************\n");
    
    /**
    	Libero le risorse allocate con free_resources() e termino.
     */
    free_resources(buffer_comune, shmid, semid_empty, semid_full);
    
    exit(0);
}