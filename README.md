# WeeChat 

WeeChat est une application de chat client/serveur en C permettant
d'échanger des messages entre plusieurs utilisateurs ou à destination
des utilisateurs connectés au serveur. 

C'est une application graphique utilisant la librairie GTK 3.0

pour compiler l'application client utilisant la librairie GTK,
utiliser la commande suivante : 
        gcc `pkg-config --cflags gtk+-3.0` -o client client.c `pkg-config --libs gtk+-3.0`

pour compiler l'application server
utiliser la commande suivante :
        gcc server.c -o server -Wall

Par défaut le serveur écoute sur l'adresse 127.0.0.1 
et sur le port 8080. 

En cours de développement...
