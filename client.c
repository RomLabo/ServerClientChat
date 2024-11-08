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

/* 
    Déclaration des fonctions
*/

void create_socket(int fd);
struct sockaddr_in config_socket(char *addr, char *port);
void connect_to_server(int fd, struct sockaddr *ptr_addr, int lg_addr);

/*
    Main
*/

int main(int argc, char *argv[]) {
    int fd;
    char msg;
    char acquit[3];
    char chaine[4];
    char acquitMsg[3] = "OK";
    char reponse[4];

    printf("Client TCP \n");

    if (argc == 4) {

        /*if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
            perror("socket");
            exit(1);
        }*/

        struct sockaddr_in sock;
        sock = config_socket(argv[1], argv[2]);
        /*sock.sin_addr.s_addr = inet_addr(argv[1]);
        sock.sin_family = AF_INET;
        sock.sin_port = htons(atoi(argv[2]));*/

        /* Etablir la connection */
        connect_to_server(fd, (struct sockaddr*)&sock, sizeof(sock));
        /*if (connect(fd, (struct sockaddr*)&sock,sizeof(sock)) == -1) {
            perror("connect");
            exit(1);
        }*/

        int cle = atoi(argv[3]);

        /* Envoyer la cle (un entier ici )*/
        if (write(fd,&cle,sizeof(cle)) == -1) {
            perror("Envoie de la clé impossible");
            exit(1);
        }


        /* Attente de l'acquittement */
        while (strcmp(acquit,acquitMsg) != 0) {
            if (read(fd,&acquit,3) == -1) {
                perror("Lecture acquittement impossible");
                exit(1);
            }
        }

        printf("Saisir le mode : ");
        scanf("%c", &msg);

        /* Envoie du mode */
        if (write(fd,&msg,sizeof(msg)) == -1) {
            perror("write");
            exit(1);
        }
        printf("Mode envoyé : %c\n", msg);

        printf("Saisir la chaine : ");
        scanf("%3[^\n]", chaine);

        /* Envoie du mode */
        if (write(fd,&chaine,sizeof(chaine)) == -1) {
            perror("write");
            exit(1);
        }
        printf("Chaine envoyé : %s\n", chaine);

        /* Recevoir la reponse */
        if (read(fd,&reponse,sizeof(reponse)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        printf("Reponse : %s\n", reponse);


        /* Fermer la connexion */
        shutdown(fd,2);
        close(fd);
    } else {
        printf("Veuillez renseigner ip et port \n");
    }
    return 0;
}

/* 
    Définition des fonctions
*/

void create_socket(int fd) {
    if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("Creation socket impossible");
        exit(1);
    }
}

struct sockaddr_in config_socket(char* addr, char* port) {
    struct sockaddr_in sock;
    sock.sin_addr.s_addr = inet_addr(addr);
    sock.sin_family = AF_INET;
    sock.sin_port = htons(atoi(port));
    return sock;
}

void connect_to_server(int fd, struct sockaddr *ptr_addr, int lg_addr) {
    if (connect(fd, ptr_addr, lg_addr) == -1) {
        perror("Connexion impossible");
        exit(1);
    }
}