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
const int buffer_size = 512;

enum msg_config { 
    DEFAULT, PORT_MISSING, TOO_MUCH 
};

// Code erreur de l'application
enum error_type {
    ERR_TOO_MUCH_ARG = -6, // Trop d'arguments passés lors du lancement de l'application
    ERR_CREATE_SOCKET,     // Problème lors de la création du socket
    ERR_CONNECT_SERVER,    // Problème lors de la connection au serveur principal
    ERR_READ_ACK,          // Problème lors de la lecture de l'acquittement reçu du serveur
    ERR_SEND_ACK,          // Problème lors de l'envoi de l'acquittement reçu du serveur
    ERR_HANDLE_SIG,        // Problème lors de l'appel à la fonction sigaction
    ERR_ALLOC              // Problème lors de l'allocation du buffer principal 
};

/*
    Variables
*/

int main_socket;
int channel_socket;
int off_client = 0;
int off_recv = 0;
int off_send = 0;
int nb_bytes = 0;
char* buffer = NULL;

/*
    Déclaration des fonctions
*/

/**
 * Cette fonction permet de configurer l'adresse
 * du socket du serveur principal (main_socket).
 *
 * argc : nombre d'arguments 
 * argv : les valeurs des arguments
 * addr : adresse du socket
 */
void setup_addr(int argc, char *argv[], struct sockaddr_in *addr);

/**
 * Cette fonction permet d'attendre un message d'acquittement
 * reçu au serveur principal.
 */
void wait_acquit(void);

/**
 * Cette fonction permet d'envoyer un message d'acquittement
 * au serveur principal.
 */
void send_acquit(void);

/**
 * Cette fonction gère la réception des messages envoyés
 * au client par le socket passé en parmaètre. 
 *
 * socket : le socket sur lequel réceptionner les messages
 */
void receive_msg(void* socket);

/**
 * Cette fonction permet de récupérer le signal 
 * envoyé au processus pour ensuite fermer les connections
 * et libérer les ressources.
 * 
 * sig : numéro du signal  
 */
void handle_signal(int signal);

/**
 * Ces trois fonctions affichent des messages sur
 * en fonction de la configuration lors du lancement
 * du programme.
 */
void msg_default_config(void);
void msg_port_missing(void);
void msg_too_much(void);

/**
 * Cette fonction gère la connection au channel puis 
 * via un thread elle gère la réception des messages 
 * reçus du channel, et gère la récupération et envoi
 * du message de l'utilisateur.
 *
 * ip : l'adresse ip du channel sur lequel se connecter
 * port : le port du channel sur lequel se connecter
 */
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

    /* Allocation pour le buffer principal */
    buffer = (char*)malloc(sizeof(char) * buffer_size);
    if (buffer == NULL) {
        perror("  Allocation buffer principal impossible\n");
        return ERR_ALLOC;
    }

    /* Création de la connection */
    if (connect(main_socket, (struct sockaddr*)&main_addr_server,sizeof(main_addr_server)) == -1) {
        close(main_socket);
        perror("  Connexion au serveur impossible\n");
        return ERR_CONNECT_SERVER;
    }

    /* Attente de l'acquittement connection */
    wait_acquit();

    /* Envoi de l'acquittement */
    send_acquit();

    /* Réception du menu principal */
    nb_bytes = read(main_socket, buffer, buffer_size);
    if (nb_bytes > 0) {
        printf("%s", buffer);
    }

    /* Saisi et envoi du choix de l'utilisateur */
    printf("Saisir votre choix : ");
    fgets(buffer, sizeof(buffer), stdin);
    write(main_socket, buffer, strlen(buffer) + 1);

    /* Réception de l'ip et port du channel si l'utilisateur
       a choisi un channel, sinon c'est qu'il a demandé
       de créer un channel */
    nb_bytes = read(main_socket, buffer, buffer_size);
    if (nb_bytes <= 0) {
        printf("Erreur: Le serveur ne répond pas\n");
        close(main_socket);
        return -1;
    }
    buffer[nb_bytes] = '\0';

    /* Saisi et envoi du nom du channel */
    if (strcmp(buffer, "Saisir le nom du channel :") == 0) {
        printf("%s", buffer);
        fgets(buffer, buffer_size, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        if (write(main_socket, buffer, strlen(buffer) + 1) < 0) { 
            printf("Erreur: Impossible de transmettre le nom du channel\n");
            close(main_socket);
            return -1;
        }

        int nb_byt = read(main_socket, buffer, buffer_size);
        if (nb_byt <= 0) {
            printf("Erreur: Le serveur ne répond pas\n");
            close(main_socket);
        }
        buffer[nb_byt] = '\0';
    }

    /* Extraction de l'adresse ip et du port du channel */
    char channel_ip[50];
    int channel_port; 
    sscanf(buffer, "%[^:]:%d", channel_ip, &channel_port);

    /* Fermeture de la connexion avec le serveur principal */
    sleep(1);
    close(main_socket);
    free(buffer);

    connect_on_channel(channel_ip, channel_port);

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
    strcpy(acquit_msg, "");
}

