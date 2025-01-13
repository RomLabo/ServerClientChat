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
const int buffer_size = 512;
const int max_channels = 10;
const char acquit_msg[8] = "SYN-ACK";
const char exit_msg[4] = "EXT";
const int path_history_size = 140;
const int line_history_size = 264;
const int date_size = 100;
const int info_channel_size = 32;

// Paramètres stockage clients
const int clients_min_size = 6;
const int clients_gap_size = 4;

// Code erreur de l'application
enum error_type {
    ERR_TOO_MUCH_ARG = -20,    // Trop d'arguments passés lors du lancement de l'application
    ERR_CREATE_SOCKET,         // Problème lors de la création du socket
    ERR_BIND_SOCKET,           // Problème lors l'assignation du socket à une adresse
    ERR_LISTEN_SOCKET,         // Problème lors de l'écoute du socket
    ERR_READ_ACK,              // Problème lors de la lecture de l'acquittement
    ERR_HANDLE_SIG,            // Problème lors de l'appel à la fonction sigaction
    ERR_READ_CHANNELS,         // Problème pour lire les noms des channels (channels.txt)
    ERR_CLIENTS_STORAGE,       // Problème lors de l'allocation pour le stockage des utilisateurs
    ERR_ADD_CLIENT,            // Problème lors de l'ajout d'un nouveau utilisateur
    ERR_MAX_CHANNELS,          // Nombre de channels maximum atteint
    ERR_CREATE_CHANNEL,        // Problème lors de la création d'un nouveau channel
    ERR_CREATE_CHANNEL_SOCKET, // Problème lors de la création du socket pour un channel
    ERR_BIND_CHANNEL_SOCKET,   // Problème lors de l'assignation du socket channel à une adresse
    ERR_LISTEN_CHANNEL_SOCKET, // Problème lors de l'écoute du socket d'un channel
    ERR_SEND_HISTORY,          // Problème lors de l'envoi des messages de l'historique
    ERR_READ_HISTORY,          // Problème lors da lecture des messages de l'historique
    ERR_WRITE_HISTORY          // Problème lors de l'ajout d'un message dans l'historique
};

/*
    Variables
*/

int main_socket;
int off_server = 0;
int clients_curr_size = 6;
int* clients = NULL;
int clients_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; 
pid_t child_pids[10];
int child_count = 0;

typedef struct {
    char name[50];
    char ip[18];
    int port;
} Channel;

typedef struct {
    int socket;
    const char* channel_name;
} Client;

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
 * Cette fonction permet de récupérer le signal 
 * envoyé au processus pour ensuite appeler 
 * les fonctions adéquates.
 * 
 * sig : numéro du signal  
 */
void handle_signal(int sig);

/**
 * Cette fonction permet d'envoyer l'historique 
 * des messages d'un channel à un client.
 *
 * client : pointeur de type Client
 */
int send_history(Client* client);

/**
 * Cette fonction permet de dater le message
 * qu'un utilisateur souhaite envoyer dans le channel.
 * 
 * buffer_msg_date : le buffer qui contiendra le message daté
 * buffer_msg : le buffer contenant le message de l'utilisateur
 * size_msg_date : la taille alloué pour le buffer
 */
void add_date_msg(char* buffer_msg_date, char* buffer_msg, size_t size_msg_date);

/**
 * Cette fonction permet de sauvegarder le message
 * de l'utilisateur dans une fichier qui représente l'historique 
 * des messages du channel.
 * 
 * channel_name : nom du channel 
 * msg : message de l'utilisateur à sauvegarder
 */
int save_msg(const char* channel_name, const char* msg);

/**
 * Cette fonction permet de diffuser le message envoyé
 * par un utilisateur à tout les utilisateurs connectés
 * au channel.
 * 
 * msg : message de l'utilisateur à diffuser
 */
void broadcast_msg(const char* msg);

/**
 * Cette fonction permet d'ajouter un utilisateur dans
 * le tableau de sotckahe des utilisateurs et si besoin
 * augmente la taille alloué pour ce tableau.
 * 
 * all_clients : pointeur vers le pointeur du tableau d'utilisateurs
 * client : pointeur de l'utilisateur à ajouter
 */
