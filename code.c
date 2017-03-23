#include <string.h>
#include "tp.h"
#include "tp_y.h"

extern char* strdup(const char *);


/* Nombre de structures if...then...else */
int iteNumber = 0;


/* Forward-declaration des fonctions privées */
void classesConstructors(ClassP class, ClassP allClasses, char*** labels, FILE* f);
int instructionCode(TreeP ins, int argsNb,
    ClassP classes, char*** labels, FILE* f);
void allocObject(VarDeclP obj, ClassP classes, char*** labels, FILE* f);
void bodyCode(TreeP body, int argsNb, ClassP classes, char*** labels, FILE* f);
void methodsCode(ClassP classes, ClassP allClasses, char*** labels, FILE* f);
int getNbMethods(ClassP class);
int getNbArgs(MethodP method);
void generateLabels(ClassP classes);
char*** virtualTables(ClassP classes, FILE* f);
void freeLabels(ClassP classes);
void callMethod(TreeP caller, TreeP called, TreeP args, char*** labels, 
    ClassP classes, FILE* f);
int childsCode(TreeP node, int argsNb, ClassP classes, char*** labels, FILE* f);
int getNbClassesWithMethod(ClassP classes);
void generateCalls(FILE* f, ClassP classes);
void freeTable(char*** labels, ClassP classes);
void generateVirtualTables(ClassP classes, char*** labels, FILE* f);


/* Fonction principale appelée par le programme */
void generateCode(ClassP classes, TreeP body, FILE* f)
{
  /* Ouverture du fichier par défaut si aucun n'est spécifié */
  if(f == NIL(FILE))
    f = fopen("code.interp", "wb");
  
  if(f == NIL(FILE))
  {
    printf("Cannot create or open code generation file.\n"
        "Please check active directory and output file permissions.");
    exit(UNEXPECTED);
  }
  
  /* Génération des label des méthodes */
  generateLabels(classes);
  char*** labels = virtualTables(classes->next->next, f);

  /* JUMP init -> JUMP main pour tester sans les tables virtuelles */
  fprintf(f, "JUMP init\n"
      "\n\n-- Constructeurs des classes du programme --\n");
  classesConstructors(classes->next->next, classes, labels, f);
  fprintf(f, "\n\n-- Etiquettes callX --\n");
  generateCalls(f, classes);
  fprintf(f, "\n\n-- Tables virtuelles --\n");
  generateVirtualTables(classes->next->next, labels, f);
  fprintf(f, "\n\tPUSHA main\n\tCALL\n\tSTOP\n"
      "\n\n-- Methodes de chacune des classes --\n");
  methodsCode(classes->next->next, classes, labels, f);
  fprintf(f, "\n\n-- Instructions de la fonction principale --\n"
      "main: NOP\n");
  bodyCode(body, 0, classes, labels, f);
  fprintf(f, "\tRETURN\n");

  fclose(f);
  freeTable(labels, classes);
  freeLabels(classes);
}


/* Allocation d'un objet composé d'autres objets (primitifs ou non)
 * L'offset du premier appel donne l'adresse courante en mémoire */
void allocObject(VarDeclP obj, ClassP classes, char*** labels, FILE* f)
{
  if(obj != NIL(VarDecl))
  {
    if(obj->aff) /* Si l'objet a une affectation */
    {
      instructionCode(obj->aff, 0, classes, labels, f);
    }
    else /* Sinon, initialisation par défaut */
    {
      if(!strcmp(obj->type, "Integer"))
        fprintf(f, "\tPUSHI 0\n");
      else if(!strcmp(obj->type, "String"))
        fprintf(f, "\tPUSHS \"\"\n");
      else
        fprintf(f, "\tPUSHS \"%s_emptyVar\"\n", obj->name);
    }
  }
}


/* Génération du code pour une instruction
 * Retourne le nombre d'objets ajoutés à la pile */
