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

int socket_server;
int off_server = 0;
time_t current_time;
struct tm * m_time; 
int id_clients = 0;

typedef struct {
    int id;
    int socket;
    const char* channel_name;
} Client;

Client clients[30];
char* channels[10];
char* menu = NULL;

int client_count = 0;
int* nb_channels = NULL;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; 

/*
    Déclaration des fonctions
*/
void handle_signal(int sig);
void send_acquit(int *socket);
void shut_down(void);

void get_config(FILE* file, char* ip, char* port);
void create_menu(FILE* file, char* menu, int* nb_channels, int max_channels);
struct sockaddr_in create_addr(char* ip, char* port);
void handle_client(void *socket_client);

void get_time(void);
void save_msg(int client_id, const char* msg);
void send_history(int client_id);
void add_date_msg(char* buffer, const char* msg);

void broadcast_msg(int client_id, const char* message);
void remove_client(int client_socket);
void handle_client(void *socket_desc);
//void free_channels(char* channels, int count);

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

    addr_inet = (char*)malloc(sizeof(char) * 100);
    if (addr_inet == NULL) {
        perror("[ERROR] Allocation adresse ip impossible\n");
        close(socket_server);
        return ERR_IP_ALLOC;
    }

    addr_port = (char*)malloc(sizeof(char) * 30);
    if (addr_inet == NULL) {
        perror("[ERROR] Allocation port impossible\n");
        close(socket_server);
        return ERR_PORT_ALLOC;
    }

    FILE* file = fopen("settings/server.conf", "r");
    if (file == NULL) {
        close(socket_server);
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

    socket_server = socket(AF_INET, SOCK_STREAM, 0);
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

    nb_channels = (int*)malloc(sizeof(int) * 2);
    if (nb_channels == NULL) {
        perror("[ERROR] Allocation adresse ip impossible\n");
        close(socket_server);
        return ERR_IP_ALLOC;
    }
    (*nb_channels) = 0;

    create_menu(file, menu, nb_channels, max_channels);
    fclose(file);

    if ((*nb_channels) < 1) {
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
            send_acquit(new_socket);
            pthread_t thread_id;
            pthread_attr_init(&attr);
            pthread_create(&thread_id, &attr, (void*)handle_client, (void*)new_socket);
            pthread_detach(thread_id);
        }
    }

    close(socket_server);
    free(menu);
    //free_channels(channels, nb_channels);
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
        if (strcmp(clients[i].channel_name, clients[client_id].channel_name) == 0) {
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
        printf("%d\n", (*nb_channels));
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

void remove_client(int client_socket) {
    int i = 0;
    pthread_mutex_lock(&clients_mutex);
    while (i < client_count) {
        if (clients[i].socket == client_socket) {
            clients[i] = clients[--client_count];
            break;
        }
        i ++;
    }
    pthread_mutex_unlock(&clients_mutex);
    close(client_socket);
}

void handle_client(void *socket_desc) {
    int client_socket = *(int*)socket_desc;
    free(socket_desc);

    int client_id = 0;
    char buffer[buffer_size];
    char buffer2[buffer_size];
    char buffer_msg_date[buffer_size];

    if (send(client_socket, menu, strlen(menu) + 1, 0) < 0) {
        close(client_socket);
    }
    
    int bytes_received = read(client_socket, buffer2, sizeof(buffer2));
    if (bytes_received <= 0) {
        close(client_socket);
    }
    
    buffer2[bytes_received] = '\0';
    int choice = atoi(buffer2);

    pthread_mutex_lock(&clients_mutex);
    if (choice > (*nb_channels) || choice <= 0) { choice = 0; }
    client_id = client_count;
    clients[client_count].id = client_count;
    clients[client_count].channel_name = channels[choice - 1];
    clients[client_count].socket = client_socket;
    client_count ++;
    pthread_mutex_unlock(&clients_mutex);

    printf("[INFO] client %d connecté\n", client_id);
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
    char path[70];
    pthread_mutex_lock(&clients_mutex);
    snprintf(path, sizeof(path), "data/%s_h.txt", clients[client_id].channel_name);
    pthread_mutex_unlock(&clients_mutex);

    FILE* file = fopen(path, "a");
    if (file != NULL) { fprintf(file, "%s", msg); } 
    else { perror("[ERROR] Sauvegarde message impossible\n"); }
    fclose(file);
}

void send_history(int client_id) {
    char path[70];
    int socket;

    pthread_mutex_lock(&clients_mutex);
    snprintf(path, sizeof(path), "data/%s_h.txt", clients[client_id].channel_name);
    socket = clients[client_id].socket;
    pthread_mutex_unlock(&clients_mutex);
    
    FILE* file = fopen(path, "r");
    if (file == NULL) { file = fopen(path, "w+"); }

    if (file != NULL) {
        char line[buffer_size];
        while (fgets(line, sizeof(line), file) != NULL) {
            send(socket, line, strlen(line), 0);
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