int add_client(int** all_clients, int socket);

/**
 * Cette fonction permet de supprimer un utilisateur
 * du tableau des stockage des utilisateurs et si besoin
 * reduit la taille alloué pour ce tableau.
 * 
 * all_clients : pointeur vers le pointeur du tableau d'utilisateurs
 * client : pointeur de l'utilisateur à supprimer
 */
int remove_client(int** all_clients, Client* client);

/**
 * Cette fonction permet de créer le channel en écrivant son nom 
 * et le port associé et construire la chaine (ip:port) qui sera
 * ensuite envoyé au client.
 *
 * channel_name : le nom du channel à créer
 * channel_info : une chaine dans laquelle sera enrgistré (ip:port) 
 */
int create_channel(char* channel_name, char* channel_info, const char* ip, int port);

/**
 * Cette fonction permet d'ajouter un nouveau channel
 * dans le tableau de stockage des channels, puis 
 * configure le channel en lui affectant l'ip, le nom et
 * le port passés en paramètres. 
 * ( Sauf si le nombre de channels maximum a été atteint .)
 * 
 * channels : le tableau de stockage des channels
 * count : le nombre de channels stockés
 * ip : l'adresse ip pour se connecter au channel
 * name : le nom du channel à ajouter
 * port : le port du channel
 */
int add_channel(Channel channels[], int* count, const char* ip, const char* name, int port);

/**
 * Cette fonction permet de gérer un utilisateur qui vient
 * de se connecter au channel, en réceptionnant les messages
 * envoyés puis les sauvegarder et les diffuser au autres 
 * utilisateurs du channel.
 * 
 * client_socket : le socket de l'utilisateur à gérer
 */
void handle_client(void* client_socket);

/**
 * Cette fonction permet de démarrer un channel, en configurant
 * son socket, le mettre en écoute afin qu'il gère les connections
 * des utilisateurs.
 * 
 * name : le nom du channel 
 * ip : l'adresse ip du channel
 * port : le port sur lequel le channel acceptera les connecitons
 */
void start_channel(const char* name, const char* ip, int port);

/**
 * Cette fonction permet d'envoyer un message d'acquittement
 * au socket passé en paramètre.
 * 
 * socket : le socket sur lequel envoyé l'acquittement
 */
void send_acquit(int* socket);

/**
 * Cette fonction permet d'attendre un message d'acquittement
 * reçu sur le socket passé en paramètre.
 * 
 * socket : le socket sur lequel attendre l'acquittement
 */
void wait_acquit(int* socket);

/*
    Main
*/

