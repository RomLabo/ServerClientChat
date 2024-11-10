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
#include <gtk/gtk.h>

/*
    Constantes préprocesseur
*/

// Paramètres socket
#define SOCK_DOMAIN AF_INET
#define SOCK_TYPE SOCK_STREAM
#define SOCK_PROTOCOL 0

#define PORT 8080
#define BUFFER_SIZE 512

/*
    Variables
*/

char buffer[BUFFER_SIZE];
int socket_client;
GtkBuilder *builder;
GObject *window;
GObject *button;
GtkWidget *entry;

/* 
    Déclaration des fonctions
*/

static void quit_app(GtkWidget *widget, gpointer data);
static void send_msg(GtkWidget *widget, gpointer data);

/*
    Main
*/

int main(int argc, char *argv[]) {
    GError *error = NULL;

    /* Vérification des arguments */
    if (argc != 3) { 
        printf("Veuillez renseigner ip et port \n");
        return EXIT_FAILURE;
    }

    /* Création socket client */
    if ((socket_client = socket(SOCK_DOMAIN, SOCK_TYPE, SOCK_PROTOCOL)) == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /* Configuration socket serveur */
    struct sockaddr_in socket_server;
    socket_server.sin_addr.s_addr = inet_addr(argv[1]);
    socket_server.sin_family = SOCK_DOMAIN;
    socket_server.sin_port = htons(atoi(argv[2]));

    /* Création de la connection */
    if (connect(socket_client, (struct sockaddr*)&socket_server,sizeof(socket_server)) == -1) {
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

    entry = GTK_WIDGET(gtk_builder_get_object(builder, "searchentry"));

    button = gtk_builder_get_object (builder, "send");
    g_signal_connect (button, "clicked", G_CALLBACK (send_msg), entry);

    button = gtk_builder_get_object (builder, "quit");
    g_signal_connect (button, "clicked", G_CALLBACK (quit_app), NULL);

    /* Démarrage de la boucle principale Gtk */
    gtk_main ();

    /*while (1) {
        printf("Saisir un message ('exit' pour quitter) : ");
        fgets(buffer, BUFFER_SIZE, stdin);

        buffer[strcspn(buffer, "\n")] = '\0';

        if (send(socket_client, buffer, strlen(buffer), 0) < 0) {
            perror("Erreur envoi message\n");
            break;
        }

        if (strcmp(buffer, "exit") == 0) {
            printf("Déconnexion du serveur\n");
            break;
        }
    }*/

    /* Fermeture de la connexion */
    close(socket_client);
    return 0;
}

/* 
    Définition des fonctions
*/

static void quit_app(GtkWidget *widget, gpointer data) {
    strncpy(buffer, "exit", BUFFER_SIZE -1);
    buffer[BUFFER_SIZE -1] = '\0';
    if (send(socket_client, buffer, strlen(buffer), 0) < 0) {
        perror("Erreur envoi message\n");
    }
    close(socket_client);
    gtk_main_quit();
}

static void send_msg(GtkWidget *widget, gpointer data) {
    GtkWidget *entry = GTK_WIDGET(data);
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    
    if (send(socket_client, text, strlen(text), 0) < 0) {
        perror("Envoi message impossible");
    }

    gtk_entry_set_text(GTK_ENTRY(entry), "");
}