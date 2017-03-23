#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include "tp.h"
#include "tp_y.h"

extern int yyparse();
extern int yylineno;

/* Affichage ou non de l'AST généré */
bool verbose = FALSE;

/* Generation de code ou pas. Par defaut, on produit le code */
bool noCode = FALSE;

/* Pose de points d'arret ou pas dans le code produit */
bool debug = FALSE;

/* Code d'erreur à retourner */
int errorCode = NO_ERROR;

/* Fichier de sortie pour le code engendré */
FILE *out = NIL(FILE);


/* Forward-declaration des fonctions utilisées uniquement dans tp.c */
ClassP addPrimitives(ClassP classes);
ClassP generatePrimitives();
ClassP generateInteger();
ClassP generateString();


/* Fonction appelée au lancement du programme */
int main(int argc, char **argv) {
  int fi;
  int i, res;

  for(i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
    {
      switch (argv[i][1])
      {
        case 'd':
        case 'D':
          debug = TRUE;
          continue;
        case 'v':
        case 'V':
          verbose = TRUE;
          continue;
        case 'e':
        case 'E':
          noCode = TRUE;
          continue;
        case '?':
        case 'h':
        case 'H':
          fprintf(stderr, "Appel: tp -v -e -d -o file.out programme.txt\n");
          exit(USAGE_ERROR);
        case'o':
          if ((out= fopen(argv[++i], "wb")) == NULL)
          {
            fprintf(stderr, "erreur: Cannot open %s\n", argv[i]);
            exit(USAGE_ERROR);
          }
          break;
      default:
        fprintf(stderr, "Option inconnue: %c\n", argv[i][1]);
        exit(USAGE_ERROR);
      }
    }
    else
      break;
  }

  if (i == argc) {
    fprintf(stderr, "Fichier programme manquant\n");
    exit(USAGE_ERROR);
  }

  if ((fi = open(argv[i++], O_RDONLY)) == -1) {
    fprintf(stderr, "erreur: Cannot open %s\n", argv[i-1]);
    exit(USAGE_ERROR);
  }

  /* redirige l'entree standard sur le fichier... */
  close(0); dup(fi); close(fi);

  res = yyparse();
  return res ? SYNTAX_ERROR : errorCode;
}


/* Lance toutes les étapes de la compilation une fois l'analyse lexicale
 * et syntaxique effectuées */
void launchProgram(ClassP classes, TreeP main)
{
  /* Affichage de l'AST généré */
  if(verbose)
    printFile(classes, main);
  
  /* Ajout des classes primitives */
  classes = addPrimitives(classes);
  
  /* Classe de retour de fonction nul utilisée pour les vérifications */
  ClassP voidClass = makeClass("Void", NIL(Tree),
      NIL(Method), NIL(VarDecl), NIL(Method));
  voidClass->level = 0;
  
  /* Vérifications contextuelles */
  checkClass(classes->next->next, classes, 0, voidClass);
  checkMain(main, classes, voidClass);

  /* Génération du code s'il n'y a pas eu d'erreur */
  if(!noCode && errorCode == NO_ERROR)
    generateCode(classes, main, out);
}


/* Change le code d'erreur courrant, mais n'arrête pas le programme */
void setError(int code)
{
  errorCode = code;
  if (code != NO_ERROR) 
    noCode = TRUE;
}


/* yyerror:  fonction importee par Bison et a fournir explicitement. Elle
 * est appelee quand Bison detecte une erreur syntaxique.
 * Ici on se contente d'un message minimal.
 */
void yyerror(char *ignore)
{
  printf("erreur de syntaxe: Ligne %d\n", yylineno);
  setError(SYNTAX_ERROR);
}


/* Retourne le rankieme fils d'un arbre (de 0 a n-1) */
TreeP getChild(TreeP tree, int rank)
{
  if (tree->nbChildren < rank -1)
  { 
    fprintf(stderr, "Incorrect rank in getChild: %d\n", rank);
    abort();
  }
  return tree->u.children[rank];
}


/* Ajoute les classes primitives (String et Integer) à la
 * liste des classes */
ClassP addPrimitives(ClassP classes)
{
 	ClassP tempClasses = generatePrimitives();
  tempClasses->next->next = classes;
  return tempClasses; 
}


/* Génère les classes primitives (String et Integer) */
ClassP generatePrimitives()
{
  ClassP tempString = generateString();
  tempString->size = 1;
  ClassP tempInteger = generateInteger();
  tempInteger->size = 1;
  tempString->next = tempInteger;
  tempInteger->next = NIL(Class);
  return tempString;
}


