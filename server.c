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
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>

/*
    Constantes
*/

// Paramètres socket
const int sock_domain = AF_INET;
const int sock_type = SOCK_STREAM;
const int sock_protocol = 0;
const int port = 8080;
const char addr_inet[10] = "127.0.0.1";

// Paramètyres généraux
const int buffer_size = 1024;
const int max_channels = 10;
const char acquit_msg[4] = "ACK";

// Code erreur de l'application
enum error_type {
    ERR_TOO_MUCH_ARG = 2, ERR_CREATE_SOCKET,
    ERR_BIND_SOCKET, ERR_LISTEN_SOCKET,
    ERR_CONNECT_SERVER, ERR_READ_ACK, 
    ERR_HANDLE_SIG
};

/*
    Variables
*/

int socket_server;
int off_server = 0;
time_t current_time;
struct tm * m_time; 
int id_clients = 0;

typedef struct {
    int id;
    const char* nom;
} Channel;

typedef struct {
    int id;
    int socket;
    const Channel* channel;
} Client;

Client clients[30];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; 

/*
    Déclaration des fonctions
*/
void handle_signal(int sig);
void send_acquit(int *socket);
void shut_down(void);
void setup_addr(int argc, char *argv[], struct sockaddr_in *addr);
void handle_client(void *socket_client);

void get_time(void);
void save_msg(int client_id, const char* msg);
void send_history(int client_id);
void add_date_msg(char* buffer, const char* msg);

void broadcast_msg(int client_id, const char* message);
void remove_client(int client_socket);
void handle_client(void *socket_desc);

/*
    Main
*/

int main(int argc, char *argv[]) {
    struct sigaction action;
    struct sockaddr_in addr_client, addr_server;
    socklen_t addr_len = sizeof(addr_client);
    pthread_attr_t attr;
    int *new_socket;
    
    printf("[INFO] Démarrage du serveur\n");

    if (argc > 3) {
        printf("[ERROR] Trop d arguments renseignés \n");
        printf("[INFO] Arrêt du serveur\n");
        return ERR_TOO_MUCH_ARG;
    }

    /* Configuration de l'adresse du serveur */
    setup_addr(argc, argv, &addr_server);

    /* Gestion des signaux */
    action.sa_handler = &handle_signal;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("[ERROR] Gestion signaux impossible\n");
        return ERR_HANDLE_SIG;
    }

    socket_server = socket(sock_domain, sock_type, sock_protocol);
    if (socket_server < 0) {
        perror("[ERROR] Création socket impossible\n");
        return ERR_CREATE_SOCKET;
    }

    if (bind(socket_server, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0) {
        close(socket_server);
        perror("[ERROR] Liaison socket impossible\n");
        return ERR_BIND_SOCKET;
    }

    if (listen(socket_server, 5) < 0) {
        close(socket_server);
        perror("[ERROR] Ecoute impossible\n");
        return ERR_LISTEN_SOCKET;
    }

    printf("[INFO] Démarrage ecoute connexion\n");
    printf("[CONFIG] %s:%d\n", addr_inet, port);

    while (off_server == 0) {
        new_socket = malloc(sizeof(int));
        *new_socket = accept(socket_server, (struct sockaddr*)&addr_client, &addr_len);
        if (*new_socket < 0 && off_server == 0) {
            perror("[ERROR] Dialogue client impossible\n");
            free(new_socket);
            continue;
        }

        if (off_server == 0) {
            /* Acquittement connexion */
            send_acquit(new_socket);

            pthread_t thread_id;
            pthread_attr_init(&attr);
            pthread_create(&thread_id, &attr, (void*)handle_client, (void*)new_socket);
            pthread_detach(thread_id);
        }
    }

    close(socket_server);
    printf("[INFO] Arrêt du serveur\n");
    return EXIT_SUCCESS;
}

/*
    Définition des fonctions
*/

void handle_signal(int signal) {
    if (signal == SIGINT) { shut_down(); }
}

void send_acquit(int *socket) {
    if (send((*socket), acquit_msg, strlen(acquit_msg), 0) < 0) {
        perror("[ERROR] Envoi acquittement impossible\n");
        close((*socket));
    } else { printf("[INFO] Envoi acquittement\n"); }
}

void shut_down(void) {
    off_server = 1;
    // Gérer l'envoi d'un message aux clients
    printf("\n");
    close(socket_server);
}

void get_time(void) {
    time(&current_time);
    m_time = localtime(&current_time);
}

void broadcast_msg(int client_id, const char* message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (strcmp((*clients[i].channel).nom, (*clients[client_id].channel).nom) == 0) {
            if (send(clients[i].socket, message, strlen(message) + 1, 0) < 0) {
                perror("[ERROR] Diffusion du message impossible\n");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

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
    }
}

void remove_client(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == client_socket) {
            clients[i] = clients[--client_count];
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    close(client_socket);
}

void handle_client(void *socket_desc) {
    int client_socket = *(int*)socket_desc;
    free(socket_desc);

    char buffer[buffer_size];
    char buffer_msg_date[buffer_size];

    char channel_nom[50] = "general"; 
    Channel general;
    general.id = 1;
    general.nom = channel_nom;

    char channel_deux[50] = "sport"; 
    Channel deux;
    deux.id = 2;
    deux.nom = channel_deux;

    pthread_mutex_lock(&clients_mutex);
    clients[client_count].id = client_count;
    int client_id = client_count;
    if (client_id == 2) {
        clients[client_count].channel = &deux;
    } else {
        clients[client_count].channel = &general;
    }
    clients[client_count].socket = client_socket;
    client_count ++;
    pthread_mutex_unlock(&clients_mutex);

    printf("[INFO] client %d connecté\n", client_id);
    snprintf(buffer, sizeof(buffer), "Bienvenue sur WeeChat !\n\n");
    send(client_socket, buffer, strlen(buffer), 0);
    send_history(client_id);

    while (1) {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("[INFO] Client %d déconnecté\n", client_id);
            remove_client(client_socket);
            break;
        } else {
            buffer[bytes_received] = '\0';
            add_date_msg(buffer_msg_date, buffer);
            save_msg(client_id, buffer_msg_date);
            broadcast_msg(client_id, buffer_msg_date);
        }
    }
    
    pthread_exit(NULL);
}

void save_msg(int client_id, const char* msg) {
    char file_name[50];

    pthread_mutex_lock(&clients_mutex);
    strcpy(file_name, clients[client_id].channel->nom);
    pthread_mutex_unlock(&clients_mutex);

    strcat(file_name, "_h.txt");
    FILE* file = fopen(file_name, "a");

    if (file != NULL) {
        fprintf(file, "%s", msg);
    } else { perror("[ERROR] Sauvegarde message impossible\n"); }
    fclose(file);
}


void send_history(int client_id) {
    char channel_name[60];
    int socket_val;

    pthread_mutex_lock(&clients_mutex);
    strcpy(channel_name, clients[client_id].channel->nom);
    socket_val = clients[client_id].socket;
    pthread_mutex_unlock(&clients_mutex);
    
    strcat(channel_name, "_h.txt");
    FILE* file = fopen(channel_name, "r");

    if (file != NULL) {
        char line[buffer_size];
        while (fgets(line, sizeof(line), file) != NULL) {
            send(socket_val, line, strlen(line), 0);
        }
    } else { perror("[ERROR] Recuperation historique impossible"); }
    fclose(file);
}

void add_date_msg(char* buffer, const char* msg) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(buffer, 75, "%d-%02d-%02d %02d:%02d:%02d : ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    strcat(buffer, msg);
}