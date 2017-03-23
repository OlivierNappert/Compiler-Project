#include <unistd.h>
#include <stdio.h>
#include "tp.h"
#include "tp_y.h"

/* Forward déclaration des fonctions d'affichage */
void printFile(ClassP classes, TreeP body);
void printClass(ClassP class, int tabsCount);
void printTree(TreeP body, int tabsCount);
void printMethod(MethodP method, int tabsCount);
void printVarDecl(VarDeclP varDecl, int tabsCount);
void printEtiquette(short op);


/* Fonction principale */
void printFile(ClassP classes, TreeP body)
{
  /* Affichage des classes */
  printClass(classes, 0);
  
  /* Affichage du body */
  printf("Main\n");
  printTree(body, 1);
  
  /* Fin du fichier si tout s'est déroulé correctement */
  printf("EOF\n");
}


/* Affiche la structure d'une classe, de ses variables et méthodes */
void printClass(ClassP class,  int tabsCount)
{
  Class* temp = class;
  int i;
  for(i = 0; i < tabsCount; i++)
      printf("|   ");
  if(temp != NULL)
  {
    /* Affichage du nom de la classe */
    printf("class %s\n", temp->name);
    
    /* Affichage des parametres du constructeur de la classe */
    if(temp->constructor != NULL)
      printMethod(temp->constructor, tabsCount);
    
    /* Affichage de la classe Mere */
    if(temp->super != NULL)
    {
    for(i = 0; i < tabsCount + 1; i++)
        printf("|   ");
      printf("Extends\n");
			printTree(temp->super, tabsCount + 2);
    }
    
    /* Affichage du constructeur */
    if(temp->constructor->bloc != NULL)
    {
      for(i = 0; i < tabsCount + 1; i++)
        printf("|   ");
      printf("Constructor\n");
      printTree(temp->constructor->bloc, tabsCount + 2);
    }
    
    /* Affichage du body */
    for(i = 0; i < tabsCount + 1; i++)
        printf("|   ");
    printf("Body\n");
    
    /* Affichage des declaration de variables du body */
    if(temp->vars != NULL)
      printVarDecl(temp->vars, tabsCount + 2);
    
    /* Affichage des declaration de méthodes du body */
    if(temp->methods != NULL)
      printMethod(temp->methods, tabsCount + 2);
    
    /* Affichage de la classe suivante */
    printf("\n");
    printClass(temp->next, 0);
    
    
  }
}


/* Affiche toute la structure d'un AST */
void printTree(TreeP body, int tabsCount)
{
  int i;
  if(body == NULL || (body->op != E_LVAR && body->op != E_INST))
  {
    for(i = 0; i < tabsCount; i++)
      printf("|   ");
  }

  /* Si le body n'est pas nul */
  if(body != NULL)
  {
    /* Si l'on se situe sur une feuille */
    if(body->nbChildren == 0)
    {
      switch(body->op)
      {
        case E_IDVAR:
          printf("Var[%s]\n", body->u.str);
          break;
        case E_STR:
          printf("String[\"%s\"]\n", body->u.str);
          break; 
        case E_CONST:
          printf("Const[%d]\n", body->u.val);
          break;
        case E_LVAR:
		      printVarDecl(body->u.lvar, tabsCount);
		      break;
        case E_IDMETH:
          printf("AppMeth[%s]\n", body->u.str);
          break;
        case E_THIS:
        case E_RESULT:
        case E_SUPER:
        case E_PREDEF:
          printEtiquette(body->op);
          printf("\n");
          break;
        default:
          printf("Feuille inconnue\n");
          break;
      }
    }
    /* S'il sagit d'une instruction */
    else if(body->op == E_INST)
    {
      printTree(body->u.children[0], tabsCount);
      printTree(body->u.children[1], tabsCount);
    }
    /* Sinon */
    else
    {
      printEtiquette(body->op);
      printf(" :\n");
      for(i = 0; i < body->nbChildren; i++)
        printTree(body->u.children[i], tabsCount + 1);
    }
  }
  else
  {
	  printf("NULL\n");
  }
}