int main(int argc, char *argv[]) {
    struct sigaction action;
    struct sockaddr_in addr_client, main_addr_server;
    socklen_t addr_len = sizeof(addr_client);
    Channel channels[max_channels];
    int channels_count = 0;
    char ip_server[18];
    int last_port = 0;
    
    if (argc > 1) { strncpy(ip_server, argv[1], sizeof(ip_server)); }
    else { strncpy(ip_server, addr_inet, sizeof(ip_server)); }
    
    printf("[INFO] Démarrage du serveur\n");

    /* Control du nombre d'arguments passé au programme. */
    if (argc > 3) {
        printf("[ERROR] Trop d arguments renseignés \n");
        printf("[INFO] Arrêt du serveur\n");
        return ERR_TOO_MUCH_ARG;
    }

    /* Configuration de l'adresse du serveur */
    setup_addr(argc, argv, &main_addr_server);

    /* Gestion des signaux */
    action.sa_handler = &handle_signal;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("[ERROR] Gestion signaux impossible\n");
        return ERR_HANDLE_SIG;
    }

    /* Création du socket serveur principal */
    main_socket = socket(sock_domain, sock_type, sock_protocol);
    if (main_socket < 0) {
        perror("[ERROR] Création socket impossible\n");
        return ERR_CREATE_SOCKET;
    }

    /* Liaison du socket principal */
    if (bind(main_socket, (struct sockaddr *)&main_addr_server, sizeof(main_addr_server)) < 0) {
        close(main_socket);
        perror("[ERROR] Liaison socket impossible\n");
        return ERR_BIND_SOCKET;
    }

    /* Mise en écoute des demandes de connection */
    if (listen(main_socket, 5) < 0) {
        close(main_socket);
        perror("[ERROR] Ecoute impossible\n");
        return ERR_LISTEN_SOCKET;
    }

    /* Récupération des noms des channels */
    FILE* file = fopen("data/channels.txt", "r");
    if (file == NULL) {
        close(main_socket);
        perror("[ERROR] Lecture channels impossible\n");
        return ERR_READ_CHANNELS;
    }

    printf("[INFO] Démarrage ecoute connexion\n");
    printf("[CONFIG] %s:%d\n", addr_inet, port);

    /* Ajoute et démarrage des channels */
    char line[100];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token == NULL) { continue; }

        char channel_name[100];
        strncpy(channel_name, token, sizeof(channel_name)); 
        token = strtok(NULL, ",");
        if (token == NULL) { continue; }

        int port = atoi(token);
        last_port = port + 1;

        if (add_channel(channels, &channels_count, ip_server, channel_name, port) < 0) {
            printf("[ERROR] Nombre maximal de channels atteint\n");
            continue; 
        }

        pid_t pid = fork();
        if (pid == 0) {
            start_channel(channel_name, ip_server, port);
            exit(0);
        } else if (pid > 0) {
            child_pids[child_count] = pid;
            child_count ++;
        } else { perror("[ERROR] Démarrage des channels impossible\n"); }
    }

    fclose(file);

    /* Boucle principale du serveur principal */
    while (off_server == 0) {
        int* new_client_socket = (int*)malloc(sizeof(int));
        if (new_client_socket == NULL) {
            perror("[ERROR] Allocation socket client impossible\n");
            continue;
        }

        (*new_client_socket) = accept(main_socket, (struct sockaddr*)&addr_client, &addr_len);
        if ((*new_client_socket) < 0 && off_server == 0) {
            perror("[ERROR] Connexion client impossible\n");
            free(new_client_socket);
            continue;
        }

        if (off_server == 0) {
            /* Envoie acquittement connexion au server principal */
            send_acquit(new_client_socket);

            /* Attente acquittement réception du client */
            wait_acquit(new_client_socket);

            
            char* buffer = (char*)malloc(sizeof(char) * buffer_size);
            if (buffer == NULL) {
                perror("[ERROR] Allocation pour reception choix client impossible\n");
                close((*new_client_socket));
                free(new_client_socket);
                continue;
            }

            /* Construction du menu principal */
            snprintf(buffer, buffer_size - 1, "Bienvenue sur ServerClientChat !\n Choisissez un channel :\n");
            for (int i = 0; i < channels_count; i++) {
                char number[20];
                int choiceNum = i + 1;
                snprintf(number, sizeof(number), "%d) ", choiceNum);
                strcat(buffer, number);
                strcat(buffer, channels[i].name);
                strcat(buffer, "\n");
            }
            char lastNumber[40];
            int lastChoiceNum = channels_count + 1;
            snprintf(lastNumber, sizeof(lastNumber), "%d) Créer un channel\n", lastChoiceNum);
            strcat(buffer, lastNumber);

            /* Envoi du menu principal */
            if (write((*new_client_socket), buffer, strlen(buffer) + 1) < 0) {
                perror("[ERROR] Envoi menu impossible\n");
                close((*new_client_socket));
                free(buffer);
                free(new_client_socket);
                continue;
            }

            /* Réception du choix de channel */
            int nb_bytes = read((*new_client_socket), buffer, sizeof(buffer));
            if (nb_bytes <= 0) {
                perror("[ERROR] Réception choix channel impossible\n");
                close((*new_client_socket));
                free(buffer);
                free(new_client_socket);
                continue;
            } 

            buffer[nb_bytes] = '\0';
            int choice = atoi(buffer);

            /* Envoie au client ip et port du channel */
            if (choice > 0 && choice <= channels_count) {
                char port_str[6];
                char temp_addr[26];
                snprintf(port_str, sizeof(port_str), "%d", channels[(choice - 1)].port);
                strncpy(temp_addr, channels[(choice - 1)].ip, sizeof(temp_addr));
                strcat(temp_addr, ":");
                strcat(temp_addr, port_str);
                snprintf(buffer, buffer_size - 1, "%s\n", temp_addr);
            } else if (choice == (channels_count + 1)) {
                snprintf(buffer, buffer_size - 1, "Saisir le nom du channel :");
            } else { 
                snprintf(buffer, buffer_size - 1, "Choix invalide\n");
                choice = -1; 
            }

            if (write((*new_client_socket), buffer, strlen(buffer) + 1) < 0) {
                perror("[ERROR] Envoi info channel impossible\n");
                close((*new_client_socket));
                free(buffer);
                free(new_client_socket);
                continue;
            }

            if (choice <= channels_count) {
                close((*new_client_socket));
                free(buffer);
                free(new_client_socket);
                continue;
            }

            /* Gestion création nouveau channel */
            char* buffer_ip = (char*)malloc(sizeof(char) * info_channel_size);
            if (buffer_ip == NULL) { 
                perror("[ERROR] Allocation pour creation channel impossible\n");
                close((*new_client_socket));
                free(buffer);
                free(new_client_socket);
                continue;
            }

            /* Réception du nom du channel */
            nb_bytes = read((*new_client_socket), buffer, sizeof(buffer));
            if (nb_bytes <= 0) { 
                perror("[ERROR] Réception nom du channel impossible\n");
                close((*new_client_socket));
                free(buffer);
                free(buffer_ip);
                free(new_client_socket);
                continue;
            }

            /* Création du nouveau channel et récupération ip et port */
            if (create_channel(buffer, buffer_ip, ip_server, last_port) < 0) {
                close((*new_client_socket));
                free(buffer);
                free(buffer_ip);
                free(new_client_socket);
                continue;
            }

            /* Ajout du nouveau channel */
            if (add_channel(channels, &channels_count, ip_server, buffer, last_port) < 0) {
                printf("[ERROR] Nombre maximal de channels atteint\n");
                close((*new_client_socket));
                free(buffer);
                free(buffer_ip);
                free(new_client_socket);
                continue;
            }
            
            /* Envoi ip et port nouveau channel */
            if (write((*new_client_socket), buffer_ip, strlen(buffer_ip) +1) < 0) {
                perror("[ERROR] Envoi info nouveau channel impossible\n"); 
            }

            free(buffer_ip);

            pid_t pid = fork();
            if (pid == 0) {
                start_channel(buffer, ip_server, last_port);
                exit(0);
            } else if (pid > 0) {
                child_pids[child_count] = pid;
                child_count ++;              
            } else { perror("[ERROR] Démarrage des channels impossible\n"); }

            close((*new_client_socket));
            free(buffer);
            free(new_client_socket);
        }
    }

    close(main_socket);
    printf("[INFO] Arrêt du serveur\n");
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
    }
}