/* Génère la classe primitive pour l'Integer */
ClassP generateInteger()
{  
  /* Constructeur d'un Integer */
  MethodP integerConstructor = makeMethod("Integer", NIL(char), 
          FALSE, NIL(VarDecl), NIL(Tree));
  
  /* Liste des methodes disponibles pour un Integer */
  MethodP printMethod = makeMethod("toString", "String", 
                        FALSE, NIL(VarDecl), makeTree(E_PREDEF,0));          
                        
  /* Creation de la classe Integer */
  return makeClass("Integer", NIL(Tree), 
          integerConstructor, NIL(VarDecl), printMethod);
          
}


/* Génère la classe primitive pour le String */
ClassP generateString()
{
  MethodP stringConstructor = makeMethod("String", NIL(char), 
          FALSE, NIL(VarDecl), NIL(Tree));
  
  /* Liste des methodes disponibles pour un String */
  MethodP printMethod = addMethod(makeMethod("print", "String", FALSE, 
    NIL(VarDecl), makeTree(E_PREDEF,0)), makeMethod("println", "String", 
    FALSE, NIL(VarDecl), makeTree(E_PREDEF,0))); 
                         
  /* Creation de la classe String */
  return makeClass("String", NIL(Tree), 
          stringConstructor, NIL(VarDecl), printMethod);
}


/* Constructeur de classe */
ClassP makeClass(char* name, TreeP super, MethodP constructor,
    VarDeclP vars, MethodP methods)
{
  ClassP class = NEW(1, Class);
  class->name = name;
  class->super = super;
  class->superClass = NIL(Class);
  class->vars = vars;
  class->size = 0;
  class->constructor = constructor;
  class->methods = methods;

  return class;
}


/* Constructeur de méthode */
MethodP makeMethod(char* name, char* type, bool over,
    VarDeclP args, TreeP bloc)
{
  MethodP method = NEW(1, Method);
  method->name = name;
  method->type = type;
  method->over = over;
  method->args = args;
  method->bloc = bloc;

  return method;
}


/* Ajoute une classe à la liste des classes */
ClassP addClass(ClassP classes, ClassP new)
{
  /* Ajout de la classe en tête de pile */
  new->next = classes;
  classes = new;
  return classes;
}


/* Ajoute une méthode à la liste des méthodes d'une classe */
MethodP addMethod(MethodP methods, MethodP new)
{
 /* Ajout de la méthode en tête de pile */
  new->next = methods;
  methods = new;
  return methods;
}


/* Constructeur de noeud
 * Non utilisé directement */
TreeP makeNode(int nbChildren, short op) 
{
  TreeP tree = NEW(1, Tree);
  tree->op = op;
  tree->nbChildren = nbChildren;
  tree->u.children = nbChildren > 0 ? NEW(nbChildren, TreeP) : NIL(TreeP);
  return(tree);
}


/* Construction d'un arbre a nbChildren branches, passees en parametres */
TreeP makeTree(short op, int nbChildren, ...) 
{
  va_list args;
  int i;
  TreeP tree = makeNode(nbChildren, op); 
  va_start(args, nbChildren);
  for (i = 0; i < nbChildren; i++) { 
    tree->u.children[i] = va_arg(args, TreeP);
  }
  va_end(args);
  return(tree);
}


/* Constructeur de feuille dont la valeur est une chaine de caracteres */
TreeP makeLeafStr(short op, char *str) 
{
  TreeP tree = makeNode(0, op);
  tree->u.str = str;
  return tree;
}


/* Constructeur de feuille dont la valeur est un entier */
TreeP makeLeafInt(short op, int val) 
{
  TreeP tree = makeNode(0, op); 
  tree->u.val = val;
  return(tree);
}


/* Constructeur de feuille représentant un identificateur de variable */
TreeP makeLeafLVar(short op, VarDeclP lvar) 
{
  TreeP tree = makeNode(0, op); 
  tree->u.lvar = lvar;
  return(tree);
}


/* Ajoute la variable en tête de file si son nom n'est pas déjà utilisé  */
VarDeclP addVarDecl(VarDeclP vars, VarDeclP new)
{
  VarDeclP it;
  for(it = vars; it != NIL(VarDecl); it = it->next)
  {
    if (!strcmp(it->name, new->name))
    {
      fprintf(stderr, "Error: Multiple declaration in the same scope of %s\n",
        it->name);
      setError(CONTEXT_ERROR);
      break;
      /* Ne termine pas à la compilation de façon à repérer d'autres erreurs */
    }
  }

  new->next = vars;
  vars = new;
  return new;
}


/* Créé une variable et effectue les vérifications contextuelles */
VarDeclP declVar(char *name, char* type, TreeP aff)
{
  VarDeclP nvar = NEW(1, VarDecl);
  nvar->name = name;
  nvar->type = type;
  nvar->aff = aff;
  nvar->level = 0;
  nvar->isInClass = FALSE;
  nvar->next = NIL(VarDecl);
  nvar->typeClass = NIL(Class);
  return nvar;
}
