# FileStorage_Server
## Progetto_SistemiOperativi2021
Il progetto consiste in un **file storage server** in cui la memorizzazione dei file avviene in memoria
principale. La capacità dello storage è fissata all’avvio e non varia dinamicamente durante l’esecuzione del server.
Per poter leggere, scrivere o eliminare file all’interno del file storage, il client deve connettersi al server ed utilizzare
una API che dovrà essere sviluppata dallo studente (come descritto nel seguito). Il server esegue tutte le operazioni
richieste dai client operando sempre e solo in memoria principale e mai sul disco.
La capacità del file storage, unitamente ad altri parametri di configurazione, è definita al momento dell’avvio del
server tramite un file di configurazione testuale.
Il file storage server è implementato come un singolo processo multi-threaded in grado di accettare connessioni
da multipli client. Il processo server dovrà essere in grado di gestire adeguatamente alcune decine di connessioni
contemporanee da parte di più client.
Ogni client mantiene **una sola connessione** verso il server sulla quale invia una o più richieste relative ai file
memorizzati nel server, ed ottiene le risposte in accordo al protocollo di comunicazione “richiesta-risposta”. Un file è
identificato univocamente dal suo path assoluto.

### Files
Per la Realizzazione del file system, quindi la gestione della memorizzazione dei file, si utilizza un Hash 
map che gestisce le collisioni con concatenamento, per rappresentare un file all’interno dell’Hash map si 
utilizza una **struct(file.h)**, infine c’è un ulteriore lista **(list.h)** per realizzare la politica di rimpiazzamento 
FIFO per la gestione dei file da espellere dal server in caso di *capacity misses*.
### CLIENT
Il client invia le proprie richieste al server grazie ad una *API*, per ogni operazione presente il client assegna il 
codice al campo request.OP e il nome del file al campo pathname, nel caso della OPEN viene anche riempito 
il campo flags invece per le operazioni di scrittura viene assegnata anche la dimensione del file nel campo 
size.
Infine, il client invierà l’intera struct sulla socket e rimarrà in attesa della risposta del server.
### SERVER
Il server manterrà una coda di struct request ad ogni richiesta dei client il server invocherà la funzione 
readMessage che leggerà l’intera struct e assegnerà il campo request.fd e in caso size sia diverso 0, quindi 
per le operazioni di scrittura, attenderà che il client invii anche il contenuto del file, una volta ottenute tutte le 
informazioni la richiesta verrà inserita in testa alla lista.
Le operazioni appena citate sono gestite da un **thread dispatcher** che, in più, si occupa di accettare le 
richieste di connessione dai vari client.

Vengono anche mandati in esecuzione una serie di **thread Workers** che si occupano di prelevare una 
richiesta dalla coda e di completare la funzione richiesta grazie al campo OP, una volta completata la 
richiesta invierà un codice al client che corrisponderà all’esito dell’operazione.
I codici cambiano a seconda dell’esito dell’operazione (Protcol.h)

### Istruzioni per l'esecuzione e per la compilazione

Per compilare ed eseguire:
* chmod +x Test/test1
* chmod +x Test/test2
* chmod +x Test/test3
* chmod +x Test/scriptclient
* chmod +x script/scriptstat
* make test1
* make test2
* make test

