/*
0000000001 Author RomLabo 111111111
1000111000 client_gtk.c 11111111111
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
#include <arpa/inet.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <pthread.h>

/*
    Constantes préprocesseur
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
int off_app;
GtkBuilder *builder;
GObject *window;
GObject *button;
GtkWidget *entry;
GtkWidget *text_view;
GtkTextBuffer *gtk_buffer;
pthread_t recv_thread;

/* 
    Déclaration des fonctions
*/

void setup_addr(int argc, char *argv[], struct sockaddr_in *addr);
void handle_signal(int sig);
void wait_acquit(void);
void quit_app(GtkWidget *widget, gpointer data);
void send_msg(GtkWidget *widget, gpointer data);
void receive_msg(void* socket_cl);
gboolean update_text(gpointer data);

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
    pthread_attr_t attr;
    GError *error = NULL;
    struct sigaction action;
    struct sockaddr_in addr_server;
    off_app = 0;

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

    gtk_init(&argc, &argv);

    /* Récupération des elements builder.ui */
    builder = gtk_builder_new();
    if (gtk_builder_add_from_file(builder, "interface.ui", &error) == 0) {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    /* Création interface */
    window = gtk_builder_get_object(builder, "window");
    g_signal_connect(window, "destroy", G_CALLBACK(quit_app), NULL);

    text_view = GTK_WIDGET(gtk_builder_get_object(builder, "text_view"));
    gtk_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    
    entry = GTK_WIDGET(gtk_builder_get_object(builder, "searchentry"));

    button = gtk_builder_get_object(builder, "send");
    g_signal_connect (button, "clicked", G_CALLBACK (send_msg), entry);

    button = gtk_builder_get_object (builder, "quit");
    g_signal_connect (button, "clicked", G_CALLBACK (quit_app), NULL);
    gtk_widget_grab_focus(entry);

    /* Création du thread pour la réception des messages*/
    pthread_attr_init(&attr);
    if (pthread_create(&recv_thread, NULL, (void *)receive_msg, &socket_client) != 0) {
        perror("Erreur lors de la création du thread");
        return EXIT_FAILURE;
    }

    /* Démarrage de la boucle principale Gtk */
    gtk_main ();

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

void handle_signal(int signal) {
    if (signal == SIGINT) {
        off_app = 1;
        shutdown(socket_client, 0);
        close(socket_client);
        pthread_join(recv_thread, NULL);
        gtk_main_quit();
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
    strcpy(acquit_msg, "");
}

void quit_app(GtkWidget *widget, gpointer data) {
    off_app = 1;
    // char exit_msg[4] = "EXT";
    
    // if (send(socket_client, exit_msg, 4, 0) < 0) {
    //     perror("Envoi message impossible");
    // }
    shutdown(socket_client, 0);
    close(socket_client);
    pthread_join(recv_thread, NULL);
    gtk_main_quit();
}

void send_msg(GtkWidget *widget, gpointer data) {
    GtkWidget *entry = GTK_WIDGET(data);
    char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    strcat(text, " \n");
    if (send(socket_client, text, strlen(text), 0) < 0) {
        perror("Envoi message impossible");
    }
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

void receive_msg(void* socket_cl) {
    int socket = *(int*)socket_cl;
    char buffer_msg[buffer_size];
    while (off_app == 0) {
        int nb_char = recv(socket, buffer_msg, sizeof(buffer_msg) - 1, 0);
        if (nb_char <= 0) { break; }
        buffer_msg[nb_char] = '\0';
        char *message = g_strdup(buffer_msg);
        g_idle_add(update_text, message);
    }
    pthread_exit(NULL);
}

gboolean update_text(gpointer data) {
    char *message = (char *)data;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(gtk_buffer, &end);
    gtk_text_buffer_insert(gtk_buffer, &end, message, -1);
    g_free(message); 
    return FALSE; 
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