int instructionCode(TreeP ins, int argsNb,
    ClassP classes, char*** labels, FILE* f)
{
  int iteNumberTemp;
  TreeP itP;

  if(ins != NIL(Tree))
  {
    if(ins->nbChildren > 0) /* C'est un noeud */
    {
      switch(ins->op)
      {
        /* Sous-bloc */
        case E_IS:
          bodyCode(ins, argsNb, classes, labels, f);
          break;

        /* Appels de méthode */
        case E_NEW: /* Appel de constructeur */
          fprintf(f, "\tPUSHI 0\n");
          argsNb = instructionCode(ins->u.children[1],
              0, classes, labels, f);
          fprintf(f, "\tPUSHI 0\n\tPUSHA %s\n\tCALL\n",
              ins->u.children[0]->u.str);
          if(argsNb > 0)
            fprintf(f, "\tPOPN %d\n", argsNb + 1);
          return 1;
        case E_SM: /* Appel de méthode */
	        callMethod(ins->u.children[0], ins->u.children[1]->u.children[0], 
	        ins->u.children[1]->u.children[1], labels, classes, f);
          break;

        /* Opérateurs de comparaison */
        case E_EQ: /* Egalité */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tEQUAL\n");
          break;
        case E_NE: /* Inégalité */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tEQUAL\n\tNOT\n");
          break;
        case E_LE: /* Plus petit ou égal */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tINFEQ\n");
          break;
        case E_GE: /* Plus grand ou égal */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tSUPEQ\n");
          break;
        case E_LT: /* Strictement plus petit */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tINF\n");
          break;
        case E_GT: /* Strictement plus grand */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tSUP\n");
          break;
       
        /* Opérandes */
        case E_ADD: /* Addition */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tADD\n");
          break;
        case E_MINUS: /* Soustraction */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tSUB\n");
          break;
        case E_MULT: /* Multiplication */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tMUL\n");
          break;
        case E_DIV: /* Division */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tDIV\n");
          break;
        case E_CONC: /* Division */
          childsCode(ins, argsNb, classes, labels, f);
          fprintf(f, "\tCONCAT\n");
          break;

        /* Structures du programme */
        case E_AFF: /* Affectation */
          /* On ignore les this, super, etc */
          for(itP = ins;
              itP->op != E_IDVAR && itP->op != E_RESULT;
              itP = itP->u.children[0]);
          if(itP->op == E_IDVAR)
          {
            if(itP->u.lvar->isInClass)
            {
              fprintf(f, "\tPUSHL -1\n");
              instructionCode(ins->u.children[1], argsNb, classes, labels, f);
              fprintf(f, "\tSTORE %d\n",
                  itP->u.lvar->address);
            }
            else
            {
              instructionCode(ins->u.children[1], argsNb, classes, labels, f);
              fprintf(f, "\tSTOREL %d\n",
                  itP->u.lvar->address);
            }
          }
          else /* Cas result */
          {
            instructionCode(ins->u.children[1], argsNb, classes, labels, f);
            fprintf(f, "\tSTOREL %d\n", -1 * argsNb - 2);
          }
          break;
        case E_ITE: /* if ... then ... else */
          iteNumberTemp = ++iteNumber;
          instructionCode(ins->u.children[0], argsNb, classes, labels, f);
          fprintf(f, "\tJZ if_else_%d\n", iteNumberTemp);
          instructionCode(ins->u.children[1], argsNb, classes, labels, f);
          fprintf(f, "\tJZ if_end_%d\n"
              "\tif_else_%d: NOP\n", iteNumberTemp, iteNumberTemp);
          instructionCode(ins->u.children[2], argsNb, classes, labels, f);
          fprintf(f, "\tif_end_%d: NOP\n", iteNumberTemp);
          break;

        /* Autres */
        case E_SEL: /* Séléction (ex: this.x) */
          return instructionCode(ins->u.children[0], argsNb,classes, labels, f);
        case E_CAST: /* Cast (instruction as Class) */
          break;
        case E_LIST: /* Liste */
          argsNb = instructionCode(ins->u.children[1],0,classes,labels, f);
          return argsNb + instructionCode(ins->u.children[0], argsNb,
              classes, labels, f);
      }
    }
    else /* C'est une feuille */
    {
      switch(ins->op)
      {
        /* Constantes */
        case E_CONST: /* Entier */
          fprintf(f, "\tPUSHI %d\n", ins->u.val);
          return 1;
        case E_STR: /* Chaine de caractères */
          fprintf(f, "\tPUSHS %s\n", ins->u.str);
          return 1;

        /* Identifiants */
        case E_IDVAR: /* Identifiant */
          if(ins->u.lvar->isInClass)
            fprintf(f, "\tPUSHL -1\n\tLOAD %d\n", ins->u.lvar->address);
          else
            fprintf(f, "\tPUSHL %d\n", ins->u.lvar->address);
          return 1;
        case E_RESULT: /* Retour de fonction */
          fprintf(f, "\tPUSHL %d\n", 0);
          return 1;
        case E_THIS: /* Classe-courante */
        case E_SUPER: /* Super-classe */
          /* Pas de code généré ; lien fait à la vérification */
          break;

        /* Instructions simples */
        case E_RETU: /* Fin de fonction */
          fprintf(f, "\tRETURN\n");
          break;
      }
    }
  }
  return 0;
}

