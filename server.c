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

// Paramètyres généraux
const int buffer_size = 1024;
const char acquit_msg[4] = "ACK";

// Code erreur de l'application
enum error_type {
    ERR_TOO_MUCH_ARG = -30, ERR_CREATE_SOCKET,
    ERR_BIND_SOCKET, ERR_LISTEN_SOCKET,
    ERR_CONNECT_SERVER, ERR_READ_ACK, 
    ERR_HANDLE_SIG, ERR_READ_CONFIG_FILE,
    ERR_READ_CONFIG, ERR_READ_CHANNELS_FILE,
    ERR_READ_CHANNELS, ERR_IP_ALLOC, 
    ERR_PORT_ALLOC
};

/*
    Variables
*/

typedef struct {
    int socket;
    const char* channel_name;
} Client;

Client clients[30];
char* channels[10];
char* menu = NULL;

int client_count = 0;
int off_server = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; 

/*
    Déclaration des fonctions
*/

void handle_signal(int sig);
void send_acquit(int *socket);

void get_config(FILE* file, char* ip, char* port);
void create_menu(FILE* file, char* menu, int* nb_channels, int max_channels);
struct sockaddr_in create_addr(char* ip, char* port);

int get_menu_choice(int socket, char* response, int nb_channels);

void save_msg(const char* channel_name, const char* msg);
void send_history(int socket, const char* channel_name);
void add_time_msg(char* buffer, const char* msg);
void broadcast_msg(const char* channel_name, const char* message);

void add_client(int socket, const char* channel_name);
void remove_client(int socket);
void handle_client(void* temp_client);

/*
    Main
*/

int main(int argc, char *argv[]) {
    struct sigaction action;
    struct sockaddr_in addr_client;
    struct sockaddr_in addr_server;

    socklen_t addr_len = sizeof(addr_client);
    pthread_attr_t attr;
    
    char* addr_inet = NULL;
    char* addr_port = NULL;

    int *new_socket;
    int max_channels = 10;
    int nb_channels = 0;

    addr_inet = (char*)malloc(sizeof(char) * 100);
    if (addr_inet == NULL) {
        perror("[ERROR] Allocation adresse ip impossible\n");
        return ERR_IP_ALLOC;
    }

    addr_port = (char*)malloc(sizeof(char) * 30);
    if (addr_inet == NULL) {
        perror("[ERROR] Allocation port impossible\n");
        return ERR_PORT_ALLOC;
    }

    FILE* file = fopen("settings/server.conf", "r");
    if (file == NULL) {
        perror("[ERROR] Lecture fichier configuration impossible\n");
        return ERR_READ_CONFIG_FILE;
    }

    get_config(file, addr_inet, addr_port);
    fclose(file);
    addr_server = create_addr(addr_inet, addr_port);
    printf("[INFO] Démarrage du serveur\n");

    action.sa_handler = &handle_signal;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("[ERROR] Gestion signaux impossible\n");
        return ERR_HANDLE_SIG;
    }

    int socket_server = socket(AF_INET, SOCK_STREAM, 0);
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
    printf("[CONFIG] %s:%s\n", addr_inet, addr_port);

    free(addr_inet);
    free(addr_port);

    file = fopen("settings/channels.conf", "r");
    if (file == NULL) {
        close(socket_server);
        perror("[ERROR] Lecture fichier channels impossible\n");
        return ERR_READ_CHANNELS_FILE;
    }
    
    menu = (char*)malloc(sizeof(char) * buffer_size);
    if (menu == NULL) {
        perror("[ERROR] Allocation pour reception choix client impossible\n");
        close(socket_server);
        return -1;
    }
    menu[0] = '\0';
    create_menu(file, menu, &nb_channels, max_channels);
    fclose(file);

    if (nb_channels < 1) {
        close(socket_server);
        perror("[ERROR] Lecture channels impossible\n");
        return ERR_READ_CHANNELS;
    }

    while (off_server == 0) {
        new_socket = malloc(sizeof(int));
        *new_socket = accept(socket_server, (struct sockaddr*)&addr_client, &addr_len);
        if (*new_socket < 0 && off_server == 0) {
            perror("[ERROR] Dialogue client impossible\n");
            free(new_socket);
            continue;
        }

        if (off_server == 0) {
            char buffer2[buffer_size];
            if (send((*new_socket), menu, strlen(menu) + 1, 0) < 0) {
                close((*new_socket));
                free(new_socket);
                continue;
            }
            
            int choice = get_menu_choice((*new_socket), buffer2, nb_channels);
            if ( choice < 0) {
                close((*new_socket));
                free(new_socket);
                continue;
            } 

            Client* client = (Client*)malloc(sizeof(Client));
            if (client == NULL) {
                close((*new_socket));
                free(new_socket);
                continue;
            }

            (*client).socket = (*new_socket);
            (*client).channel_name = channels[choice];
            add_client((*new_socket), channels[choice]);
            free(new_socket);

            pthread_t thread_id;
            pthread_attr_init(&attr);
            pthread_create(&thread_id, &attr, (void*)handle_client, (void*)client);
            pthread_detach(thread_id);
        }
    }

    close(socket_server);
    free(menu);
    printf("[INFO] Arrêt du serveur\n");
    return EXIT_SUCCESS;
}

