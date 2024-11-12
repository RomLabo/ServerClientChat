/*
0000000001 Author RomLabo 111111111
1000111000 server.c 111111111111111
1000000001 Created on 08/11/2024 11
10001000111110000000011000011100001
10001100011110001100011000101010001
00000110000110000000011000110110001
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>

/*
    Constantes préprocesseur
*/

// Paramètres socket
#define SOCK_DOMAIN AF_INET
#define SOCK_TYPE SOCK_STREAM
#define SOCK_PROTOCOL 0

#define PORT 8080
#define LIMIT_CLIENT 5
#define BUFFER_SIZE 512

/*
    Variables
*/

int socket_server;
int socket_client;
int off_server = 0;

/*
    Déclaration des fonctions
*/
void handle_signal(int sig);
void shut_down(void);

/*
    Main
*/

int main(int argc, char *argv[]) {
    struct sigaction action;
    struct sockaddr_in addr_server, addr_client;
    socklen_t addr_len = sizeof(addr_client);
    char buffer[BUFFER_SIZE];
    
    printf("Serveur TCP \n");

    if (argc != 3) {
        printf("\n => Arrêt du serveur\n");
        printf(" => adresse ip et port non renseignés\n");
        return EXIT_FAILURE;
    }

    /* Gestion des signaux */
    action.sa_handler = &handle_signal;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("\n => Gestion signaux impossible\n");
    }

    socket_server = socket(SOCK_DOMAIN, SOCK_TYPE, SOCK_PROTOCOL);
    if (socket_server < 0) {
        perror("\n => Création socket impossible\n");
        return EXIT_FAILURE;
    }

    addr_server.sin_family = SOCK_DOMAIN;
    addr_server.sin_addr.s_addr = inet_addr(argv[1]);
    addr_server.sin_port = htons(atoi(argv[2]));

    if (bind(socket_server, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0) {
        perror("\n => Liaison socket impossible\n");
        return EXIT_FAILURE;
    }

    if (listen(socket_server, 5) < 0) {
        perror("\n => Ecoute impossible\n");
        return EXIT_FAILURE;
    }

    printf("\n => Démarrage du serveur\n");

    socket_client = accept(socket_server, (struct sockaddr *)&addr_client, &addr_len);
    if (socket_client < 0) {
        perror("\n => Connexion client impossible\n");
        close(socket_server);
        return EXIT_FAILURE;
    }

    printf("\n => Connexion client acceptée\n");

    /* Acquittement de la connexion */
    char *ack = "ACK";
    send(socket_client, ack, strlen(ack), 0);

    while (off_server == 0) {
        int nb_char = recv(socket_client, buffer, BUFFER_SIZE -1, 0);
        if (nb_char <= 0) {
            perror("\n => Erreur reception message\n");
            break;
        }

        buffer[nb_char] = '\0';
        printf("\n => Message: %s\n", buffer);

        /*if (strcmp(buffer, "exit") == 0) {
            printf("\n => Fermeture connexion\n");
            break;
        } */
    }

    shut_down();
    return 0;
}

/*
    Définition des fonctions
*/

void handle_signal(int signal) {
    if (signal == SIGINT) { 
        off_server = 1;
        shut_down(); 
    }
}

void shut_down(void) {
    // Gérer l'envoi d'un message aux clients
    close(socket_client);
    close(socket_server);
    printf("\n => Arrêt du serveur\n");
}