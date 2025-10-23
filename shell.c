
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/wait.h>   /* pour avoir wait & co. */
#include <ctype.h>      /* pour avoir isspace & co. */
#include <string.h>
#include <errno.h>      /* pour avoir errno */
#define MAXELEMS 40
char* elems[MAXELEMS];
typedef enum {
    SEP_NONE,
    SEP_SEMICOLON,
    SEP_AND,
    SEP_OR
} Separator;

Separator separators[MAXELEMS];


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
  printf("%s\n~", cwd);
  fflush(stdout);
}

void lit_ligne()
{
  if (!fgets(ligne,sizeof(ligne)-1,stdin)) { // lire la commande taper par l'utilisateur et la stocker dans ligne 
    /* ^D ou fin de fichier => on quittte */
    printf("\n");
    exit(0);
  }
  ligne[strcspn(ligne, "\n")] = '\0';
}



void decoupe() {
    char *debut = ligne;
    int i = 0;

    while (*debut && i < MAXELEMS - 1) {
        // sauter les espaces au début
        while (*debut && isspace(*debut)) debut++;

        if (!*debut) break;

        // mémoriser le début de la commande
        elems[i] = debut;

        // chercher le prochain séparateur
        separators[i] = SEP_NONE; // par défaut
        while (*debut) {
            if (*debut == ';') {
                separators[i] = SEP_SEMICOLON;
                *debut = '\0';
                debut++;
                break;
            } else if (*debut == '&' && *(debut+1) == '&') {
                separators[i] = SEP_AND;
                *debut = '\0';
                debut += 2;
                break;
            } else if (*debut == '|' && *(debut+1) == '|') {
                separators[i] = SEP_OR;
                *debut = '\0';
                debut += 2;
                break;
            }
            debut++;
        }

        // enlever les espaces de fin
        char *fin = elems[i] + strlen(elems[i]) - 1;
        while (fin > elems[i] && isspace(*fin)) *fin-- = '\0';

        i++;
    }

    elems[i] = NULL;
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

void execute() {
    pid_t pid;
    int i = 0;
    int last_status = 0; // statut de la commande précédente

    while (elems[i] != NULL) {
        // Vérifier condition pour && et ||
        if (i > 0) {
            if (separators[i-1] == SEP_AND && last_status != 0) {
                i++;
                continue; // && : exécuter seulement si la précédente a réussi
            }
            if (separators[i-1] == SEP_OR && last_status == 0) {
                i++;
                continue; // || : exécuter seulement si la précédente a échoué
            }
        }

        // découper en arguments
        char *args[MAXELEMS];
        char *token = strtok(elems[i], " \t");
        int j = 0;
        while (token && j < MAXELEMS - 1) {
            args[j++] = token;
            token = strtok(NULL, " \t");
        }
        args[j] = NULL;

        // fork pour exécuter la commande
        pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }

        if (pid == 0) {
            execvp(args[0], args);
            perror(args[0]);
            exit(1);
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
                last_status = WEXITSTATUS(status);
            else
                last_status = 1; // si fin par signal => considérer comme échec
        }

        i++;
    }
}

        

   

int main()
{
  /* boucle d'interaction */
  while (1) {
    affiche_invite();
    lit_ligne();
    decoupe();
    execute();
  }
  return 0;
}
