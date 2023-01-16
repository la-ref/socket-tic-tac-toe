#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <unistd.h> /* pour read, write, close, sleep */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h> /* pour htons et inet_aton */
#include <time.h> /* pour reset le temps pour générer un nb random*/

#define PORT IPPORT_USERRESERVED // = 5000 (ports >= 5000 réservés pour usage explicite)

#define LG_MESSAGE 256 // taile du message
#define TAILLE_GRILLE 3 // taille de la grille en 1 ligne, pour être n*n par la suite donc ici 3*3
#define NBPLAYER 2

// Structure représentant une grille du morpion avec sa taille et son tableau
struct grilleMorpion {
	int taille;
	char tableau[TAILLE_GRILLE*TAILLE_GRILLE];
};

// Fonction qui permet d'initialiser à vide une grille de morpion
void initializeGrille(struct grilleMorpion *grille)
{
	// pour tout les élèments ont les mets à un espace
	for (int i = 0;i<(grille->taille*grille->taille); i++)
	{
		grille->tableau[i] = ' ';
	}
};

/*
	Fonction qui vérifie si les coordonnées en x,y ne sont correct
	retourne 1 si les coordonnées sont correctes sinon 0
*/
int verifCoords(int x, int y,struct grilleMorpion *grille){
	if (x >= 0 && x < grille->taille && y >= 0 && x < grille->taille && (grille->tableau[y*grille->taille+x]==' ')){ // verif si les coordonnées ne sont pas hors du tableau
		return 1;	
	}
	return 0;
}


/*Fonction permettant de jouer à une coordonnée sélectionné sur la grille en fonction du type de joueur
	retourne 1 si le placement est possible sinon 0*/
int jouerMorpion(int x, int y, struct grilleMorpion *grille,int local){
	x=x-1;
	y=y-1;
	if (verifCoords(x,y,grille)){ // verif si les coordonnées ne sont pas hors du tableau
		if (grille->tableau[y*grille->taille+x]==' '){
			if (local) // si le serveur joue
			{
				grille->tableau[y*grille->taille+x] = 'O';
			}
			else // si le client joue
			{
				grille->tableau[y*grille->taille+x] = 'X';
			}
			return 1;
		}
		
	}
	return 0;
};




/* Fonction permettant d'envoyer un message sur un socket 
	retourne 1 si le message est envoyé sinon 0
*/
int sendMessage(int descripteurSock,char buff[])
{
	int nb; // résultat du write
	// Envoi du message
	switch(nb = write(descripteurSock, buff, strlen(buff))){ // vérif de récupéraiton du message
		case -1 : /* une erreur ! */
     			perror("Erreur en écriture...\n");
		     	close(descripteurSock);
		     	exit(-3);
		case 0 : /* la socket est fermée */
			fprintf(stderr, "La socket a été fermée par le serveur !\n\n");
			return 0;
		default: /* envoi de n octets */
			printf("j'envoie : %s\n",buff);
			return 1;
	};
};

/* Fonction permettant de lire un message sur un socket 
	retourne 1 si le message est envoyé sinon 0
*/
int readMessage(int descripteurSock,char buff[])
{
	int nb;// résultat du write
	// Réception du message
	switch(nb = read(descripteurSock, buff, 256*sizeof(char))){
		case -1 : /* une erreur ! */
     			perror("Erreur en lecture...\n");
		     	close(descripteurSock);
				exit(-3);
				return 0;
		case 0 : /* la socket est fermée */
			fprintf(stderr, "La socket a été fermée par le serveur !\n\n");
			return 0;
		default: 
			buff[nb] = '\0'; // clear le buffer,évite les caractères invisibles
			return 1;
	}
};

/*Fonction qui permet de vérifier la victoire sur une grille morpion en retournant : 
     - 0 si partie non finie
     - 1 si X gagne
     - 2 si O gagne
     - 3 si égalité
*/
int estFini(struct grilleMorpion *grille)
{
	int totalCount = grille->taille*grille->taille;
    int colCount = 0;
    int rowCount = 0;
    int diagDescCount = 0;
    int diagAscCount = 0;

    for (int i = 0; i<grille->taille; i++){
		// cas des lignes et colonnes
        for (int j = 0; j<grille->taille; j++){
            if (grille->tableau[i*grille->taille+j]=='X'){
                rowCount++;
            }else if (grille->tableau[i*grille->taille+j]=='O'){
                rowCount--;
            }else{
                totalCount--;
            }
            if (grille->tableau[j*grille->taille+i]=='X'){
                colCount++;
            }else if (grille->tableau[j*grille->taille+i]=='O'){
                colCount--;
            }
        }
		// vérif ligne et colonnes
        if ((rowCount == 3) || (colCount == 3)){
            printf("X gagne\n");
            return 1;
        }else if ((rowCount == -3) || (colCount == -3)){
            printf("0 gagne\n");
            return 2;
        }

        colCount = 0; 
        rowCount = 0;

        // cas des diagonales
        if (grille->tableau[i*(grille->taille)+i]=='X'){
            diagDescCount++;
        }else if (grille->tableau[i*(grille->taille)+i]=='O'){
            diagDescCount--;
        }
		if (grille->tableau[i*grille->taille+((grille->taille-1)-i)]=='X'){
            diagAscCount++;
        }else if (grille->tableau[i*grille->taille+((grille->taille-1)-i)]=='O'){
            diagAscCount--;
        }
    }
	// vérif diagonales
    if ((diagDescCount == 3) || (diagAscCount == 3)){
        printf("X gagne\n");
        return 1;
    }else if ((diagDescCount == -3) || (diagAscCount == -3)){
        printf("0 gagne\n");
        return 2;
    }
	// vérif grille pleine
    if(totalCount==9){
        printf("égalité\n");
        return 3;
    }
    return 0;
}

