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
- L'historique n'est pas correctement envoyé ou recèptionné par le client.

## Reste à faire
- Actuellement, lorsque l'application serveur est arrêtée via un CTRL + C, 
  la gestion de la terminaison des threads n'est pas correctement prise en charge, 
  ce qui entraîne des fuites de mémoire. 
  
  J'ai commencé à implémenter un mécanisme permettant d'attendre la fin 
  de chaque thread lorsque le processus enfant (channel) reçoit le signal SIGTERM. 
  Cependant, il reste à stocker les identifiants des threads (pthread_t) dans un tableau.

  Pour le moment, je crée les threads en mode détaché. À l'avenir, 
  il sera nécessaire de gérer dynamiquement ce tableau des threads, 
  comme je le fais déjà pour les clients. 
  Actuellement, la gestion des clients est dynamique : j'ajuste la taille du stockage 
  en fonction du nombre de clients encore connectés. 
  Je devrai appliquer la même logique au tableau des threads, afin d'éviter 
  que le processus enfant (channel) attende la terminaison d'un thread 
  déjà terminé après la déconnexion d'un client.