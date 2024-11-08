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

int enregistrer_cle(int fd);
char enregistrer_mode(int fd);
void encrypter_chaine(int fd, int cle);
void decrypter_chaine(void);

int main(int argc, char *argv[]) {
    pid_t id, idFils;
    int etat, fd, li, fa, size;
    int cpt = 0;
    int co = 0;

    char mode = ' ';
    int cle_cryptage = 0;
    printf("Serveur TCP \n");

    if (argc == 3) {
        /* Création de la socket */
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            printf("Impossible de créer la socket\n");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in sin;
        sin.sin_addr.s_addr = inet_addr(argv[1]);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(atoi(argv[2]));

        /* Associer une socket à un point de terminaison local */
        if ((bind(fd, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
            printf("Association de la socket impossible\n");
            exit(EXIT_FAILURE);
        }

        /* Mettre la socket en état d'écoute */
        li = listen(fd, 3);
        if (li < 0) {
            printf("Ecoute impossible\n");
            exit(EXIT_FAILURE);
        }

        /* Accepter un appel de connexion avec le client */
        struct sockaddr_in client;
        size = sizeof(struct sockaddr_in);

        while(1) {
            if ((fa = accept(fd, (struct sockaddr *)&client, &size)) < 0) {
                fprintf(stderr, "Impossible d'accepter la socket distante\n");
                exit(EXIT_FAILURE);
            }

            id = fork();
            switch(id) {
                case -1: exit(EXIT_FAILURE); break;
                case 0:
                    /* Processus fils */

                    /* Enregistrement de la clé de cryptage */
                    cle_cryptage = enregistrer_cle(fa);
                    while(fa >= 0) {
                        /* Enregistrement du mode ( encryptage: + ou décryptage: - )*/
                        mode = enregistrer_mode(fa);
                        /* Transformation de la chaine */
                        encrypter_chaine(fa, cle_cryptage);
                        /*if (mode == '+') { encrypter_chaine(fa, cle_cryptage); }
                        else { }*/
                    }

                    /* Fermer la connexion socket et libérer toutes les ressources associées */
                    shutdown(fa,2);
                    close(fa);
                    exit(0);
                default:
                    idFils = wait(&etat);
                    close(fd);
                    if (idFils == -1) { exit(EXIT_FAILURE); }
                    else { exit(EXIT_SUCCESS); }
                    break;
            }
        }
    } else {
        printf("Veuillez renseigner ip et port \n");
    }
    return 0;
}


int enregistrer_cle(int fd) {
    int cle = 0;
    char acquit[3] = "OK";
    if (read(fd,&cle, sizeof(cle)) < 0) {
        perror("Impossible de lire la clé");
        exit(4);
    }

    printf("Clé reçue : %d\n", cle);

    if (write(fd, &acquit, sizeof(acquit)) == -1) {
        perror("Acquittement impossible");
        exit(2);
    }

    return cle;
}

char enregistrer_mode(int fd) {
    char mode = '+';
    if (read(fd,&mode, sizeof(mode)) < 0) {
        perror("Impossible de lire la clé");
        exit(4);
    }

    printf("Mode reçu : %c\n", mode);
    return mode;
}

void encrypter_chaine(int fd, int cle) {
    char chaine[4];
    size_t i = 0;
    int key = 0;
    if (read(fd,&chaine, 4) < 0) {
        perror("Impossible de lire la chaine");
        exit(4);
    }

    printf("Chaine reçue : %s\n", chaine);

    while (chaine[i] != '\0') {
      key = (chaine[i] + cle) % 26;
      chaine[i] = chaine[i] + cle;
      printf("%c", key);
      i++;
    }


    if (write(fd, &chaine, i +1 ) == -1) {
        perror("Envoie chaine impossible");
        exit(2);
    }

}