/* Génère le code des fils d'un arbre.
 * Retourne le nombre d'objets ajoutés à la pile */
int childsCode(TreeP node, int argsNb, ClassP classes, char*** labels, FILE* f)
{
  int i;
  for(i = 0; i < node->nbChildren; i++)
    instructionCode(node->u.children[i], argsNb, classes, labels, f);
  return i;
}


/* Génère le code des méthodes du programme */
void methodsCode(ClassP classes, ClassP allClasses, char*** labels, FILE* f)
{
  ClassP itC;
  MethodP itM;

  for(itC = classes; itC != NIL(Class); itC = itC->next)
  {
    for(itM = itC->methods; itM != NIL(Method); itM = itM->next)
    {
      fprintf(f, "\n%s: NOP\n", itM->label);
      bodyCode(itM->bloc, getNbArgs(itM), allClasses, labels, f);
      fprintf(f, "\tRETURN\n");
    }
  }
}


/* Génération du code des constructeurs des classes du programme */
void classesConstructors(ClassP classes, ClassP allClasses, char*** labels,FILE* f)
{
  if(classes != NIL(Class))
  {
    VarDeclP itV;
    int nbArgs = getNbArgs(classes->constructor);

    /* Début du constructeur publique accessible par tous */
    fprintf(f, "\n%s: \tALLOC %d\t\t-- Objet de type %s\n",
        classes->name, classes->size, classes->name);

    /* Stockage de la table virtuelle */
    fprintf(f, "\tDUPN 1\n\tPUSHG %d\n"
        "\tSTORE 0\t\t-- table virtuelle\n", classes->id);

    /* Retour de l'objet alloué */
    fprintf(f, "\tDUPN 1\n\tSTOREL -1\n\tSTOREL %d\n", -1 * nbArgs - 2);

    /* Appel du constructeur */
    fprintf(f, "\tPUSHA _%s\n\tCALL\n",
        classes->name);

    /* Fin du constructeur publique */
    fprintf(f, "\tRETURN\n");


    /* Début du constructeur privé accessible par le constructeur
     * publique et les constructeurs des super classes */
    fprintf(f, "_%s:", classes->name);

    /* Appel du constructeur de la classe mère si elle existe */
    if(classes->superClass)
    {
      /* Arguments */
      instructionCode(classes->super->u.children[1], 0, allClasses, labels, f); 
      fprintf(f, "\tPUSHL -1\n\tPUSHA _%s\n\tCALL\n",
          classes->superClass->name);
      if((nbArgs = getNbArgs(classes->superClass->constructor)) > 0)
        fprintf(f, "\tPOPN %d\n", nbArgs);
    }

    /* On ignore les variables des classes au dessus dans la hiérarchie */
    for(itV = classes->vars;
        itV != NIL(VarDecl) && itV->level < classes->level;
        itV = itV->next);
    /* Allocation des variables de la classe courrante */
    fprintf(f, "\tPUSHL -1\n");
    for(; itV != NIL(VarDecl); itV = itV->next)
    {
      fprintf(f, "\tDUPN 1\n");
      allocObject(itV, allClasses, labels, f);
      fprintf(f, "\tSTORE %d\t\t-- var %s (%d) : %s\n",
          itV->address, itV->name, itV->level, itV->type);
    }

    /* Appel du constructeur parametré de la classe courrante s'il existe */
    if(classes->constructor)
      bodyCode(classes->constructor->bloc, getNbArgs(classes->constructor), 
	  allClasses, labels, f);

    /* Fin du constructeur */
    fprintf(f, "\tRETURN\n");

    /* Appel récursif pour générer le code des autres classes */
    classesConstructors(classes->next, allClasses, labels, f);
  }
}


