/*

  TCPCLIENT.C
  ==========

*/


#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */
#include <netinet/in.h>
#include <netdb.h>

#include "helper.h"           /*  Our own helper functions  */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*  Global constants  */
#define MAX_LINE           (1000)
int       conn_s;                /*  connection socket         */

int ParseCmdLine(int argc, char *argv[], char **szAddress, char **szPort);
void check(char* tmp);
void signal_handler(int signal);

/*  main()  */
int main(int argc, char *argv[]) {
    
    short int port;                  /*  port number               */
    struct    sockaddr_in servaddr;  /*  socket address structure  */
    char      buffer[MAX_LINE];      /*  character buffer          */
    char     *szAddress;             /*  Holds remote IP address   */
    char     *szPort;                /*  Holds remote port         */
    char     *endptr;                /*  for strtol()              */
    struct	hostent *he;
    char      tmp[MAX_LINE]; /* temporary buffer used for data manipulations */
    char      op[10];                /* buffer to store the selected operation */
    
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
    
    he = NULL;

    /*  Get command line arguments  */
    ParseCmdLine(argc, argv, &szAddress, &szPort);

    
    /*  Set the remote port  */
    port = strtol(szPort, &endptr, 0);
    if ( *endptr ) {
        printf("client: porta non riconosciuta.\n");
        exit(EXIT_FAILURE);
    }
	

    /*  Create the listening socket  */
    if ( (conn_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        fprintf(stderr, "client: errore durante la creazione della socket.\n");
        exit(EXIT_FAILURE);
    }


    /*  Set all bytes in socket address structure to zero, and fill in the relevant data members   */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_port        = htons(port);

    
    /*  Set the remote IP address  */
    if ( inet_aton(szAddress, &servaddr.sin_addr) <= 0 ) {
        printf("client: indirizzo IP non valido.\nclient: risoluzione nome...");
		
        if ( (he = gethostbyname(szAddress)) == NULL ) {
            printf("fallita.\n");
            exit(EXIT_FAILURE);
        }
        printf("riuscita.\n\n");
        servaddr.sin_addr = *((struct in_addr *) he->h_addr );
    }
	
    
                         /* ===============      CLIENT-USER AND CLIENT-SERVER INTERACTION    =============== */
    
    /*  login  */
    printf("Benvenuto nell'elenco telefonico!\nDigita add per inserire un nuovo contatto oppure search per cercarne il numero.\n");
    fgets(tmp, MAX_LINE, stdin);
    strtok(tmp, "\n"); // fgets stops because of \n but it considers it and I need to take it off
    strcpy(op,tmp); // store the operation for further purposes
    strcpy(buffer, "Y "); // Y = YES. Here the concatenation in the buffer starts: login op name password
    strcat(buffer, tmp);
    strcat(buffer, " ");
    
    
    /*  check correctness of operation  */
    if ( strncmp(tmp, "add", strlen("add")) != 0 && strncmp(tmp, "search", strlen("search")) != 0 ) {
        printf("L'operazione inserita non corrisponde a quelle indicate.\n");
        exit(EXIT_FAILURE);
    }
    
    /*  username  */
    printf("Inserisci il tuo nome: ");
    fgets(tmp, MAX_LINE, stdin);
    check(tmp); // validating name
    strtok(tmp, "\n");
    strcat(buffer, tmp);
    strcat(buffer, " ");
    
    /*  password  */
    printf("Inserisci la tua password: ");
    fgets(tmp, MAX_LINE, stdin);
    check(tmp); // validating password
    strcat(buffer,tmp); // not taking off \n because it's necessary for Readline
    strcat(buffer, " ");
    
    
    /*  once everything is ready, connect() to the remote echo server  */
    if ( connect(conn_s, (struct sockaddr *) &servaddr, sizeof(servaddr) ) < 0 ) {
        printf("client: errore durante la connect.\n");
        exit(EXIT_FAILURE);
    }     

    /*  Send buffer to echo server */
    Writeline(conn_s, buffer, strlen(buffer));
    memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    
    /*  Retrieve response from echo server  */
    Readline(conn_s, buffer, MAX_LINE-1);

    /*  Output echoed string  */
    printf("Risposta del server: %s\n", buffer);
    
    if ( strcmp(buffer, "Impossibile accedere: username o password errati.\n") == 0 ) {
        exit(EXIT_FAILURE);
    }
    
    if ( strcmp(buffer, "") == 0 ){
        printf("Problemi con il server o tempo per la scelta scaduto. Provare a riconnettersi.\n");
        exit(EXIT_FAILURE);
    }
    
    if ( close(conn_s) < 0 ) {
        fprintf(stderr, "client: errore durante la close.\n");
        exit(EXIT_FAILURE);
    }
    
    /*  Login is over, now operation starts  */
    memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    
    if ( strcmp(op, "add") == 0 ) {
        strcpy(buffer, "N add ");
        
        printf("Inserisci il nome del contatto da aggiungere: ");
        fgets(tmp, MAX_LINE, stdin);
        check(tmp);
        strtok(tmp, "\n");
        strcat(buffer, tmp);
        strcat(buffer, " ");
        
        printf("Inserisci il cognome del contatto da aggiungere: ");
        fgets(tmp, MAX_LINE, stdin);
        check(tmp);
        strtok(tmp, "\n");
        strcat(buffer, tmp);
        strcat(buffer, " ");
        
        printf("Inserisci il numero del contatto da aggiungere: ");
        fgets(tmp, MAX_LINE, stdin);
        check(tmp);
        strcat(buffer, tmp);
        strcat(buffer, " ");
    }
    
    else {
        strcpy(buffer, "N search ");
        
        printf("Inserisci il nome del contatto da cercare: ");
        fgets(tmp, MAX_LINE, stdin);
        check(tmp);
        strtok(tmp, "\n");
        strcat(buffer, tmp);
        strcat(buffer, " ");
        
        printf("Inserisci il cognome del contatto da cercare: ");
        fgets(tmp, MAX_LINE, stdin);
        check(tmp);
        strcat(buffer, tmp);
        strcat(buffer, " ");

    }
    
    
    /*  Opening connection again  */
    if ( (conn_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        fprintf(stderr, "client: errore durante la creazione della socket.\n");
        exit(EXIT_FAILURE);
    }
    
    if ( connect(conn_s, (struct sockaddr *) &servaddr, sizeof(servaddr) ) < 0 ) {
        printf("client: errore durante la connect.\n");
        exit(EXIT_FAILURE); 
    }
    
    /*  Send buffer to echo server */
    Writeline(conn_s, buffer, strlen(buffer));
    memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    
    /*  Retrieve response from echo server  */
    Readline(conn_s, buffer, MAX_LINE-1);

    /*  Output echoed string  */
    printf("Risposta del server: %s\n", buffer);
    
    if ( strcmp(buffer, "Impossibile inserire il numero in quanto è già presente.\n") == 0  ||
         strcmp(buffer, "Numero non presente in rubrica.\n") == 0 ) {
        
         if ( close(conn_s) < 0 ) {
            fprintf(stderr, "server: errore durante la close.\n");
         }
         exit(EXIT_FAILURE);
     }
    
    if ( strcmp(buffer, "") == 0 ){
        printf("Problemi con il server o tempo per la scelta scaduto. Provare a riconnettersi.\n");
        
        if ( close(conn_s) < 0 ) {
            fprintf(stderr, "server: errore durante la close.\n");
        }
        exit(EXIT_FAILURE);
    }
    
    
    if ( close(conn_s) < 0 ) {
        fprintf(stderr, "server: errore durante la close.\n");
    }
    
    return EXIT_SUCCESS;
}


            /* ===============      AUXILIARY FUNCTIONS     =============== */


void check(char* tmp) {
   
    if ( strcmp(tmp, "\n" ) == 0 ) {
        printf("L'inserimento del campo è obbligatorio.\n");
        exit(EXIT_FAILURE);
    }
    
    if ( strlen(tmp) > 26 ) { // max length for name is 25, but tmp includes \n, so tmp length is not supposed to be greater than 26
        printf("Non possono essere usati più di 25 caratteri.\n");
        exit(EXIT_FAILURE);
    }
    
}




void signal_handler(int signal) {
    
    switch (signal) {
            
        case SIGHUP:
            printf("Catturato il segnale SIGHUP\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
            }
            exit(EXIT_FAILURE);
            break;
            
        case SIGINT:
            printf("Catturato il segnale SIGINT\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
            }
            exit(EXIT_FAILURE);
            break;
            
        case SIGQUIT:
            printf("Catturato il segnale SIQUIT\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
            }
            exit(EXIT_FAILURE);
            break;
            
        case SIGILL:
            printf("Catturato il segnale SIGILL\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
            }
            exit(EXIT_FAILURE);
            break;
            
        case SIGSEGV:
            printf("Catturato il segnale SIGSEGV\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
            }
            exit(EXIT_FAILURE);
            break;
            
        case SIGTERM:
            printf("Catturato il segnale SIGTERM\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
            }
            exit(EXIT_FAILURE);
            break;
            
    }
}



int ParseCmdLine(int argc, char *argv[], char **szAddress, char **szPort) {
    int n = 1;

    while ( n < argc ) {
        
        if ( !strncmp(argv[n], "-a", 2) || !strncmp(argv[n], "-A", 2) ) {
            *szAddress = argv[++n];
        }

        else 
            if ( !strncmp(argv[n], "-p", 2) || !strncmp(argv[n], "-P", 2) ) {
	*szPort = argv[++n];
            }
            else
                if ( !strncmp(argv[n], "-h", 2) || !strncmp(argv[n], "-H", 2) ) {
                    printf("Sintassi:\n\n");
                    printf("    client -a (indirizzo server) -p (porta del server) [-h].\n\n");
                    exit(EXIT_SUCCESS);
                }
        ++n;
    }
    
    if (argc==1) {
        printf("Sintassi:\n\n");
        printf("    client -a (indirizzo server) -p (porta del server) [-h].\n\n");
        exit(EXIT_SUCCESS);
    }
    
    return 0;
}