/* Affiche la structure d'une méthode et de son bloc */
void printMethod(MethodP method, int tabsCount)
{
  int i;
  Method* temp;
  temp = method;
  
  for(i = 0; i < tabsCount; i++)
    printf("|   ");

  if(temp != NULL)
  {
    /* Si l'on veut afficher une méthode */
    if(temp->name != NULL)
    {
      /* Affichage de son nom, de l'héritage et de son type de retour */
      printf("%sMethode %s : %s \n", (temp->over ? "Override " : ""), temp->name, (temp->type == NULL ? "void" : temp->type));
      if(temp->args != NULL)
      {
        /* Affichage des parametres */
        for(i = 0; i < tabsCount + 1; i++)
          printf("|   ");
        printf("Liste parametres\n");
        printVarDecl(temp->args, tabsCount + 2);
      }
      /* Affichage de son bloc s'il existe */
      if(temp->bloc != NULL)
      {
        for(i = 0; i < tabsCount + 1; i++)
          printf("|   ");
        printf("Body\n");
        printTree(temp->bloc, tabsCount + 2);
      }
      /* Affichage de la méthode suivante */
      printMethod(temp->next, tabsCount);
    }
    /* Si l'on veut uniquement afficher un constructeur soit un Nom vide */
    else
    {
      if(temp->args != NULL)
      {
        printVarDecl(temp->args, tabsCount + 1);
      }
    }
  }
}


/* Affiche une déclaration de variable ainsi que son affectation */
void printVarDecl(VarDeclP varDecl, int tabsCount)
{	
	if(varDecl != NULL)
	{
    int i;
    for(i = 0; i < tabsCount; i++)
      printf("|   ");
		printf("VarDecl: %s : %s\n", varDecl->name, varDecl->type);
		if(varDecl->aff != NULL)
			printTree(varDecl->aff, tabsCount+1);
		if(varDecl->next != NULL)
			printVarDecl(varDecl->next, tabsCount);
	}
}


/* Affiche la valeur d'une étiquette sur la sortie standard */
void printEtiquette(short op)
{
  switch(op){
    case 1:
      printf("E_NE");
      break;
    case 2:
      printf("E_EQ");
      break;
    case 3:
      printf("E_LT");
      break;
    case 4:
      printf("E_LE");
      break;
    case 5:
      printf("E_GT");
      break;
    case 6:
      printf("E_GE");
      break;
    case 7:
      printf("E_IDVAR");
      break;
    case 8:
      printf("E_CONST");
      break;
    case 9:
      printf("E_STR");
      break;
    case 10:
      printf("E_ADD");
      break;
    case 11:
      printf("E_MINUS");
      break;
    case 12:
      printf("E_MULT");
      break;
    case 13:
      printf("E_DIV");
      break;
    case 14:
      printf("E_CONC");
      break;
    case 15:
      printf("E_ITE");
      break;
    case 16:
      printf("E_RETU");
      break;
    case 17:
      printf("E_RESULT");
      break;
    case 18:
      printf("E_THIS");
      break;
    case 19:
      printf("E_SUPER");
      break;
    case 20:
      printf("E_LIST");
      break;
    case 21:
      printf("E_CLASS");
      break;
    case 22:
      printf("E_AFF");
      break;
    case 23:
      printf("E_NEW");
      break;
    case 24:
      printf("E_SM");
      break;
    case 25:
      printf("E_METH");
      break;
    case 26:
      printf("E_IS");
      break;
    case 27:
      printf("E_SEL");
      break;
    case 28:
      break;
    case 29:
      printf("E_BLOC");
      break;
    case 30:
      printf("E_LVAR");
      break;
    case 31:
      printf("ID_METH");
      break;
    case 32:
      printf("E_PREDEF");
      break;
    case 33:
      printf("E_CAST");
      break;
  }
}
