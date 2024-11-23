/*
0000000001 Author RomLabo 111111111
1000111000 client.c 111111111111111
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
#define SOCK_DOMAIN AF_INET
#define SOCK_TYPE SOCK_STREAM
#define SOCK_PROTOCOL 0

#define PORT 8080
#define ADDR_INET "127.0.0.1"
#define BUFFER_SIZE 264

/*
    Variables
*/

char buffer[BUFFER_SIZE];
int socket_client;
struct sockaddr_in addr_server;
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

static void quit_app(GtkWidget *widget, gpointer data);
void handle_signal(int sig);
static void send_msg(GtkWidget *widget, gpointer data);
void receive_msg(void);
gboolean update_text(gpointer data);
void setup_addr(int argc, char *argv[]);

/*
    Main
*/

int main(int argc, char *argv[]) {
    pthread_attr_t attr;
    GError *error = NULL;
    struct sigaction action;
    off_app = 0;

    /* Vérification des arguments */
    if (argc > 3) {
        printf("Trop d arguments renseignés \n");
        return EXIT_FAILURE;
    }

    /* Configuration de l'adresse du serveur */
    setup_addr(argc, argv);

    /* Gestion des signaux */
    action.sa_handler = &handle_signal;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("Gestion signaux impossible\n");
    }

    /* Création socket client */
    if ((socket_client = socket(SOCK_DOMAIN, SOCK_TYPE, SOCK_PROTOCOL)) == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /* Création de la connection */
    if (connect(socket_client, (struct sockaddr*)&addr_server,sizeof(addr_server)) == -1) {
        perror("connect");
        return EXIT_FAILURE;
    }

    /* Attente de l'acquittement */
    while (strcmp(buffer, "ACK") != 0) {
        int nb_char = recv(socket_client, buffer, BUFFER_SIZE -1, 0);
        if (nb_char <= 0) {
            perror("Lecture acquittement impossible");
            break;
        }
        buffer[nb_char] = '\0';
    }

    gtk_init (&argc, &argv);

    /* Récupération des elements builder.ui */
    builder = gtk_builder_new ();
    if (gtk_builder_add_from_file (builder, "interface.ui", &error) == 0) {
        g_printerr ("Error loading file: %s\n", error->message);
        g_clear_error (&error);
        return 1;
    }

    /* Création des différents signaux */
    window = gtk_builder_get_object (builder, "window");
    g_signal_connect (window, "destroy", G_CALLBACK (quit_app), NULL);

    text_view = GTK_WIDGET(gtk_builder_get_object(builder, "text_view"));
    gtk_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    
    entry = GTK_WIDGET(gtk_builder_get_object(builder, "searchentry"));

    button = gtk_builder_get_object (builder, "send");
    g_signal_connect (button, "clicked", G_CALLBACK (send_msg), entry);

    button = gtk_builder_get_object (builder, "quit");
    g_signal_connect (button, "clicked", G_CALLBACK (quit_app), NULL);

    /* Création du thread pour la réception des messages*/
    pthread_attr_init(&attr);
    if (pthread_create(&recv_thread, NULL, (void *)receive_msg, NULL) != 0) {
        perror("Erreur lors de la création du thread");
        return EXIT_FAILURE;
    }

    /* Démarrage de la boucle principale Gtk */
    gtk_main ();

    /* Fermeture de la connexion */
    close(socket_client);
    return 0;
}

/* 
    Définition des fonctions
*/

static void quit_app(GtkWidget *widget, gpointer data) {
    off_app = 1;
    char exit_msg[4] = "EXT";
    
    if (send(socket_client, exit_msg, 4, 0) < 0) {
        perror("Envoi message impossible");
    }
    shutdown(socket_client, 0);
    close(socket_client);
    pthread_join(recv_thread, NULL);
    gtk_main_quit();
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

static void send_msg(GtkWidget *widget, gpointer data) {
    GtkWidget *entry = GTK_WIDGET(data);
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (send(socket_client, text, strlen(text), 0) < 0) {
        perror("Envoi message impossible");
    }
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

void receive_msg(void) {
    char buffer_msg[BUFFER_SIZE];
    while (off_app == 0) {
        int nb_char = recv(socket_client, buffer_msg, BUFFER_SIZE - 1, 0);
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

void setup_addr(int argc, char *argv[]) {
    addr_server.sin_family = SOCK_DOMAIN;
    if (argc == 3) {
        addr_server.sin_addr.s_addr = inet_addr(argv[1]);
        addr_server.sin_port = htons(atoi(argv[2]));
    } else if (argc == 2) {
        addr_server.sin_addr.s_addr = inet_addr(argv[1]);
        addr_server.sin_port = htons(PORT);
    } else if (argc == 1) {
        addr_server.sin_addr.s_addr = inet_addr(ADDR_INET);
        addr_server.sin_port = htons(PORT);
    }
}