/* Génère le code d'une méthode */
void bodyCode(TreeP body, int argsNb, ClassP classes, char*** labels, FILE* f)
{
  if(body != NIL(Tree))
  {
    TreeP itT;
    VarDeclP itV;

    if(body->op == E_IS) /* Bloc de déclaration et d'instructions */
    {
      if(body->nbChildren == 2)
      {
        /* Allocation des variables de la méthode */
        for(itV = body->u.children[0]->u.lvar;
           itV != NIL(VarDecl);
           itV = itV->next)
           allocObject(itV, classes, labels, f);
        itT = body->u.children[1];
      }
      else
        itT = body->u.children[0];

      /* Génération des instruction*/
			if(itT != NIL(Tree))
			{	
				if(itT->op == E_INST)
				{
					for(; itT != NIL(Tree) && itT->op == E_INST; itT = itT->u.children[1])
						instructionCode(itT->u.children[0], argsNb, classes, labels, f);
				}
				instructionCode(itT,argsNb,classes,labels,f); /* Dernière instruction */
			}
    }
    else /* Cas à une seule instruction */
    {
      instructionCode(body, argsNb, classes, labels, f);
      fprintf(f, "\tPUSHL %d\n", -1 * argsNb - 2);
    }
  }
}

/* cette fonction renvoie le nombre de methodes d'une classe
 * on compte le nombre de methodes d'une classe et de ses super classes
 * mais on ne compte les methodes qui sont redefinies dans des sous classes
 * qu'une seule fois*/
int getNbMethods(ClassP class)
{
  int nbMethods = 0;
  MethodP itM = class->methods;
  while(itM != NIL(Method))
  {
    /* on ne compte que les methodes qui ne sont pas des redefinitions */
    if(itM->over == FALSE)
      nbMethods++;
    itM = itM->next;
  }

  /* on doit egalement compter les methodes des superClasses */
  if(class->superClass != NIL(Class))
    nbMethods += getNbMethods(class->superClass);

  return nbMethods;
}

int getNbArgs(MethodP method)
{
  int nbArgs = 0;
  VarDeclP itV;
  for(itV = method->args; itV != NIL(VarDecl); itV = itV->next)
    nbArgs++;

  return nbArgs;
}

/* Cette fonction genere les labels des methodes de chaque classe
 * ces labels seront utilise pour generes les tables virtuelles
 * les labels sont composes du nom de la facon suivante :
 * nomClasse_nomMethode */
void generateLabels(ClassP classes)
{
  ClassP itC = classes;
  MethodP itM;

  while(itC != NIL(Class))
  {
    itM = itC->methods;
    while(itM != NIL(Method))
    {
      itM->label = malloc(128);
      sprintf(itM->label, "%s_%s", itC->name, itM->name);
      itM = itM->next;
    }
    itC = itC->next;
  }
}

/* cette fonction renvoie le nombre de classe qui ont au moins une classe
 * une classe qui n'a pas de methodes mais qui est une sous classe d'une 
 * classe qui en a est consideree comme ayant des methodes */
int getNbClassesWithMethod(ClassP classes)
{
  ClassP itC = classes;
  /*on avance de 2 classes pour sauter Integer et String */
  int nbClass = 0;

  while(itC != NIL(Class))
  {
    if(getNbMethods(itC) != 0)
      nbClass++;

    itC = itC->next;
  }

  return nbClass;
}

/* cette fonction cree et renvoie un tableau a 3 dimensions contenant
 * l'ensemble des labels de methodes pour chaque classe en respectant les
 * emplacements pour les methodes heritees et redefinies 
 * respecter ces emplacements est indispensable pour que l'appel dynamique
 * des methodes fonctionne */
