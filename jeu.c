/*
	Canvas pour algorithmes de jeux à 2 joueurs

	joueur 0 : humain
	joueur 1 : ordinateur

*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Paramètres du jeu
#define LARGEUR_MAX 42 		// nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 3		// temps de calcul pour un coup avec MCTS (en secondes)

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

// Critères de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {

    int joueur; // à qui de jouer ?
    char plateau[6][7];

} Etat;

// Definition du type Coup
typedef struct {

    int ligne;
    int colonne;

} Coup;

// Copier un état
Etat * copieEtat( Etat * src ) {
   Etat * etat = (Etat *)malloc(sizeof(Etat));

   etat->joueur = src->joueur;

   int i,j;
   for (i=1; i<= 6; i++)
      for ( j=1; j<=7; j++)
         etat->plateau[i][j] = src->plateau[i][j];



   return etat;
}

// Etat initial
Etat * etat_initial( void ) {
   Etat * etat = (Etat *)malloc(sizeof(Etat));

   int i,j;
   for (i=1; i<= 6; i++)
      for ( j=1; j<=7; j++)
         etat->plateau[i][j] = ' ';

   return etat;
}


void afficheJeu(Etat * etat) {

   int i,j;
   printf("   |");
   for ( j = 1; j <=7; j++)
      printf(" %d |", j);
   printf("\n");
   printf("---+---+---+---+---+---+---+---+");
   printf("\n");

   for(i=1; i <= 6; i++) {
      printf(" %d |", i);
      for ( j = 1; j <= 7; j++)
         printf(" %c |", etat->plateau[i][j]);
      printf("\n");
      printf("---+---+---+---+---+---+---+---+");
      printf("\n");
   }
}


// Nouveau coup
Coup * nouveauCoup( int i, int j ) {
   Coup * coup = (Coup *)malloc(sizeof(Coup));

   coup->ligne = i;
   coup->colonne = j;


   return coup;
}


// Demander à l'humain quel coup jouer
Coup * demanderCoup () {
   int i =0 ,j;

   printf(" quelle colonne ? ") ;
   scanf("%d",&j);

   return nouveauCoup(i,j);
}

// Modifier l'état en jouant un coup
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

   int lgn;
   for ( lgn = 1; lgn <=6; ++lgn) {
      if (etat->plateau[lgn][coup->colonne] == ' '){
         coup->ligne = lgn;
      }
   }
   if ( etat->plateau[coup->ligne][coup->colonne] != ' ' )
      return 0;
   else {
      etat->plateau[coup->ligne][coup->colonne] = etat->joueur ? 'O' : 'X';

      // à l'autre joueur de jouer
      etat->joueur = AUTRE_JOUEUR(etat->joueur);

      return 1;
   }
}

// Retourne une liste de coups possibles à partir d'un etat
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {

   Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );

   int k = 0;

   int i,j;
   for(i=1; i <= 6; i++) {
      for (j=1; j <= 7; j++) {
         if ( etat->plateau[i][j] == ' ' ) {
            coups[k] = nouveauCoup(i,j);
            k++;
         }
      }
   }

   coups[k] = NULL;

   return coups;
}


// Definition du type Noeud
typedef struct NoeudSt {

    int joueur; // joueur qui a joué pour arriver ici
    Coup * coup;   // coup joué par ce joueur pour arriver ici

    Etat * etat; // etat du jeu

    struct NoeudSt * parent;
    struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
    int nb_enfants;	// nb d'enfants présents dans la liste

    // POUR MCTS:
    int nb_victoires;
    int nb_simus;

} Noeud;


// Créer un nouveau noeud en jouant un coup à partir d'un parent
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
   Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));

   if ( parent != NULL && coup != NULL ) {
      noeud->etat = copieEtat ( parent->etat );
      jouerCoup ( noeud->etat, coup );
      noeud->coup = coup;
      noeud->joueur = AUTRE_JOUEUR(parent->joueur);
   }
   else {
      noeud->etat = NULL;
      noeud->coup = NULL;
      noeud->joueur = 0;
   }
   noeud->parent = parent;
   noeud->nb_enfants = 0;

   // POUR MCTS:
   noeud->nb_victoires = 0;
   noeud->nb_simus = 0;


   return noeud;
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
   Noeud * enfant = nouveauNoeud (parent, coup ) ;
   parent->enfants[parent->nb_enfants] = enfant;
   parent->nb_enfants++;
   return enfant;
}

void freeNoeud ( Noeud * noeud) {
   if ( noeud->etat != NULL)
      free (noeud->etat);

   while ( noeud->nb_enfants > 0 ) {
      freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
      noeud->nb_enfants --;
   }
   if ( noeud->coup != NULL)
      free(noeud->coup);

   free(noeud);
}


// Test si l'état est un état terminal
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {

   // tester si un joueur a gagné
   int i,j,k;
   int n = 0;
   for (i=1; i <= 6; i++) {
      for(j=1; j <= 7; j++) {
         if ( etat->plateau[i][j] != ' ') {
            n++;	// nb coups joués

            // lignes
            k=0;
            while ( k < 4 && i+k <= 6 && etat->plateau[i+k][j] == etat->plateau[i][j] )
               k++;
            if ( k == 4 )
               return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

            // colonnes
            k=0;
            while ( k < 4 && j+k <= 7 && etat->plateau[i][j+k] == etat->plateau[i][j] )
               k++;
            if ( k == 4 )
               return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

            // diagonales
            k=0;
            while ( k < 4 && i+k <= 6 && j+k <= 7 && etat->plateau[i+k][j+k] == etat->plateau[i][j] )
               k++;
            if ( k == 4 )
               return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

            k=0;
            while ( k < 4 && i+k <= 6 && j-k >= 0 && etat->plateau[i+k][j-k] == etat->plateau[i][j] )
               k++;
            if ( k == 4 )
               return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;
         }
      }
   }

   // et sinon tester le match nul
   if ( n == 6*7 )
      return MATCHNUL;

   return NON;
}



// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {

   clock_t tic, toc;
   tic = clock();
   int temps;

   Coup ** coups;
   Coup * meilleur_coup ;

   // Créer l'arbre de recherche
   Noeud * racine = nouveauNoeud(NULL, NULL);
   racine->etat = copieEtat(etat);

   // créer les premiers noeuds:
   coups = coups_possibles(racine->etat);
   int k = 0;
   Noeud * enfant;
   while ( coups[k] != NULL) {
      enfant = ajouterEnfant(racine, coups[k]);
      k++;
   }


   meilleur_coup = coups[ rand()%k ]; // choix aléatoire

   /*  TODO :
       - supprimer la sélection aléatoire du meilleur coup ci-dessus
       - implémenter l'algorithme MCTS-UCT pour déterminer le meilleur coup ci-dessous*/

   int iter = 0;

   do {






       toc = clock();
       temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
       iter ++;
   } while ( temps < tempsmax );

   /* fin de l'algorithme  */

   // Jouer le meilleur premier coup
   jouerCoup(etat, meilleur_coup );

   // Penser à libérer la mémoire :
   freeNoeud(racine);
   free (coups);
}

int main(void) {

   Coup * coup;
   FinDePartie fin;

   // initialisation
   Etat * etat = etat_initial();

   // Choisir qui commence :
   printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
   scanf("%d", &(etat->joueur) );

   // boucle de jeu
   do {
      printf("\n");
      afficheJeu(etat);

      if ( etat->joueur == 0 ) {
         // tour de l'humain

         do {
            coup = demanderCoup();
         } while ( !jouerCoup(etat, coup) );

      }
      else {
         // tour de l'Ordinateur

         ordijoue_mcts( etat, TEMPS );

      }

      fin = testFin( etat );
   }	while ( fin == NON ) ;

   printf("\n");
   afficheJeu(etat);

   if ( fin == ORDI_GAGNE )
      printf( "** L'ordinateur a gagné **\n");
   else if ( fin == MATCHNUL )
      printf(" Match nul !  \n");
   else
      printf( "** BRAVO, l'ordinateur a perdu  **\n");
   return 0;
}
