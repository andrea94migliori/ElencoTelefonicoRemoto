/*

  TCPSERVER.C
  ==========

*/


#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */
#include <errno.h>
#include "helper.h"           /*  our own helper functions  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>


/*  Global constants  */
#define MAX_LINE           (1000)
int       conn_s;                /*  connection socket         */

void manager(int conn_s, char* buffer);
void find(int conn_s, char* username, char* password, char* path);
void add(int conn_s, char* name, char* surname, char* number);
void search(int conn_s, char* name, char* surname);
void signal_handler(int signal);
int ParseCmdLine(int argc, char *argv[], char **szPort);

/*  main()  */
int main(int argc, char** argv) {

    int       list_s;                /*  listening socket          */
    short int port;                  /*  port number               */
    struct    sockaddr_in servaddr;  /*  socket address structure  */
    struct	  sockaddr_in their_addr;
    char      buffer[MAX_LINE];      /*  character buffer          */
    char     *endptr;                /*  for strtol()              */
    int 	sin_size;
    
    
    signal(SIGALRM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE,SIG_IGN); // it never happens
    
    
    /*  Get command line arguments  */
    ParseCmdLine(argc, argv, &endptr);
	
    port = strtol(endptr, &endptr, 0);
    if ( *endptr ) {      
        fprintf(stderr, "server: porta non riconosciuta.\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Server in ascolto sulla porta %d\n",port);
	
      
    /*  Create the listening socket  */
    if  (( list_s = socket(AF_INET, SOCK_STREAM, 0 )) < 0 ) {
        fprintf(stderr, "server: errore nella creazione della socket.\n");
        exit(EXIT_FAILURE);
    }
    
    
    /*  Set all bytes in socket address structure to zero, and fill in the relevant data members   */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);
   
    
    /*  Bind our socket addresss to the listening socket, and call listen()  */
    if ( bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
        fprintf(stderr, "server: errore durante la bind.\n");
        exit(EXIT_FAILURE);
    }
    
    if ( listen(list_s, LISTENQ) < 0 ) {
        fprintf(stderr, "server: errore durante la listen.\n");
        exit(EXIT_FAILURE);
    }
    
    
    /*  Enter an infinite loop to respond to client requests and echo input  */
    while ( 1 ) {

    /*  Wait for a connection, then accept() it  */
        sin_size = sizeof(struct sockaddr_in);
        if ( (conn_s = accept(list_s, (struct sockaddr *)&their_addr, &sin_size) ) < 0 ) {
            fprintf(stderr, "server: errore nella accept.\n");
            exit(EXIT_FAILURE);
        }

        printf("server: connessione da %s\n", inet_ntoa(their_addr.sin_addr));
        
        alarm(1);
        
        /*  Retrieve an input line from the connected socket */
        Readline(conn_s, buffer, MAX_LINE-1);
        
        printf("Buffer ricevuto: %s\n",buffer);
        
        /*  Auxiliary function to manage all the further aspects  */
        manager(conn_s, buffer);
        
        
        /*  Close the connected socket  */
        if ( close(conn_s) < 0 && errno != EBADF) {
            // if there's an error on close and it's not due to a previous close (errno == EBADF) go on with printf
            // otherwise (there's no error || it's due to a previous close) do nothing because it's already closed.
            
            fprintf(stderr, "server: errore durante la close.\n");
            printf("Error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    
    if ( close(list_s) < 0 ){
        fprintf(stderr, "server: errore durante la close.\n");
        exit(EXIT_FAILURE);
    }
    
}
    
                                        /* ===============      AUXILIARY FUNCTIONS     =============== */   


/*  function to manage operations  */
void manager(int conn_s, char* buffer) {
    
    /*  decompose buffer fields */
    char login_need[5];
    char op[10];
    char username[25];
    char password[25];
    char name[25];
    char surname[25];
    char number[25];
    
    sscanf(buffer, "%s %s", login_need, op);    
    
    /*  check if login_need is YES  */
    if ( strcmp(login_need, "Y") == 0 ) {
        
        sscanf(buffer, "%s %s %s %s", login_need, op, username, password);
            
        /*  serverside data print  */
        printf("Operation: %s\n", op);
        printf("Username: %s\n", username);
        printf("Password: %s\n", password);
        
        if ( strcmp(op, "add") == 0) find(conn_s, username, password, "addPermission.txt");
        else find(conn_s, username, password, "searchPermission.txt");
    }
    
    else {
        
        /*  If login_need is NO, do the correct operation  */
        if ( strcmp(op, "add") == 0 ) {
            
            sscanf(buffer, "%s %s %s %s %s", login_need, op, name, surname, number);
            printf("Operation: %s\n", op);
            printf("Name: %s\n", name);
            printf("Surname: %s\n", surname);
            printf("Number: %s\n", number);
            add(conn_s, name, surname, number);
        }
        
        if ( strcmp(op, "search") == 0 ) {
            
            sscanf(buffer, "%s %s %s %s", login_need, op, name, surname);
            printf("Operation: %s\n", op);
            printf("Name: %s\n", name);
            printf("Surname: %s\n", surname);
            search(conn_s, name, surname);
        }
    }
}




/*  function to check if the user is allowed to do an operation  */
void find(int conn_s, char* username, char* password, char* path) {
    FILE* file;
    char name[25];
    char pw[25];
    char buff[200];
    char buffer[MAX_LINE];
    char* res;
    int found = 0;
    
    file = fopen (path, "r");
    if ( file == NULL ) {
        printf("Errore nell'apertura del file.\n");
        exit(1);
    }
    
    while(1) {
        res = fgets(buff, 70, file);
        if ( res == NULL ) break;
        
        sscanf(buff, "%s %s", name, pw);
        
        if ( strncmp(username, name, strlen(username)) == 0 && strncmp(password, pw, strlen(password)) == 0 ) {
            found = 1;
            break;
        }        
    }
    
    if ( found ) {
        strcpy(buffer,"Login andato a buon fine.\n");
        Writeline(conn_s, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    }
    
    else {
        strcpy(buffer,"Impossibile accedere: username o password errati.\n");
        Writeline(conn_s, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    }
    
    fclose(file); 
}


/*  Function to add a number  */
void add(int conn_s, char* name, char* surname, char* number) {
    
    /*  Check if the number is present  */
    FILE* file;
    char nome[25];
    char cognome[25];
    char numero[25];
    char buff[200];
    char buffer[MAX_LINE];
    char* res;
    int found = 0;
    
    file = fopen ("ElencoTelefonico.txt", "r");
    if ( file == NULL ) {
        printf("Errore nell'apertura del file.\n");
        exit(1);
    }
    
    while(1) {
        res = fgets(buff, 70, file);
        if ( res == NULL ) break;
        
        sscanf(buff, "%s %s %s", nome, cognome, numero); //sistema ricerca solo per numero
        
        if ( strncmp(number, numero, strlen(number)) == 0 ) {
            found = 1;
            break;
        }        
    }
    
    if ( found ) {
        strcpy(buffer,"Impossibile inserire il numero in quanto è già presente.\n");
        Writeline(conn_s, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    }
    
    else {
        fclose(file);
        file = fopen ("ElencoTelefonico.txt", "a");
        if ( file == NULL ) {
            printf("Errore nell'apertura del file.\n");
            exit(1);
        }
        fprintf(file, "%s %s %s\n", name, surname, number);
        strcpy(buffer,"Operazione add completata con successo.\n");
        Writeline(conn_s, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    }
    fclose(file); 
}


/*  Function to search a number  */
void search(int conn_s, char* name, char* surname) {
    /*  Check if the number is present  */
    FILE* file;
    char nome[25];
    char cognome[25];
    char numero[25];
    char buff[200];
    char buffer[MAX_LINE];
    char* res;
    int found = 0;
    
    file = fopen ("ElencoTelefonico.txt", "r");
    if ( file == NULL ) {
        printf("Errore nell'apertura del file.\n");
        exit(1);
    }
    
    strcpy(buffer, "I numeri associati al contatto sono: ");
    
    while(1) {
        res = fgets(buff, 70, file);
        if ( res == NULL ) break;
        
        sscanf(buff, "%s %s %s", nome, cognome, numero); //sistema ricerca solo per numero
        
        if ( strncmp(name, nome, strlen(name)) == 0 && strncmp(surname, cognome, strlen(surname)) == 0 ) {
            found = 1;
            strcat(buffer, numero);
            strcat(buffer, " ");
        }        
    }
    
    if ( found ) {
        strcat(buffer, "\n");
        printf("%s", buffer);
    }
    
    else {
        strcpy(buffer,"Numero non presente in rubrica.\n");
    }
    
    Writeline(conn_s, buffer, strlen(buffer));
    memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    fclose(file); 
}



void signal_handler(int signal) {
    
    switch(signal) {
        case SIGALRM:
            if ( close(conn_s) < 0 ) { // error on close
                if ( errno != EBADF ) { // it's not due to a previous close
                    fprintf(stderr, "server: errore durante la close.\n");
                    printf("Error: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                // I ignore it because the socker has already been closed
            }
            else printf("Timeout scaduto.\n");
            break;
    
        case SIGHUP:
            printf("Catturato il segnale SIGHUP\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
                exit(EXIT_FAILURE);
            }
            break;
    
        case SIGINT:
            printf("Catturato il segnale SIGINT\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
                exit(EXIT_FAILURE);
            }
            break;
    
        case SIGQUIT:
            printf("Catturato il segnale SIQUIT\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
                exit(EXIT_FAILURE);
            }
            break;
    
        case SIGILL:
            printf("Catturato il segnale SIGILL\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
                exit(EXIT_FAILURE);
            }
            break;
    
        case SIGSEGV:
            printf("Catturato il segnale SIGSEGV\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
                exit(EXIT_FAILURE);
            }
            break;
    
        case SIGTERM:
            printf("Catturato il segnale SIGTERM\n");
            if ( close(conn_s) < 0 ) {
                fprintf(stderr, "server: errore durante la close.\n");
                exit(EXIT_FAILURE);
            }
            break;
    }
}


/*  get command line arguments  */
int ParseCmdLine(int argc, char *argv[], char **szPort) {
    int n = 1;
    
    while ( n < argc ) {
        if ( !strncmp(argv[n], "-p", 2) || !strncmp(argv[n], "-P", 2) )
            *szPort = argv[++n];
        else
            if ( !strncmp(argv[n], "-h", 2) || !strncmp(argv[n], "-H", 2) ) {
                printf("Sintassi:\n\n");
                printf("    server -p (porta) [-h]\n\n");
                exit(EXIT_SUCCESS);
            }
        ++n;
    }
    if (argc==1) {
        printf("Sintassi:\n\n");
        printf("    server -p (porta) [-h]\n\n");
        exit(EXIT_SUCCESS);
    }
    return 0;
}