char*** virtualTables(ClassP classes, FILE* f)
{
  ClassP itC = classes;
  ClassP itSC;
  MethodP itM;
  int i = 0, j, nbMethods, indexSuperClass, indexLabel, indexSuperLabel,
      indexSuperMethod;
  int nbClassesWithMethod = getNbClassesWithMethod(classes);

  /* on cree un tableau a 3 dimensions qui contiendra les labels des methodes
   * a liees a chaque table virtuelle
   * ce tableau contient autant case qu'il y a de classe avec au moins une
   * methode */
  char*** labels = malloc(nbClassesWithMethod * sizeof(char**));

  /* pour chaque case, on a un nombre de labels egal au nombre de methodes de la
   * classe
   * (en comptant les methodes des super classes mais sans compter les methodes
   * redefinies plusieurs fois) */
  while(itC != NIL(Class))
  {
    /* si la classe itC a au moins une methode, on doit lui allouer une case
     * dans la tableau de labels */
    if(getNbMethods(itC) != 0)
    {
      nbMethods = getNbMethods(itC);
      labels[i] = malloc(nbMethods * sizeof(char*));
      /* on alloue enfin la memoire pour la chaine de caracteres contenant le
       * label */
      for(j = 0; j < nbMethods; j++)
	labels[i][j] = malloc(128);

      i++;
    }
    itC = itC->next;
  }

  /* on doit maintenant ajouter les labels dans le tableau
   * - si une classe n'a pas de super classe, on ajoute simplement ses methodes
   *   dans le tableau dans l'ordre dans lequel elles sont stockees
   * - si une classe a une super classe, on doit ajouter dans l'ordre :
   *	- les methodes que cette classe redefinies au meme emplacement que dans
   *	  la liste de labels de sa super classe
   *	- les methodes non redefinies qui sont dans la liste de sa super
   *	  classe au meme emplacement que dans la super classe
   *	- les methodes que cette classe definies
   */
  itC = classes;
  i = 0;/* parcour des cases du tableau correspondants aux classes */

  while(itC != NIL(Class))
  {
    /* on doit toujours sauter les classes sans methodes */
    if(getNbMethods(itC) != 0)
    {
      j = 0;/* parcour des cases du tableau correspondants aux labels */
      /* cas sans super classe */
      if(itC->superClass == NIL(Class))
      {
	itM = itC->methods;
	while(itM != NIL(Method))
	{
	  sprintf(labels[i][j], "%s", itM->label);
	  j++;
	  itM = itM->next;
	}
      }
      /* cas avec super classe */
      else
      {
	/* on doit commencer par trouver l'indice de la super classe dans le
	 * tableau */
	indexSuperClass = 0;
	itSC = classes;
	while(itSC != NIL(Class))
	{
	  if(!strcmp(itSC->name, itC->superClass->name))
	    break;
	  /* on n'incremente l'index que si itSC possede des methodes */
	  if(getNbMethods(itSC) != 0)
	    indexSuperClass++;
	  itSC = itSC->next;
	}
	/* on ajoute au tableau de label les methodes que la classe redefinie
	 * au meme emplacement que pour la super classe */
	itM = itC->methods;
	while(itM != NIL(Method))
	{
	  if(itM->over == TRUE)
	  {
	    /* on cherche l'emplacement de la methode redefinie dans le tableau
	     * de la super classe */
	    indexSuperMethod = 0;
	    char* labelSM = malloc(128);
	    sprintf(labelSM, "%s_%s", itC->superClass->name, itM->name);
	    while(strcmp(labels[indexSuperClass][indexSuperMethod], labelSM))
	      indexSuperMethod++;

	    free(labelSM);
	    /* puis on ajoute le label au bon emplacement */
	    strcpy(labels[i][indexSuperMethod], itM->label);
	  }

	  itM = itM->next;
	}

	/* puis on ajoute les labels qui sont dans la liste de labels de la
	 * super classe et qui ne correspondent pas a des methodes que la
	 * classe redefinies (en conservant l'emplacement) */
	bool add = TRUE;

	for(indexSuperLabel = 0; indexSuperLabel<getNbMethods(itC->superClass);
	    indexSuperLabel++)
	{
	  add = TRUE;
	  indexLabel = 0;
	  /* on compare les parties droites du super label et des labels
	   * qui existent deja dans le tableau */
	  char* temp = malloc(128);
	  strcpy(temp, labels[indexSuperClass][indexSuperLabel]);
	  char* tokenSuperLabel = strtok(temp, "_");
	  /* on appelle strtok 2 fois pour avoir la partie a droite de _ */
	  tokenSuperLabel = strtok(NULL, "_");
	  
	  while(indexLabel < getNbMethods(itC))
	  {
	    if(labels[i][indexLabel][0] != '\0')
	    {
	      char* temp2 = malloc(128);
	      strcpy(temp2, labels[i][indexLabel]);
	      char* tokenLabel = strtok(temp2, "_");
	      tokenLabel = strtok(NULL, "_");
	      
	      /* si tokenLabel == tokenSuperLabel alors on ne doit pas ajouter
	       * ce label */
	      if(!strcmp(tokenLabel, tokenSuperLabel))
		add = FALSE;

	      free(temp2);
	    }
	    indexLabel++;
	  }
	  
	  /* si add est encore a true, on ajoute le label au meme emplacement
	   * que dans la liste de labels de la super classe */
	  if(add)
	    strcpy(labels[i][indexSuperLabel], 
		labels[indexSuperClass][indexSuperLabel]);

	  free(temp);
	}

	/* puis on ajoute les methodes que cette classe definie dans les
	 * emplacements encore disponibles */
	itM = itC->methods;
	while(itM != NIL(Method))
	{
	  /* on saute les redefinitions car elles sont deja ajoutees au
	   * tableau */
	  if(itM->over == FALSE)
	  {
	    /* tant que la case j du tableau est occupee, on passe a la suivante*/
	    while(labels[i][j][0] != '\0')
	      j++;

	    /* puis on ajoute le label */
	    strcpy(labels[i][j], itM->label);
	    j++;
	  }
	  itM = itM->next;
	}
      }
      i++;
    }
    itC = itC->next;
  }

  return labels;
}

