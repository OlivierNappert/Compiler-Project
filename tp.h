#include <stdlib.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

typedef unsigned char bool;

#define NEW(howmany, type) (type *) calloc((unsigned) howmany, sizeof(type))
#define NIL(type) (type *) 0

/* Etiquettes additionnelles pour les arbres de syntaxe abstraite.
 * Certains tokens servent directement d'etiquette. */
#define E_NE      1
#define E_EQ      2
#define E_LT      3
#define E_LE      4
#define E_GT      5
#define E_GE      6
#define E_IDVAR   7
#define E_CONST   8
#define E_STR	    9
#define E_ADD	    10
#define E_MINUS	  11
#define E_MULT	  12
#define E_DIV	    13
#define E_CONC    14
#define E_ITE	    15
#define E_RETU	  16
#define E_RESULT  17
#define E_THIS    18
#define E_SUPER   19
#define E_LIST    20
#define E_CLASS   21
#define E_AFF     22
#define E_NEW     23
#define E_SM      24
#define E_METH    25
#define E_IS      26
#define E_SEL     27
#define E_INST    28
#define E_LVAR    30
#define E_IDMETH  31
#define E_PREDEF  32
#define E_CAST    33


/* Codes d'erreurs */
#define NO_ERROR	      0
#define USAGE_ERROR	    1
#define LEXICAL_ERROR	  2
#define SYNTAX_ERROR    3
#define CONTEXT_ERROR	  40
#define DECL_ERROR	    41
#define TYPE_ERROR	    42
#define EVAL_ERROR	    50
#define UNEXPECTED	    10

/* Forward declarations des structures utilisées */
typedef struct _varDecl VarDecl, *VarDeclP;
typedef struct _Tree Tree, *TreeP;
typedef struct _Method Method, *MethodP;
typedef struct _Class Class, *ClassP;

/* Structure d'une déclaration de variable */
struct _varDecl
{
  char* name;       /* Nom de la variable */
  char* type;       /* Type de la variable */
  ClassP typeClass; /* Pointeur vers le type de la variable */
  TreeP aff;        /* Possible Tree representant une affectation */

  int level;        /* Niveau d'imbrication de la variable */
  int address;      /* Adresse relative de la variable */
  bool isInClass;   /* Savoir si la variables est dans une classe */

  VarDeclP next;    /* Variable suivant pour representer une file */
};


/* Structure d'un arbre:
 *    Noeud:    nbChildren > 0, u.children
 *    Feuille:  nbChildren = 0, u.str || u.val || u.lvar  */
struct _Tree
{
  short op;           /* Etiquette de l'opérateur courant */
  short nbChildren;   /* Nombre de sous-arbres */
  union {
    char *str;        /* op = IDVAR || STR */
    int val;          /* op = E_CONST */
    VarDeclP lvar;    /* op = une ou plusieurs déclarations de variable */
    MethodP meth;     /* op = E_IDMETH */
    TreeP* children;  /* Tableau des sous-arbres */
  } u;
};


/* Structure d'une méthode */
struct _Method
{
  char* name;     /* Nom */
  char* type;     /* Type de retour */
  bool over;      /* Override ou non */
  VarDeclP args;  /* Arguments */
  TreeP bloc;     /* Variables locales et corps */

  int level;      /* Niveau de la classe associée */
  char* label;    /* Label pour la génération de code */

  MethodP next;   /* Suivant sur la pile */
};


/* Structure d'une classe */
struct _Class
{
  char* name;           /* Nom */
  TreeP super;          /* Nom de la classe mère (avec arguments) */
  ClassP superClass;    /* Pointeur vers la classe mère */
  VarDeclP vars;        /* Variables membre */
  
  int id;                /* Identifiant  de la classe */
  int size;             /* Taille d'une instance de la classe */
  int level;            /* Niveau d'imbrication dans la hiérarchie des classes*/

  MethodP constructor;  /* Méthode constructeur */
  MethodP methods;      /* Méthodes de classe */

  ClassP next;          /* Suivant sur la pile */
};


/* Type de retour de Flex */
typedef union
{
  char *S;
  char C;
  int I;
  TreeP pT;
  VarDeclP pV;
  ClassP pC;
  MethodP pM;
} YYSTYPE;

#define YYSTYPE YYSTYPE

/* Prototype des fonctions de tp.c utilisées dans les autres fichiers */

/* Fonction principale */
void launchProgram(ClassP classes, TreeP main);

/* Fonction des fichiers de chaque étape */
void printFile(ClassP classes, TreeP body);
void checkClass(ClassP class, ClassP classes, int id, ClassP voidClass);
void checkMain(TreeP tree, ClassP classes,ClassP voidClass);
void generateCode(ClassP classes, TreeP body, FILE* f);

/* Constructeurs d'arbres et de feuilles */
TreeP makeTree(short op, int nbChildren, ...);
TreeP makeLeafInt(short op, int val);
TreeP makeLeafStr(short op, char* str);
TreeP makeLeafLVar(short op, VarDeclP lvar);

/* Constructeurs de classes, méthodes et variables */
ClassP makeClass(char* name, TreeP super, MethodP constructor,
    VarDeclP vars, MethodP methods);
MethodP makeMethod(char* name, char* type, bool over,
    VarDeclP args, TreeP bloc);
VarDeclP declVar(char* name, char* type, TreeP aff);
ClassP addClass(ClassP classes, ClassP new);
MethodP addMethod(MethodP methods, MethodP new);
VarDeclP addVarDecl(VarDeclP vars, VarDeclP new);

/* Retourne le rankieme fils d'un arbre (de 0 a n-1) */
TreeP getChild(TreeP tree, int rank);
