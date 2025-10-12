
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/wait.h>   /* pour avoir wait & co. */
#include <ctype.h>      /* pour avoir isspace & co. */
#include <string.h>
#include <errno.h>      /* pour avoir errno */



//Afficher le prompt → getcwd() + printf() + fflush().

//Lire la commande → fgets().

/***Supprimer le \n → strchr().

Vérifier si ligne vide → strcmp().

Découper en commande + arguments → strtok().

Créer un fils → fork().

Exécuter la commande → execvp().

Attendre la fin du fils → waitpid() + macros WIFEXITED(), WEXITSTATUS().

Gérer les erreurs → errno + strerror().
// stdin represente l'entrée standart 
// fgets nous permet de lire ce qui est ecrit dans l'entrée standart 
*/

char ligne[4096];       /* contient la commande que je viens de taper */


void affiche_invite()
{
  char cwd[1024];
  getcwd(cwd,sizeof(cwd)); // recuperer le chemin dont je suis en ce moment 
  printf("%s~", cwd);
  fflush(stdout);
}

void lit_ligne()
{
  if (!fgets(ligne,sizeof(ligne)-1,stdin)) { // lire la commande taer par l'utilisateur et la stocker dans ligne 
    /* ^D ou fin de fichier => on quittte */
    printf("\n");
    exit(0);
  }
}

/* attent la fin du processus pid  c'est le parent qui le shel qui execute cette fonction */
void attent(pid_t pid)
{
  /* il faut boucler car waitpid peut retourner avec une erreur non fatale */
  while (1) {
    int status;
    int r = waitpid(pid,&status,0); /* attente bloquante */
    if (r<0) { 
      if (errno==EINTR) continue; /* interrompu => on recommence Ã  attendre */
      printf("erreur de waitpid (%s)\n",strerror(errno));
      break;
    }
    if (WIFEXITED(status))
      printf("terminaison normale, status %i\n",WEXITSTATUS(status));
    if (WIFSIGNALED(status))
      printf("terminaison par signal %i\n",WTERMSIG(status));
    break;
  }
}

/* execute la ligne */
void execute()
{
  pid_t pid;

  /* supprime le \n final */
  if (strchr(ligne,'\n')) *strchr(ligne,'\n') = 0;

  /* saute les lignes vides */
  if (!strcmp(ligne,"")) return;

  pid = fork(); // creation dun processus fils qui va executer la commande taper par l'utilisateur 
  if (pid < 0) {
    printf("fork a Ã©chouÃ© (%s)\n",strerror(errno));
    return;
  }

  if (pid==0) { 
    /* fils */
    execlp(ligne, /* programme Ã  exÃ©cuter */
	   ligne, /* argv[0], par convention le nom de programme exÃ©cutÃ© */
	   NULL   /* pas d'autre argument */
	   );

    /* on n'arrive ici que si le exec a Ã©chouÃ© */
    printf("impossible d'Ã©xecuter \"%s\" (%s)\n",ligne,strerror(errno));
    exit(1);
  }
  else {
    /* père */
    attent(pid);
  }
}

int main()
{
  /* boucle d'interaction */
  while (1) {
    affiche_invite();
    lit_ligne();
    execute();
  }
  return 0;
}