void handle_signal(int signal) {
    if (signal == SIGCHLD) {
        while (waitpid(-1, NULL, WNOHANG) > 0) { }
    } else if (signal == SIGINT) {
        printf("[INFO] Arrêt serveur principal et channels en cours...\n");
        for (int i = 0; i < child_count; i++) {
            kill(child_pids[i], SIGTERM);
        }

        off_server = 1;
        printf("\n");
        close(main_socket);
    }
}

int send_history(Client* client) {
    int error = 0;

    char* path = (char*)malloc(sizeof(char) * path_history_size);
    if (path == NULL) { return ERR_SEND_HISTORY; }

    char* line = (char*)malloc(sizeof(char) * line_history_size);
    if (line == NULL) {
        free(path); 
        return ERR_SEND_HISTORY; 
    }

    pthread_mutex_lock(&clients_mutex);
    snprintf(path, sizeof(char) * path_history_size, "data/%s", (*client).channel_name);
    strcat(path, "_h.txt");
    FILE* file = fopen(path, "r");

    if (file == NULL) { file = fopen(path, "w+"); }

    if (file != NULL) {
        while (fgets(line, line_history_size -2, file) != NULL) {
            if (write((*client).socket, line, strlen(line) + 1) < 0) {
                error = ERR_SEND_HISTORY;
                break;
            }
        }
    } else { error = ERR_READ_HISTORY; }

    fclose(file);
    free(path);
    free(line);
    pthread_mutex_unlock(&clients_mutex);
    return error; 
}