void send_acquit(void) {
    char acquit_msg[4];
    snprintf(acquit_msg, sizeof(acquit_msg), "ACK");
    if (send(main_socket, acquit_msg, strlen(acquit_msg) + 1, 0) < 0) {
        close(main_socket);
        perror("  Envoi acquittement impossible\n");
        exit(ERR_SEND_ACK);
    }
}
 
void receive_msg(void* socket) {
    int socket = *(int*)socket;
    char* buffer = malloc((buffer_size) * sizeof(char)); 

    if (buffer != NULL) {
        while((nb_bytes = recv(socket, buffer, buffer_size - 1, 0)) > 0) {
            if (off_recv == 1) { break; }
            if(nb_bytes <= 0) {
                printf("  Lecture des messages impossible\n"); 
                break; 
            } else {
                buffer[nb_bytes] = '\0';                     
                printf("%s", buffer);
            }
        }

        free(buffer);
    }

    printf("  Arrêt reception message\n");
    pthread_exit(NULL);
}

void handle_signal(int signal) {
    if (signal == SIGINT) { 
        free(buffer);
        shutdown(channel_socket, 2);
        close(channel_socket);
        shutdown(main_socket, 2);
        close(main_socket);
        off_recv = 1;
        off_send = 1;
        printf("\n  Arrêt de l'application en cours ...\n");
    }
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
    struct sockaddr_in channel_addr;
    pthread_t receive_thread;

    /* Allocation pour le buffer channel */
    char* buffer_channel = (char*)malloc(sizeof(char) * buffer_size);
    if (buffer_channel == NULL) {
        perror("  Allocation buffer channel impossible\n");
        exit(ERR_ALLOC);
    }

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

    size_t msg_size = 0;

    printf("Vous êtes connecté sur %s:%d\n", ip, port);
    printf("Pour quitter saisir /quitter\n");

    pthread_create(&receive_thread, NULL, (void*)receive_msg, &channel_socket);

    while (off_send == 0) {
        fgets(buffer_channel, buffer_size - 1, stdin);
        if (strcmp(buffer_channel, "/quitter\n") == 0) {
            off_send = 1;
            off_recv = 1;
            printf("Déconnexion du channel en cours..\n");
            break;
        }

        printf("\033[A\33[2K\r");
        if (off_send == 0) {
            msg_size = strlen(buffer_channel) + 1;
            if (write(channel_socket, &msg_size, sizeof(size_t)) < 0) {
                printf("Envoi taille du message impossible\n");
                continue;
            }

            if (write(channel_socket, buffer_channel, strlen(buffer_channel) + 1) < 0) {
                printf("Envoi du message impossible\n");
                continue;
            }
        }
    }

    free(buffer_channel);
    pthread_cancel(receive_thread);
    shutdown(channel_socket, 2);
    close(channel_socket);
}