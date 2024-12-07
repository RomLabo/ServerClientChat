# WeeChat 

WeeChat est une application de chat client/serveur en C permettant
d'échanger des messages entre plusieurs utilisateurs ou à destination
des utilisateurs connectés au serveur. 

C'est une application graphique utilisant la librairie GTK 3.0

pour compiler l'application client utilisant la librairie GTK 
##
        gcc -o hello_world $(pkg-config --cflags --libs gtk+-3.0) hello_world.c

pour compiler l'application server
##
        gcc server.c -o server -Wall

Par défaut le serveur écoute sur l'adresse 127.0.0.1 
et sur le port 8080. 

En cours de développement...
