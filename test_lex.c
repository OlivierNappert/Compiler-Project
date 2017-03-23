/*
 * Un petit programme de demonstration qui n'utilise que l'analyse lexicale.
 * Permet principalement de tester la correction de l'analyseur lexical et de
 * l'interface entre celui-ci et le programme qui l'appelle.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "tp.h"
#include "tp_y.h"


#define TABSIZE	1000
extern char *strdup(const char *);

/* Fonction appelee par le programme principal pour obtenir l'unite lexicale
 * suivante. Elle est produite par Flex (fichier tp_l.c)
 */
extern int yylex (void);

/* La chaine de caractere en cours de lecture : definie et mise a jour
 * dans tp_l.c
 */
extern char *yytext;

/* Le numero de ligne courant : defini et mis a jour dans tp_l.c */
extern int yylineno;

/* Pour chaque identificateur, memorise la ligne de sa premiere apparition */
typedef struct {
  int line;
  char *id;
} IdentS, *IdentP;

/* (future) table des identificateurs avec leur numero de ligne */
IdentP idents = NIL(IdentS);

/* Variable pour interfacer flex avec le programme qui l'appelle, notamment
 * pour transmettre de l'information, en plus du type d'unite reconnue.
 * Le type YYSTYPE est defini dans tp.h.
 */
YYSTYPE yylval;

bool verbose = FALSE;

/* Compteurs */
int nbIdent = 0;

void setError(int code) {
  /* presente  juste pour des raisons de compatibilite */
}



/* Recherche un identificateur dans la table */
IdentP getIdent(char * id) {
  int i;
  for (i = 0; i < nbIdent; i++) {
    if (!strcmp(id, idents[i].id)) return(&idents[i]);
  }
  return NIL(IdentS);
}

/* Cree un identificateur */
IdentP makeIdent(int line, char *id) {
  IdentP ident = getIdent(id);
  if (!ident) {
    ident = &idents[nbIdent++];
    ident->line = line;
    /* Si on a duplique la chaine dans la partie Flex, il n'y a pas de raison
     * de le faire a nouveau ici.
     * Comme ca risque de dependre de ce que les uns et les autres font dans
     * tp.l, on prefere le (re-)faire ici.
     */
    ident->id = strdup(id);
  }
  return(ident);
}

/* format d'appel */
void help() {
  fprintf(stderr, "Appel: tp [-h] [-v] programme.txt\n");
}

/* Appel:
 *   tp [-option]* programme.txt
 * Les options doivent apparaitre avant le nom du fichier du programme.
 * Options: -[vV] -[hH?]
 */
int main(int argc, char **argv) {
  idents = NEW(TABSIZE, IdentS); /* liste des identificateurs */
  int fi;
  int token;
  int i;

  for(i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'v': case 'V':
	verbose = TRUE; continue;
      case '?': case 'h': case 'H':
	help();
	exit(1);
      default:
	fprintf(stderr, "Option inconnue: %c\n", argv[i][1]);
	exit(1);
      }
    } else break;
  }

  if (i == argc) {
    fprintf(stderr, "Erreur: Fichier programme manquant\n");
    help();
    exit(1);
  }

  if ((fi = open(argv[i++], O_RDONLY)) == -1) {
    fprintf(stderr, "Erreur: fichier inaccessible %s\n", argv[i-1]);
    help();
    exit(1);
  }

  /* redirige l'entree standard sur le fichier... */
  close(0); dup(fi); close(fi);

  while (1) {
    token = yylex();
    switch (token) {
    case 0: /* EOF */
      printf("\nFin de l'analyse lexicale\n\n");
      printf("Liste des identificateurs avec leur premiere occurrence:\n");
      for (i = 0; i < nbIdent; i++) {
        printf("ligne %d : %s\n", idents[i].line, idents[i].id);
      }
      printf("\nNombre d'identificateurs: %4d\n", nbIdent);
      return 0;
    case Id:
      makeIdent(yylineno, yylval.S);
      if (verbose) printf("\nIdentificateur:\t\t%s", yylval.S);
      break;
    case Cste:
      /* ici on suppose qu'on a recupere la valeur de la constante, pas sa
       * representation sous forme de chaine de caracteres.
       */
      printf("\nConstante:\t\t%d", yylval.I);
      break;
    case LITSTR:
      makeIdent(yylineno, yylval.S);
      if(verbose) printf("\nChaine de caracteres:\t\t%s", yylval.S);
      break;
    case IS:
    case CLASS:
    case VAR:
    case EXTENDS:
    case DEF:
    case RETURN:
    case AS:
    case IF:
    case THEN:
    case ELSE:
    case THIS:
    case SUPER:
    case RESULT:
      if (verbose) printf("\nMot-clef:\t\t%s",  yytext);
      break;
    case '(':
    case ')':
    case ',':
    case '{':
    case '}':
    case ':':
    case ';':
    case '.':
      /* a completer */
      if (verbose) printf("\nSymbole:\t\t%s",  yytext);
      break;
    case ADD:
    case SUB:
    case MUL:
    case QUO:
      if (verbose) printf("\nOper arithmetique:\t%s", yytext);
      break;
    case AFF:
      if (verbose) printf("\nAffectation:\t%s", yytext);
      break;
    case CONC:
      if(verbose) printf("\nConcatenation:\t%s", yytext);
      break;
    case NEW2:
      if (verbose) printf("\nDeclaration:\t%s", yytext);
      break;
    case TYPE:
      if (verbose) printf("\nType:\t%s", yytext);
      break;
    case OVERRIDE:
      if (verbose) printf("\nSurcharge:\t%s", yytext);
      break;
    case RelOp:
      /* inutilement complique ici, mais sert a illustrer la difference
       * entre le token et l'infirmation supplementaire qu'on peut associer
       * via la valeur.
       */
      if (verbose) { 
				printf("\nOperateur de comparaison:\t%s ", yytext);
        switch(yylval.C)
        {
          case EQ:
            printf("egalite");
            break;
          case NE:
            printf("different");
            break;
          default:
            printf("inconnu");
        }
      }
      break;
    default:
      printf("\nToken non reconnu:\t\"%d\"\n", token);
    }
  }
}
