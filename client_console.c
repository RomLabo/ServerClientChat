/*
0000000001 Author RomLabo 111111111
1000111000 client_console.c 1111111
1000000001 Created on 06/12/2024 11
10001000111110000000011000011100001
10001100011110001100011000101010001
00000110000110000000011000110110001
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

/*
    Constantes
*/

// Paramètres socket
const int sock_domain = AF_INET;
const int sock_type = SOCK_STREAM;
const int sock_protocol = 0;

const int port = 8080;
const char addr_inet[10] = "127.0.0.1";
const int buffer_size = 1024;

enum msg_config { 
    DEFAULT, PORT_MISSING, TOO_MUCH 
};

// Code erreur de l'application
enum error_type {
    ERR_TOO_MUCH_ARG = 2, ERR_CREATE_SOCKET,
    ERR_CONNECT_SERVER, ERR_READ_ACK, ERR_HANDLE_SIG
};

/*
    Variables
*/

int socket_client;
int off_client = 0;
int off_recv = 0;
int off_send = 0;

/*
    Déclaration des fonctions
*/

void setup_addr(int argc, char *argv[], struct sockaddr_in *addr);
void wait_acquit(void);
void receive_msg(void* socket_desc);
void handle_signal(int signal);
void shut_down(void);

void msg_default_config(void);
void msg_port_missing(void);
void msg_too_much(void);

/*
    Tableau de pointeurs sur fonction affichage
*/

void (*show_msg[3])(void) = {
    msg_default_config, msg_port_missing, msg_too_much
};

/*
    Main
*/

int main(int argc, char *argv[]) {
    char buffer[buffer_size];
    pthread_t thread_id;
    pthread_attr_t attr;
    struct sigaction action;
    struct sockaddr_in addr_server;
    
    printf("\nClient TCP \n");

    /* Vérification des arguments */
    if (argc > 3) {
        (*show_msg[TOO_MUCH])();
        return ERR_TOO_MUCH_ARG;
    }

    /* Configuration de l'adresse du serveur */
    setup_addr(argc, argv, &addr_server);

    /* Gestion des signaux */
    action.sa_handler = &handle_signal;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("  Gestion signaux impossible\n");
        return ERR_HANDLE_SIG;
    }

    /* Création socket client */
    if ((socket_client = socket(sock_domain, sock_type, sock_protocol)) == -1) {
        perror("  Création du socket impossible\n");
        return ERR_CREATE_SOCKET;
    }

    /* Création de la connection */
    if (connect(socket_client, (struct sockaddr*)&addr_server,sizeof(addr_server)) == -1) {
        close(socket_client);
        perror("  Connexion au serveur impossible\n");
        return ERR_CONNECT_SERVER;
    }

    /* Attente de l'acquittement connection */
    wait_acquit();

    /* Créatiion d'un thread pour la réception des message
       afin de ne pas bloquer l'envoi de message */
    pthread_attr_init(&attr);
    pthread_create(&thread_id, &attr,(void*)receive_msg, &socket_client);

    /* Boucle principale pour la gestion de l'envoi des messages. */
    while (off_send == 0) {
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            printf("\033[A\33[2K\r");
            send(socket_client, buffer, strlen(buffer) + 1, 0);
        } 
    }

    printf("  Arrêt envoi message\n");
    pthread_join(thread_id, NULL);

    /* Fermeture de la connexion */
    close(socket_client);

    return EXIT_SUCCESS;
}

/*
    Définition des fonctions
*/

void setup_addr(int argc, char *argv[], struct sockaddr_in *addr) {
    (*addr).sin_family = sock_domain;
    (*addr).sin_addr.s_addr = inet_addr(addr_inet);
    (*addr).sin_port = htons(port);

    if (argc == 3) {
        (*addr).sin_addr.s_addr = inet_addr(argv[1]);
        (*addr).sin_port = htons(atoi(argv[2]));
    } else if (argc == 2) {
        (*addr).sin_addr.s_addr = inet_addr(argv[1]);
        (*addr).sin_port = htons(port);
        (*show_msg[PORT_MISSING])();
    } else {
        (*show_msg[DEFAULT])();
    }
}

void wait_acquit(void) {
    char acquit_msg[4];
    while (strcmp(acquit_msg, "ACK") != 0) {
        if (read(socket_client, &acquit_msg, 4) < 0) {
            close(socket_client);
            perror("  Lecture acquittement impossible\n");
            exit(ERR_READ_ACK);
        }
    }
    /* Réinitialisation du message d'acquittement */
    strcpy(acquit_msg, "");
}

void receive_msg(void* socket_desc) {
    int socket = *(int*)socket_desc;
    char buffer[buffer_size];

    while (off_recv == 0) {
        int bytes_received = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0 && off_recv == 0) {
            printf("  Lecture des messages impossible\n");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    printf("  Arrêt reception message\n");
    pthread_exit(NULL);
}

void handle_signal(int signal) {
    if (signal == SIGINT) { shut_down(); }
}

void shut_down(void) {
    off_recv = 1;
    off_send = 1;
    shutdown(socket_client, 2);
    printf("\n  Arrêt de l'application en cours ...\n");
}

void msg_default_config(void) {
    printf("  Paramètres manquants \n\n");
    printf("  Renseigner : \n");
    printf("    1) adresse ip serveur\n");
    printf("    2) port du serveur\n");
    printf("\n  Utilisation de paramètres par défaut :\n");
    printf("  - adresse serveur : 127.0.0.1\n");
    printf("  - port serveur : 8080\n\n");
}

void msg_port_missing(void) {
    printf("  Paramètres manquants \n");
    printf("\n  Renseigner : \n");
    printf("    1) port du serveur\n");
    printf("\n  Utilisation de paramètres par défaut :\n");
    printf("  - port serveur : 8080\n\n");
}

void msg_too_much(void) {
    printf("  Trop d arguments renseignés \n");
    printf("\n  Les paramètres à renseigner sont : \n");
    printf("    1) adresse ip serveur\n");
    printf("    2) port du serveur\n\n");
}