/*
    Définition des fonctions
*/

void handle_signal(int signal) {
    if (signal == SIGINT) { off_server = 1; }
}

void broadcast_msg(const char* channel_name, const char* message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].channel_name, channel_name) == 0) {
            if (send(clients[i].socket, message, strlen(message) + 1, 0) < 0) {
                perror("[ERROR] Diffusion du message impossible\n");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void get_config(FILE* file, char* ip, char* port) {
    char line[100];
    int nb_line = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        if (nb_line == 0) { strncpy(ip, line, 100);  } 
        else if (nb_line == 1) { strncpy(port, line, 30); }
        nb_line ++; 
    }
}

void create_menu(FILE* file, char* menu, int* nb_channels, int max_channels) {
    char line[100];
    while (fgets(line, sizeof(line), file) && (*nb_channels) <= max_channels) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        int channel_id = ((*nb_channels) + 1);
        char number[14];
        char* channel_name = strdup(line);
        channels[(*nb_channels)] = channel_name;
        snprintf(number, sizeof(number), "%d) ", channel_id);
        strcat(menu, number);
        strcat(menu, channels[(*nb_channels)]);
        strcat(menu, "\n");
        (*nb_channels) += 1;
    }
}

struct sockaddr_in create_addr(char* ip, char* port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(atoi(port));
    return addr; 
}

int get_menu_choice(int socket, char* response, int nb_channels) {
    int choice = -1;
    int nb_bytes = read(socket, response, sizeof(response));
    if (nb_bytes <= 0) { return choice; }
    response[nb_bytes] = '\0';
    choice = atoi(response) - 1;
    if (choice >= nb_channels || choice < 0) { return -1; }
    return choice;
}

void add_client(int socket, const char* channel_name) {
    pthread_mutex_lock(&clients_mutex);
    clients[client_count].socket = socket;
    clients[client_count].channel_name = channel_name; 
    client_count ++;
    pthread_mutex_unlock(&clients_mutex);
    printf("[INFO] client %d connecté\n", socket);
}

void remove_client(int socket) {
    int i = 0;
    pthread_mutex_lock(&clients_mutex);
    while (i < client_count) {
        if (clients[i].socket == socket) {
            clients[i] = clients[--client_count];
            break;
        }
        i ++;
    }
    pthread_mutex_unlock(&clients_mutex);
    close(socket);
    printf("[INFO] Client %d déconnecté\n", socket);
}

void handle_client(void *temp_client) {
    int socket = (*(Client*)temp_client).socket;
    const char* channel_name = (*(Client*)temp_client).channel_name;
    free(temp_client);

    char msg[buffer_size];
    char msg_date[buffer_size];
    send_history(socket, channel_name);

    while (1) {
        int nb_bytes = recv(socket, msg, sizeof(msg) - 1, 0);
        if (nb_bytes <= 0) { break; }
        msg[nb_bytes] = '\0';
        if (strcmp(msg, "/quitter\n") == 0) { break; }
        add_time_msg(msg_date, msg);
        save_msg(channel_name, msg_date);
        broadcast_msg(channel_name, msg_date);
    }

    remove_client(socket);
    pthread_exit(NULL);
}

void save_msg(const char* channel_name, const char* msg) {
    char path[70];
    pthread_mutex_lock(&clients_mutex);
    snprintf(path, sizeof(path), "data/%s_h.txt", channel_name);
    FILE* file = fopen(path, "a");
    if (file != NULL) { fprintf(file, "%s", msg); } 
    else { perror("[ERROR] Sauvegarde message impossible\n"); }
    fclose(file);
    pthread_mutex_unlock(&clients_mutex);
}

void send_history(int socket, const char* channel_name) {
    char path[70];
    pthread_mutex_lock(&clients_mutex);
    snprintf(path, sizeof(path), "data/%s_h.txt", channel_name);

    FILE* file = fopen(path, "r");
    if (file == NULL) { file = fopen(path, "w+"); }

    if (file != NULL) {
        char line[buffer_size];
        while (fgets(line, sizeof(line), file) != NULL) {
            send(socket, line, strlen(line), 0);
        }
    } else { perror("[ERROR] Recuperation historique impossible"); }
    
    fclose(file);
    pthread_mutex_unlock(&clients_mutex);
}

void add_time_msg(char* buffer, const char* msg) {
    time_t t = time(NULL);
    snprintf(buffer, buffer_size, "%ld : %s", t, msg);
}

void send_acquit(int *socket) {
    if (send((*socket), acquit_msg, strlen(acquit_msg), 0) < 0) {
        perror("[ERROR] Envoi acquittement impossible\n");
        close((*socket));
    } else { printf("[INFO] Envoi acquittement\n"); }
}