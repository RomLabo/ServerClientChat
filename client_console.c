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

int main_socket;
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

void connect_on_channel(const char* ip, int port);

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
    struct sigaction action;
    struct sockaddr_in main_addr_server;
    
    printf("\nClient TCP \n");

    /* Vérification des arguments */
    if (argc > 3) {
        (*show_msg[TOO_MUCH])();
        return ERR_TOO_MUCH_ARG;
    }

    /* Configuration de l'adresse du serveur */
    setup_addr(argc, argv, &main_addr_server);

    /* Gestion des signaux */
    action.sa_handler = &handle_signal;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("  Gestion signaux impossible\n");
        return ERR_HANDLE_SIG;
    }

    /* Création socket client */
    if ((main_socket = socket(sock_domain, sock_type, sock_protocol)) == -1) {
        perror("  Création du socket impossible\n");
        return ERR_CREATE_SOCKET;
    }

    /* Création de la connection */
    if (connect(main_socket, (struct sockaddr*)&main_addr_server,sizeof(main_addr_server)) == -1) {
        close(main_socket);
        perror("  Connexion au serveur impossible\n");
        return ERR_CONNECT_SERVER;
    }

    /* Attente de l'acquittement connection */
    wait_acquit();

    char acquit_msg[4];
    snprintf(acquit_msg, sizeof(acquit_msg), "ACK");
    send(main_socket, acquit_msg, strlen(acquit_msg) + 1, 0);

    if (recv(main_socket, buffer, sizeof(buffer), 0) > 0) {
        printf("%s", buffer);
    }

    printf("Saisir votre choix : ");
    fgets(buffer, sizeof(buffer), stdin);
    send(main_socket, buffer, strlen(buffer) + 1, 0);

    int nb_bytes = recv(main_socket, buffer, sizeof(buffer) - 1, 0);
    if (nb_bytes <= 0) {
        printf("Erreur: Le serveur ne répond pas\n");
        close(main_socket);
        // Mettre en place un code erreur
        return -1;
    }
    buffer[nb_bytes] = '\0';

    if (strcmp(buffer, "Saisir le nom du channel :") == 0) {
        printf("Saisir le nom du channel : ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer) - 1] = '\0';
        send(main_socket, buffer, strlen(buffer) + 1, 0);

        char buff[buffer_size];
        printf("buff :%s:\n", buff);
        sleep(1);

        int nb_byt = recv(main_socket, buff, sizeof(buff), 0);
        if (nb_byt <= 0) {
            printf("Erreur: Le serveur ne répond pas\n");
            close(main_socket);
            // Mettre en place un code erreur
            return -1;
        }
        buff[nb_byt] = '\0';
        printf("nb char: %d\n", nb_byt);
        printf("response :%s:\n", buff);
        
        sleep(1);
        char channel_ip[50];
        int channel_port; 
        sscanf(buff, "%[^:]:%d", channel_ip, &channel_port);

        //sleep(1);
        printf("Vous êtes connecté sur %s:%d\n", channel_ip, channel_port);
        connect_on_channel(channel_ip, channel_port);
    } else {
        char channel_ip[50];
        int channel_port; 
        sscanf(buffer, "%[^:]:%d", channel_ip, &channel_port);

        printf("Vous êtes connecté sur %s:%d\n", channel_ip, channel_port);
        connect_on_channel(channel_ip, channel_port);
    }

    /* Fermeture de la connexion */
    close(main_socket);
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
        if (read(main_socket, &acquit_msg, 4) < 0) {
            close(main_socket);
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
    shutdown(main_socket, 2);
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

void connect_on_channel(const char* ip, int port) {
    int channel_socket;
    struct sockaddr_in channel_addr;
    char buffer[buffer_size];
    pthread_t receive_thread;

    channel_socket = socket(sock_domain, sock_type, 0);
    if (channel_socket < 0) {
        perror("Une erreur réseau est survenue\n");
        exit(EXIT_FAILURE);
    }

    channel_addr.sin_family = sock_domain;
    channel_addr.sin_addr.s_addr = inet_addr(ip);
    channel_addr.sin_port = htons(port);

    if (connect(channel_socket, (struct sockaddr*)&channel_addr, sizeof(channel_addr)) < 0) {
        perror("Connexion au channel échouée\n");
        close(channel_socket);
        exit(EXIT_FAILURE);
    }

    printf("Vous êtes connecté sur %s:%d\n", ip, port);

    pthread_create(&receive_thread, NULL, (void*)receive_msg, &channel_socket);

    while (off_send == 0) {
        fgets(buffer, sizeof(buffer), stdin);
        if (strcmp(buffer, "/quitter\n") == 0) {
            printf("Déconnexion du channel en cours..\n");
            break;
        }

        printf("\033[A\33[2K\r");
        if (off_send == 0) {
            send(channel_socket, buffer, strlen(buffer) + 1, 0);
        }
    }

    pthread_cancel(receive_thread);
    shutdown(channel_socket, 2);
    close(channel_socket);
}