void add_date_msg(char* buffer_msg_date, char* buffer_msg, size_t size_msg_date) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(buffer_msg_date, size_msg_date, "[%d-%02d-%02d %02d:%02d:%02d] %s", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, buffer_msg);
}

int save_msg(const char* channel_name, const char* msg) {
    int error = 0;

    char* path = (char*)malloc(sizeof(char) * path_history_size);
    if (path == NULL) { return ERR_WRITE_HISTORY; }

    pthread_mutex_lock(&clients_mutex);
    snprintf(path, sizeof(char) * path_history_size, "data/%s", channel_name);
    strcat(path, "_h.txt");
    FILE* file = fopen(path, "a+");

    if (file != NULL) {
        fprintf(file, "%s", msg);
    } else { error = ERR_WRITE_HISTORY; } 
         
    fclose(file);
    free(path);
    pthread_mutex_unlock(&clients_mutex);
    return error;
}

void broadcast_msg(const char* msg) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < clients_count; i++) {
        if (write(clients[i], msg, strlen(msg) + 1) < 0) {
            perror("[ERROR] Diffusion du message impossible\n");
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

int add_client(int** all_clients, int socket) {
    /* Augmentation en mémoire stockage des socket clients */
    if (clients_count >= (clients_curr_size - clients_gap_size)) {
        int new_size = clients_curr_size + clients_min_size;
        (*all_clients) = realloc((*all_clients), new_size * sizeof(int));

        if ((*all_clients) == NULL) { return ERR_ADD_CLIENT; } 
        else { clients_curr_size = new_size; }
    }

    (*all_clients)[clients_count] = socket;
    clients_count ++;
    return 0; 
}

int remove_client(int** all_clients, Client* client) {
    int i = 0;

    pthread_mutex_lock(&clients_mutex);
    while (i < clients_count) {
        if ((*all_clients)[i] == (*client).socket) {
            clients_count --;
            (*all_clients)[i] = (*all_clients)[clients_count];
            break;
        }
        i ++;
    }

    /* Réduction en mémoire stockage des sockets clilents */
    if (clients_count <= (clients_curr_size - clients_gap_size - clients_min_size)) {
        int new_size = clients_curr_size - clients_min_size;
        if (new_size > 0) {
            (*all_clients) = realloc((*all_clients), new_size * sizeof(int));
            if ((*all_clients) == NULL) { return ERR_CLIENTS_STORAGE; } 
            else { clients_curr_size = new_size; }
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    return 0;
}

int create_channel(char* channel_name, char* channel_info, const char* ip, int port) {
    FILE* file = fopen("data/channels.txt", "a");
    if (file == NULL) {
        perror("[ERROR] Création du channel impossible\n");
        return ERR_CREATE_CHANNEL;
    } 

    fprintf(file, "%s,%d\n", channel_name, port);
    fclose(file);

    snprintf(channel_info, info_channel_size -1, "%s:%d", ip, port);
    return 0;
}

int add_channel(Channel channels[], int* count, const char* ip, const char* name, int port) {
    if ((*count) >= max_channels) { return ERR_MAX_CHANNELS; }
    snprintf(channels[(*count)].ip, sizeof(channels[(*count)].ip), "%s", ip);
    snprintf(channels[(*count)].name, sizeof(channels[(*count)].name), "%s", name);
    channels[(*count)].port = port;
    (*count) ++;
    return 0;
}

void handle_client(void* client) {
    Client* client_ptr = (Client*)client;
    int nb_bytes = 1;
    size_t msg_size = 0; 
    size_t msg_date_size = 0; 

    if (send_history(client_ptr) < 0) {
        perror("[ERROR] Envoi historique impossible\n");
    }

    while (nb_bytes > 0 && off_server == 0) {
        if (read((*client_ptr).socket, &msg_size, sizeof(size_t)) <= 0) {
            break;
        } 

        msg_date_size = msg_size + date_size;
        
        char* msg = (char*)malloc(sizeof(char) * msg_size);
        char* msg_with_date = (char*)malloc(sizeof(char) * msg_date_size);
        if (msg == NULL || msg_with_date == NULL) {
            perror("[ERROR] Allocation message client impossible\n");
            break;
        }

        nb_bytes = read((*client_ptr).socket, msg, sizeof(char) * msg_size);
        msg[nb_bytes] = '\0';

        add_date_msg(msg_with_date, msg, msg_date_size);
        if (save_msg((*client_ptr).channel_name, msg_with_date) < 0) {
            perror("[ERROR] Sauvegarde message impossible\n");
        }

        broadcast_msg(msg_with_date);
        free(msg);
        free(msg_with_date);
    }

    if (remove_client(&clients, client_ptr) < 0) {
        perror("[ERROR] Réduction stockage clients impossible\n");
    }

    close((*client_ptr).socket);
    free(client_ptr);

    printf("[INFO] Client déconnecté\n");
    pthread_exit(NULL);
}

void start_channel(const char* name, const char* ip, int port) {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    clients = (int*)malloc(clients_min_size * sizeof(int));
    if (clients == NULL) {
        perror("[ERROR] Allocation stockage clients impossible\n");
        exit(ERR_CLIENTS_STORAGE);
    }

    server_socket = socket(sock_domain, sock_type, 0);
    if (server_socket < 0) {
        perror("[ERROR] Création socket channel impossible\n");
        exit(ERR_CREATE_CHANNEL_SOCKET);
    }

    server_addr.sin_family = sock_domain;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[ERROR] Liaison socket channel impossible\n");
        close(server_socket);
        exit(ERR_BIND_CHANNEL_SOCKET);
    }

    if (listen(server_socket, max_channels) < 0) {
        perror("[ERROR] Ecoute channel impossible\n");
        close(server_socket);
        exit(ERR_LISTEN_CHANNEL_SOCKET);
    }

    printf("[INFO] %s channel disponible à %s:%d\n", name, ip, port);

    while(off_server == 0) {
        int* client_socket = (int*)malloc(sizeof(int));
        if (client_socket == NULL) {
            perror("[ERROR] Allocation socket client sur channel impossible\n");
            exit(EXIT_FAILURE);
        }

        (*client_socket) = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if ((*client_socket) < 0 && off_server == 0) {
            perror("[ERROR] Connexion client sur le channel impossible\n");
            continue; 
        }

        pthread_mutex_lock(&clients_mutex);
        printf("[INFO] Nouveau client connecté sur %s\n", name);
        
        if (clients_count < clients_curr_size) {
            Client* new_client = (Client*)malloc(sizeof(Client));
            (*new_client).socket = (*client_socket);
            (*new_client).channel_name = name;

            if (add_client(&clients, (*client_socket)) < 0) {
                write((*client_socket), exit_msg, strlen(exit_msg) + 1);
                close((*client_socket));
                free(new_client);
                perror("[ERROR] Ajout client impossible\n");
            } else {
                pthread_t client_thread; 
                pthread_create(&client_thread, NULL, (void*)handle_client, (void*)new_client);
                pthread_detach(client_thread);
            }
        } else {
            write((*client_socket), exit_msg, strlen(exit_msg) + 1);
            printf("[ERROR] Taille stockage clients insuffisante\n");
            close((*client_socket));
        }

        pthread_mutex_unlock(&clients_mutex);
        free(client_socket);
    }

    free(clients);
}

void send_acquit(int *socket) {
    if (write((*socket), acquit_msg, strlen(acquit_msg) + 1) < 0) {
        perror("[ERROR] Envoi acquittement impossible\n");
        //close((*socket));
    } else { printf("[INFO] Envoi acquittement\n"); }
}

void wait_acquit(int* socket) {
    char acquit[4];
    while (strcmp(acquit, "ACK") != 0 && off_server == 0) {
        if (read((*socket), acquit, 4) < 0) {
            close((*socket));
            perror("[ERROR] Lecture acquittement impossible\n");
            exit(ERR_READ_ACK);
        }
    }
    strncpy(acquit, "", sizeof(acquit));
}







