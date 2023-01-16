#include <stdio.h>
#include <stdlib.h> /* pour exit */
#include <unistd.h> /* pour read, write, close, sleep */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> /* pour memset */
#include <netinet/in.h> /* pour struct sockaddr_in */
#include <arpa/inet.h> /* pour htons et inet_aton */

#define TAILLE_GRILLE 3 // taille de la grille en 1 ligne, pour être n*n par la suite donc ici 3*3
#define LG_MESSAGE 256 // taile du message

// Structure représentant une grille du morpion avec sa taille et son tableau
struct grilleMorpion 
{
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

/* Fontion qui permet d'afficher graphiquement la grille sous forme ex:
	-------
	|X| | |
	-------
	| |O| |
	-------
	| | | |
	-------
*/
void afficheGrille(struct grilleMorpion *grille)
{
	printf("\n-------\n");  
	for (int i = 0; i<grille->taille; i++)
	{
		
		for (int j = 0; j<grille->taille; j++)
		{
			printf("|%c",grille->tableau[i*grille->taille+j]);// permet en fonction d'un simple tableau de séparer les élèments simulant un tab 2d
		}
		printf("|\n-------\n");
    }
};

/* Fonction qui joue en des coordonnées x,y sur une grille de morpion l'icone reçu
	retourne 1 une fois placé
*/
int jouerMorpion(int x, int y, char icone, struct grilleMorpion *grille){
	// on considère que les coordonnées sont correctes car le serveur les vérifies
	x=x-1;
	y=y-1;
	grille->tableau[y*grille->taille+x] = icone;
	return 1;
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

// Fonction permettant de clear le buffer pour éviter les caractères invisible pendant un scanf
void viderBuffer()
{
    int c = 0;
    while (c != '\n' && c != EOF) 
    {
        c = getchar(); // lis les caractères non voulu
    }
}

/*
	Fonction qui vérifie si les coordonnées en x,y ne sont pas hors du tableau
	retourne 1 si les coordonnées sont correctes sinon 0
*/
int verifCoords(int x, int y,struct grilleMorpion *grille){
	if (x >= 0 && x < grille->taille && y >= 0 && x < grille->taille){ // verif si les coordonnées ne sont pas hors du tableau
		return 1;	
	}
	return 0;
}


/*
	Fonction qui permet d'intérroger le client via des inputs consoles les coordonnées en x et y qu'il veut joué
	et en fonction de cela de les placer sur une grille morpion tout en transmettant les coordonnées jouées
	retourne 1 quand le client à choisi des coordonnées correctes
*/
int joueTour(struct grilleMorpion* grille,char* coord)
{
	int x;
	int y;
	int joue = 0;
	printf("\n----- C'est au tour du client -----\n");
	while (joue != 1) {
		x = 0;
		y = 0;
		printf("Coordonnée en x = ");
		if (scanf("%d",&x) == 1)
		{
			printf("Coordonnée en y = ");
			if (scanf("%d",&y) == 1)
			{
				joue = verifCoords(x-1,y-1,grille); // même si le serveur vérifie, le client vérifie pour éviter les communications inutiles,mais si il triche le serveur vérifiera aussi
			}
			else
			{
				printf("\nerreur coordonnée en y\n");
			}
		}
		else
		{
			printf("\nerreur coordonnée en x\n");
		}
		viderBuffer();
	}
	sprintf(coord, "%d%d", x, y);
	return 1;
}

int main(int argc, char *argv[]){
	int descripteurSocket;
	struct sockaddr_in sockaddrDistant;
	socklen_t longueurAdresse;

	int nb; /* nb d’octets écrits et lus */

	char ip_dest[16];
	char messageRecu[256];
	int  port_dest;

	struct grilleMorpion grille; // création de la grille
	grille.taille = TAILLE_GRILLE; // on set la taille de la grille
	int finished = 0; // boucle de jeu
	int resultRead = 0; // retour read
	char coordo[2]; // coordonnées xy qu'on joue
            


	// Pour pouvoir contacter le serveur, le client doit connaître son adresse IP et le port de comunication
	// Ces 2 informations sont passées sur la ligne de commande
	// Si le serveur et le client tournent sur la même machine alors l'IP locale fonctionne : 127.0.0.1
	// Le port d'écoute du serveur est 5000 dans cet exemple, donc en local utiliser la commande :
	// ./client_base_tcp 127.0.0.1 5000
	if (argc>1) { // si il y a au moins 2 arguments passés en ligne de commande, récupération ip et port
		strncpy(ip_dest,argv[1],16);
		sscanf(argv[2],"%d",&port_dest);
	}else{
		printf("USAGE : %s ip port\n",argv[0]);
		exit(-1);
	}

	// Crée un socket de communication
	descripteurSocket = socket(AF_INET, SOCK_STREAM, 0);
	// Teste la valeur renvoyée par l’appel système socket()
	if(descripteurSocket < 0){
		perror("Erreur en création de la socket...\n"); // Affiche le message d’erreur
		exit(-1); // On sort en indiquant un code erreur
	}
	printf("Socket créée! (%d)\n", descripteurSocket);


	// Remplissage de sockaddrDistant (structure sockaddr_in identifiant la machine distante)
	// Obtient la longueur en octets de la structure sockaddr_in
	longueurAdresse = sizeof(sockaddrDistant);
	// Initialise à 0 la structure sockaddr_in
	// memset sert à faire une copie d'un octet n fois à partir d'une adresse mémoire donnée
	// ici l'octet 0 est recopié longueurAdresse fois à partir de l'adresse &sockaddrDistant
	memset(&sockaddrDistant, 0x00, longueurAdresse);
	// Renseigne la structure sockaddr_in avec les informations du serveur distant
	sockaddrDistant.sin_family = AF_INET;
	// On choisit le numéro de port d’écoute du serveur
	sockaddrDistant.sin_port = htons(port_dest);
	// On choisit l’adresse IPv4 du serveur
	inet_aton(ip_dest, &sockaddrDistant.sin_addr);

	// Débute la connexion vers le processus serveur distant
	if((connect(descripteurSocket, (struct sockaddr *)&sockaddrDistant,longueurAdresse)) == -1){
		perror("Erreur de connection avec le serveur distant...\n");
		close(descripteurSocket);
		exit(-2); // On sort en indiquant un code erreur
	}
	printf("Connexion au serveur %s:%d réussie!\nEn attente d'un adversaire !",ip_dest,port_dest);

	while (finished == 0){
		memset(messageRecu, 0x00, LG_MESSAGE*sizeof(char)); // clear du message
		// Réception du message
		resultRead = readMessage(descripteurSocket,messageRecu); // lis le message reçu par le serveur
		memset(coordo, 0x00, 3*sizeof(char));
		if (resultRead == 1)
		{	
			if (strstr(messageRecu,"tu es X")!=NULL) // verif message start pour initialiser
			{
				initializeGrille(&grille);
				afficheGrille(&grille);
				printf("%s\n",messageRecu);
				joueTour(&grille,coordo);
				sendMessage(descripteurSocket,coordo); // envoie les coordonnées jouées au serveur
			}
			else if (strstr(messageRecu,"tu es O")!=NULL) // verif message start pour initialiser
			{
				initializeGrille(&grille);
				afficheGrille(&grille);
				printf("%s\n",messageRecu);
			}
			else if (strstr(messageRecu,"joue")!=NULL) // verif si le message reçu est joue pour recevoir les coordonnées en xy et Icone sous forme "xyO ou X" et les jouées ce qui valide ses coordonnées
			{
				//printf("\nJE PASSE avec x: %d, y : %d, icone : %c\n",(int)(messageRecu[0]-'0'),(int)(messageRecu[1]- '0'),(messageRecu[2]));
				printf("à l'adversaire de jouer !");
				jouerMorpion((int)(messageRecu[0]- '0'),(int)(messageRecu[1]- '0'),(messageRecu[2]),&grille); // place l'icone du client sur la grille du morpion les coordonnées reçu, donc ici ou il a joué
				afficheGrille(&grille);
			}
			else if (strstr(messageRecu,"continue")!=NULL) // verif si le message est continue pour dire au client de jouer
			{
				printf("à toi de jouer !");
				//printf("\nle serveur à joué avec x: %d, y : %d, icone : %c\n",(int)(messageRecu[0]-'0'),(int)(messageRecu[1]- '0'),(messageRecu[2]));
				jouerMorpion((int)(messageRecu[0]- '0'),(int)(messageRecu[1]- '0'),(messageRecu[2]),&grille); // place l'icone du client sur la grille du morpion les coordonnées reçu, qui est ici le joueur adverse
				afficheGrille(&grille);
				printf("A ton tour !\n");
				joueTour(&grille,coordo);
				sendMessage(descripteurSocket,coordo); // envoie les coordonnées jouées au serveur
			}
			else if(strstr(messageRecu,"Owins")!=NULL) // cas O a gagne
			{
				jouerMorpion((int)(messageRecu[0]- '0'),(int)(messageRecu[1]- '0'),(messageRecu[2]),&grille); // place les coordonnées reçu sur la grille du morpion sois courant sois adverse
				afficheGrille(&grille);
				printf("O a gagné !\n");
				finished=1;
			}
			else if(strstr(messageRecu,"Xwins")!=NULL) // cas X a gagne
			{
				jouerMorpion((int)(messageRecu[0]- '0'),(int)(messageRecu[1]- '0'),(messageRecu[2]),&grille); // place les coordonnées reçu sur la grille du morpion sois courant sois adverse
				afficheGrille(&grille);
				printf("X a gagné !\n");
				finished=1;
			}
			else if(strstr(messageRecu,"Oend")!=NULL) // cas d'égalité par 0
			{
				jouerMorpion((int)(messageRecu[0]- '0'),(int)(messageRecu[1]- '0'),(messageRecu[2]),&grille); // place les coordonnées reçu sur la grille du morpion sois courant sois adverse
				afficheGrille(&grille);
				printf("O a complété la grille : égalité!\n");
				finished=1;
			}
			else if(strstr(messageRecu,"Xend")!=NULL) // cas d'égalité par X
			{
				jouerMorpion((int)(messageRecu[0]- '0'),(int)(messageRecu[1]- '0'),(messageRecu[2]),&grille); // place les coordonnées reçu sur la grille du morpion sois courant sois adverse
				afficheGrille(&grille);
				printf("X a complété la grille : égalité!\n");
				finished=1;
			}
			else if(strstr(messageRecu,"again")!=NULL) // verif si erreur sur le placement côté serveur
			{
				joueTour(&grille,coordo); // refait joué le client
				sendMessage(descripteurSocket,coordo); // envoie les coordonnées jouées au serveur
			}
			else if(strstr(messageRecu,"close")!=NULL) // verif si déconnexion côté serveur
			{
				printf("déconnexion : l'adversaire est partie");
				finished = 1;
			}
			else{ 
				printf("message reçu inconnu : %s\n",messageRecu);
			}
			

		}
		else if (resultRead == 0) // si déconnecté la boucle est fini
		{
			finished = 1;
		}
		else
		{
			printf("erreur lecture message\n");
		};
	}

	// On ferme la ressource avant de quitter
	close(descripteurSocket);
	return 0;
}
