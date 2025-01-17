# ServerClientChat 

ServerClientChat est une application de chat client/serveur en C permettant
d'échanger des messages entre plusieurs utilisateurs connectés sur un
channel. De plus, l'utilisateur a la possibilité de créer un nouveau channel. 

/!\ L'application client gtk n'est pas à jour pour l'instant,
    elle ne fonctionnera pas avec la version actuelle de 
    l'application server. 
pour compiler l'application client utilisant la librairie GTK 
##
        gcc client_gtk.c -o client $(pkg-config --cflags --libs gtk+-3.0)

pour compiler l'application client console 
##
        gcc client_console.c -o client -Wall

pour compiler l'application server
##
        gcc server.c -o server -Wall

Par défaut le serveur écoute sur l'adresse 127.0.0.1 
et sur le port 8080. 
Au lancement du programme server on peut spécifier une 
adresse et port différent. Il faudra alors également spécifier
l'adresse et le port du serveur lors du démarrage de l'application client.

Pour arrêter l'application serveur : CTRL + C 
Pour arrêter l'application client : /quitter 

## Problèmes 
- L'historique n'est pas correctement envoyé.