int main(int argc, char *argv[]){
	
	int socketEcoute;

	struct sockaddr_in pointDeRencontreLocal;
	socklen_t longueurAdresse;

	struct sockaddr_in pointDeRencontreDistant;
	char messageRecu[LG_MESSAGE]; /* le message de la couche Application ! */
	int ecrits, lus; /* nb d’octets ecrits et lus */
	int retour;
	char result[LG_MESSAGE];

	struct grilleMorpion grille;// création de la grille de morpion
	grille.taille = TAILLE_GRILLE;  // on set la taille de la grille
	int finished = 0; // boucle de jeu
	int resultRead = 0; // retour read
	char coordo[256]; // coordonnées que le joueur courant joue
	// Crée un socket de communication
	socketEcoute = socket(PF_INET, SOCK_STREAM, 0); 
	// Teste la valeur renvoyée par l’appel système socket() 
	if(socketEcoute < 0){
		perror("socket"); // Affiche le message d’erreur 
	exit(-1); // On sort en indiquant un code erreur
	}
	printf("Socket créée avec succès ! (%d)\n", socketEcoute); // On prépare l’adresse d’attachement locale

	// Remplissage de sockaddrDistant (structure sockaddr_in identifiant le point d'écoute local)
	longueurAdresse = sizeof(pointDeRencontreLocal);
	// memset sert à faire une copie d'un octet n fois à partir d'une adresse mémoire donnée
	// ici l'octet 0 est recopié longueurAdresse fois à partir de l'adresse &pointDeRencontreLocal
	memset(&pointDeRencontreLocal, 0x00, longueurAdresse); pointDeRencontreLocal.sin_family = PF_INET;
	pointDeRencontreLocal.sin_addr.s_addr = htonl(INADDR_ANY); // attaché à toutes les interfaces locales disponibles
	pointDeRencontreLocal.sin_port = htons(PORT); // = 5000 ou plus
	
	// On demande l’attachement local de la socket
	if((bind(socketEcoute, (struct sockaddr *)&pointDeRencontreLocal, longueurAdresse)) < 0) {
		perror("bind");
		exit(-2); 
	}
	printf("Socket attachée avec succès !\n");

	// On fixe la taille de la file d’attente à 5 (pour les demandes de connexion non encore traitées)
	if(listen(socketEcoute, 5) < 0){
   		perror("listen");
   		exit(-3);
	}
	printf("Socket placée en écoute passive ...\n");
	
	// boucle d’attente de connexion : en théorie, un serveur attend indéfiniment ! 
	int arreteServeur = 0;
	int tabSocket[2]; // tableau pour récupérer les deux sockets clients
	int nbSocket = 0; // valeur pour incrementer pour récuper le nb de client à mettre dans le tableau
	while(arreteServeur==0){

		// attend la connexion de deux joueurs
		while (nbSocket<2){
			memset(messageRecu, 0x00, LG_MESSAGE*sizeof(char)); // clear du message
			printf("Attente d’une demande de connexion (quitter avec Ctrl-C)\n\n");

			// c’est un appel bloquant
			tabSocket[nbSocket] = accept(socketEcoute, (struct sockaddr *)&pointDeRencontreDistant, &longueurAdresse);
		
		
			if (tabSocket[nbSocket]  < 0) {
				perror("erreur socket dialogue\n");
				close(tabSocket[nbSocket]);
				close(socketEcoute);
				exit(-4);
			}else{
				nbSocket++;
			}
			printf("socket : %d",nbSocket);
		}

		int fils = fork(); // on fork et on laisse au fils la gestion du jeu avec les deux clients
		if (fils){
			// le père reste à l'écoute constante et transmet les sockets aux fils
			printf("fils créé");
			//close(socketEcoute);
			for (int i = 0; i<nbSocket; i++){
				close(tabSocket[i]);
			}
			nbSocket=0;
			memset(tabSocket, 0x00, 2*sizeof(char));
		}
		else{
			// empêche le serveur fils de réécouter (meurt après la partie)
			arreteServeur=1;
			finished = 0;

			initializeGrille(&grille); // initialise la grille
			// quand un dialogue est accepté
			sendMessage(tabSocket[0] , "tu es X\n"); // envoie start au client1
			sendMessage(tabSocket[1] , "tu es O\n"); // envoie start au client2
			int currentPlayer = 0;
			// 0 = X CLIENT1
			// 1 = O CLIENT2

			while (finished == 0){
				memset(messageRecu, 0x00, LG_MESSAGE*sizeof(char)); // clear du message
				memset(coordo, 0x00, LG_MESSAGE*sizeof(char)); // clear du message

				// Réception du message
				resultRead = readMessage(tabSocket[currentPlayer],messageRecu);
				int joue; // verif les conditions des victoires
				if (resultRead == 1) // vérif si on à reçu correctement le message
				{
					// verif si les coords reçus sont valide 
					if (strlen(messageRecu) == 2 && verifCoords((int)(messageRecu[0]- '0')-1,(int)(messageRecu[1]- '0')-1,&grille)) // verif si le message reçu et de taille deux pour recevoir les coordonnées en xy sous forme "xy"
					{
						jouerMorpion((int)(messageRecu[0]- '0'),(int)(messageRecu[1]- '0'),&grille,currentPlayer); // place l'icone du joueur courant sur la grille de morpion via les coordonnées reçu 
						joue = estFini(&grille);

						// prépare le message en fonction du joueur courant en indiquant son icone pour leur envoyer et mettre à jour leur grille
						if (currentPlayer==0){
							strcat(messageRecu,"X");
						}else{
							strcat(messageRecu,"O");
						}
						
						if (joue == 0) 
						{
							strcpy(coordo,messageRecu);
							strcat(messageRecu,"-joue");
							strcat(coordo,"-continue");

							sendMessage(tabSocket[currentPlayer], messageRecu); // envoie la confirmation au joueur courant pour dire que son placement est ok
							currentPlayer=(currentPlayer+1)%2; // change de joueur
							sendMessage(tabSocket[currentPlayer], coordo); // envoie les coordonnées jouées à l'autre joueur qu'il doit placer
							
						}
						else if (joue == 1) 
						{
							finished = 1;
							strcat(messageRecu,"-Xwins");
							sendMessage(tabSocket[currentPlayer], messageRecu);         // envoie la confirmation au joueur courant et le résultat sois x win
							sendMessage(tabSocket[(currentPlayer+1)%2], messageRecu);	// envoie les coordonnées jouées à l'autre joueur et le résultat sois x win
							printf("partie fini! X gagne !\n");
						}
						else if (joue == 2) 
						{
							finished = 1;
							strcat(messageRecu,"-Owins");
							sendMessage(tabSocket[currentPlayer], messageRecu); // envoie la confirmation au joueur courant et le résultat sois o win
							sendMessage(tabSocket[(currentPlayer+1)%2], messageRecu); // envoie les coordonnées jouées à l'autre joueur et le résultat sois x win
							printf("partie fini! O gagne !\n");
						}
						else 
						{
							finished = 1;
							if (currentPlayer==0){// si fini et le joueur courant est 0 alors c'est X qui a placer le dernier sinon 0
								strcat(messageRecu,"-Xend");
								sendMessage(tabSocket[currentPlayer], messageRecu); // envoie les coordonnées jouées à l'autre joueur et le résultat sois x end
								sendMessage(tabSocket[(currentPlayer+1)%2], messageRecu); // envoie les coordonnées jouées à l'autre joueur et le résultat sois x end
								printf("partie fini! égalité !\n");	
							}else{
								strcat(messageRecu,"-Oend");
								sendMessage(tabSocket[currentPlayer], messageRecu); // envoie les coordonnées jouées à l'autre joueur et le résultat sois o end
								sendMessage(tabSocket[(currentPlayer+1)%2], messageRecu); // envoie les coordonnées jouées à l'autre joueur et le résultat sois x end
								printf("partie fini! égalité !\n");	
							}
							
						}


						memset(messageRecu, 0x00, LG_MESSAGE*sizeof(char)); // clear du message
						memset(coordo, 0x00, LG_MESSAGE*sizeof(char)); // clear du message
					}
					// si coords reçu invalide
					else{
						
						sendMessage(tabSocket[currentPlayer], "again"); // envoie au client qu'il y a une erreur et il veut à nouveau des coordonnées
						printf("erreur reception coordonnées !: %s\n",messageRecu);
						memset(messageRecu, 0x00, LG_MESSAGE*sizeof(char)); // clear du message
						memset(coordo, 0x00, LG_MESSAGE*sizeof(char)); // clear du message
					}
				}
				else if (resultRead == 0)
				{
					finished = 1;
					arreteServeur = 1;
					sendMessage(tabSocket[(currentPlayer+1)%2], "close"); // envoie au client qu'il y a une erreur et il veut à nouveau des coordonnées
				}
				else
				{
					printf("erreur lecture message\n");
				};
			}
			// On ferme les ressource avant de quitter du fork
			for (int i = 0; i<NBPLAYER; i++){
				close(tabSocket[i]);
			}
		}
	};

	// On ferme les ressource avant de quitter
	for (int i = 0; i<NBPLAYER; i++){
		close(tabSocket[i]);
	}
	

   	close(socketEcoute);
	return 0; 
}