/* cette fonction genere le code des tables virtuelles de chaque classe
 * a partir du tableau contenant tous les labels de methodes pour chaque
 * classe*/
void generateVirtualTables(ClassP classes, char*** labels, FILE* f)
{
  ClassP itC = classes;
  int i, indexVT = 0, nbMethods;

  fprintf(f, "init:\tSTART\n");

  /* On parcours toutes les classes pour allouer les tables virtuelles*/
  while(itC != NIL(Class))
  {
    /* Pour chaque classe, on allouer autant d'emplacement memoire qu'elle a
     * de methodes (en comptant aussi les methodes de sa super classe mais sans compter
     * plusieurs fois les methodes redefinies) puis on lie ces emplacements a l'adresse 
     * de ces methodes */
    nbMethods = getNbMethods(itC);

    /* Si une classe n'a pas de methodes, on passe a la classe suivante*/
    if(nbMethods == 0)
    {
      itC = itC->next;
      continue;
    }
    else
    {
      fprintf(f, "\n\tALLOC %d\t\t-- Table classe %s\n", nbMethods, itC->name);

      for(i = 0; i < nbMethods; i++)
      {
	/*on fait la liaison avec le label de la methode*/
	fprintf(f, "\tDUPN 1\n\tPUSHA %s\n\tSTORE %d\n",
	    labels[indexVT][i], i);
      }
    }

    itC = itC->next;
    indexVT++;
  }
}

/* cette fonction libere la memoire allouee au tableau a 3 dimensions qui
 * contient l'ensemble des labels de methodes des tables virtuelles de chaque
 * classe */
void freeTable(char*** labels, ClassP classes)
{
  int i = 0,j, nbMethods, nbClassesWithMethod = getNbClassesWithMethod(classes->next->next);
  ClassP itC = classes;
  /* on avance de 2 classes pour sauter Integer et String */
  itC = itC->next->next;

  while(itC != NIL(Class))
  {
    if(getNbMethods(itC) != 0)
    {
      nbMethods = getNbMethods(itC);
      for(j = 0; j < nbMethods; j++)
	free(labels[i][j]);

      i++;
    }
    itC = itC->next;
  }
  
  for(i = 0; i < nbClassesWithMethod; i++)
    free(labels[i]);

  free(labels);
}

/* cette fonction genere les etiquette callN qui serviront a appeler
 * dynamiquement les methodes */
void generateCalls(FILE* f, ClassP classes)
{
  /* on genere les etiquettes callX
   * on doit en generer autant que le nombre de label maximum dans une case du
   * tableau */
  int nbCalls = 0, nbMethods, i;
  ClassP itC = classes;
  while(itC != NIL(Class))
  {
    nbMethods = getNbMethods(itC);
    if(nbMethods > nbCalls)
      nbCalls = nbMethods;

    itC = itC->next;
  }

  for(i = 0; i < nbCalls; i++)
  {
    fprintf(f, "\n--invoque la methode %d du receveur\n", i + 1);
    fprintf(f, 
	"call%d:\tPUSHL -1\n\tLOAD 0\n\tLOAD %d\n\tCALL\n\tRETURN\n",
	i + 1, i);
  }
}

/* Génère le code pour l'appel d'une méthode */
void callMethod(TreeP caller, TreeP called, TreeP args, char*** labels, 
    ClassP classes, FILE* f)
{
  /* Il faudra distinguer trois cas: fonction de Integer, de String et autre.
   * ça se récupèrera sur caller, qui peut être une constante (string ou
   * integer) ou un idvar (dans ce cas, chercher le type de la variable).
   */

  /* etapes :
   * - trouver le type de l'appelant
   * - charger les parametres
   * - charger le bon objet pour avoir acces a la bonne TV
   * - faire la resolution methode appelee/callX 
   * - depiler les arguments et l'objet appelant */ 
  int nbNewArgs, i, j, exit = 1, nbMethods, indexLabel;
  ClassP itC = classes;
  itC = itC->next->next;

  if(!strcmp(called->u.meth->label, "Integer_toString"))
  {
    instructionCode(caller, 0, classes, labels, f);
    fprintf(f, "\tSTR\n");
  }
  else if(!strcmp(called->u.meth->label, "String_print"))
  {
    instructionCode(caller, 0, classes, labels, f);
    fprintf(f, "\tWRITES\n");
  }
  else if(!strcmp(called->u.meth->label, "String_println"))
  {
    instructionCode(caller, 0, classes, labels, f);
    fprintf(f, "\tPUSHS \"\\n\"\n\tCONCAT\n\tWRITES\n");
  }
  else
  {
    if(called->u.meth->type != NIL(char))
      fprintf(f, "\tPUSHI 0\n");

    nbNewArgs = instructionCode(args, 0, classes, labels, f);
    instructionCode(caller, nbNewArgs, classes, labels, f);
    
    /* pour faire la resolution methode appelee/callX on cherche simplement
     * dans le tableau de labels l'index du label de la methode appelee
     * puis index + 1 donne le numero de l'etiquette callX a utilisee
     * cela fonctionne car dans la liste de labels d'une sous classe
     * les labels des methodes qui sont presents dans la liste des labels
     * de la super classe sont au meme index */
    i = 0;
    while(itC != NIL(Class) && exit)
    {
      if(getNbMethods(itC) != 0)
      {
	nbMethods = getNbMethods(itC);
	for(j = 0; j < nbMethods; j++)
	{
	  if(!strcmp(labels[i][j], called->u.meth->label))
	  {
	    indexLabel = j;
	    exit = 0;
	    break;
	  }
	}
	i++;
      }
      itC = itC->next;
    }
    fprintf(f, "\tPUSHA call%d\n\tCALL\n", indexLabel + 1);
    /* on pop les arguments et l'objet appellant */
    fprintf(f, "\tPOPN %d\n", nbNewArgs + 1);
  }
}

/* cette fonction libere la memoire allouee au label des methodes */
void freeLabels(ClassP classes)
{
  ClassP itC = classes;
  MethodP itM;
  while(itC != NIL(Class))
  {
    itM = itC->methods;
    while(itM != NIL(Method))
    {
      free(itM->label);
      itM = itM->next;
    }
    itC = itC->next;
  }
}
