#include <string.h>
#include <stdio.h>
#include "tp.h"
#include "tp_y.h"

extern char *strdup(const char*);
extern void setError(int code);

/* Fonctions de verifications contextuelles au niveau de la portée*/
void checkMethod(MethodP method, ClassP class, ClassP classes, ClassP voidClass);
void checkBloc(TreeP bloc, VarDeclP varsClass, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass, 
    ClassP classes,int level);
VarDeclP checkDeclared(char* name, VarDeclP varsClass, VarDeclP varsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
bool checkUnicity(VarDeclP var, VarDeclP varsClass, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);

/* Fonctions de verifications contextuelles au niveau du typage */
bool checkTypeHeritage(ClassP current, ClassP toCheck);
bool checkSelection(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
bool checkITE(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
ClassP checkMessage(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
ClassP checkOperator(TreeP tree, ClassP class, ClassP classes, ClassP curentClass, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
ClassP checkNew(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
ClassP checkAff(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
bool checkChildren(int nb, TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
ClassP checkVarsMasked(char * name, ClassP class, VarDeclP argsMethod, 
          VarDeclP varsBloc, ClassP voidClass);
ClassP checkSpecial(char * name, VarDeclP specials);
ClassP getType(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
void strToLvar(TreeP tree,VarDeclP varsClass,VarDeclP argsMethod,VarDeclP special,VarDeclP varsBloc,ClassP voidClass);


/* Fonctions générales utiles pour les vérifications contextuelles */
ClassP getClass(char* name, ClassP classes);
bool checkHeritage(TreeP super, ClassP classes);
VarDeclP getVarsBloc(TreeP tree);
bool matchingParameters(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP expectedParam, VarDeclP special, VarDeclP varsBloc, ClassP voidClass);
void printProblem(char * problem);

/* Fonction qui permet de vérifier qu'une classe est valide */
void checkClass(ClassP class, ClassP classes, int id, ClassP voidClass)
{
  /* Vérification de la non existence du nom de la classe
   * et de l'existence de la super classe */
  if(class != NIL(Class))
  {
    ClassP itC = NIL(Class);
    MethodP itM = NIL(Method);
    VarDeclP itV = NIL(VarDecl);
    bool hasSuper = (class->super != NIL(Tree)
        && getChild(class->super, 0)->u.str != NULL
        ? TRUE : FALSE);
    ClassP super = NIL(Class);

  for(itC = classes; itC != class; itC = itC->next)
  {
    if(!strcmp(itC->name, class->name))
    {
      fprintf(stderr, "Error : Multiple declaration of class named %s\n",
        class->name);
      setError(CONTEXT_ERROR);
    }

    if(hasSuper && !strcmp(itC->name, getChild(class->super, 0)->u.str))
        super = itC;
  }

  class->level = 0;
  
  if(hasSuper)
  {
    if(!checkHeritage(class->super, classes))
    {
      fprintf(stderr, "Error : Forbidden inheritance for %s (from Integer or String)\n",
      class->name);
      setError(CONTEXT_ERROR);
    }
    /* Lien avec la super classe et ajout des variables de la 
    * superclass dans les variables de class */
    else if(super) 
    {
      VarDeclP nvar = NIL(VarDecl);
      VarDeclP oldvar = NIL(VarDecl);
      VarDeclP first = NIL(VarDecl);

      class->superClass = super;
      class->level = super->level + 1;
      /* Changement du niveau des variables de la classe courante (par 
      * défaut tous à 0) pour correspondre à son niveau */
      for(itV = class->vars; itV != NIL(VarDecl); itV = itV->next)
      {
        itV->level = class->level;
        itV->isInClass = TRUE;
      }
      /* Copie et ajout des variables de la classe mère à la pile */
      for(itV = super->vars; itV != NIL(VarDecl);
          itV = itV->next)
      {
        /* Copie d'une VarDecl de la classe mère */
        nvar = NEW(1, VarDecl);
        nvar->name = itV->name;
        nvar->type = itV->type;
        nvar->aff = itV->aff;
        nvar->level = itV->level;
        nvar->address = itV->address;
        nvar->typeClass = itV->typeClass;
        nvar->isInClass = TRUE;

        if(oldvar == NIL(VarDecl)) /* Première variable rencontrée */
          first = nvar;
        else
          oldvar->next = nvar; /* Lien avec la variable précédente */
        oldvar = nvar;
      }
      if(first != NIL(VarDecl)) /* Lien avec la classe courrante */
      {
        nvar->next = class->vars;
        class->vars = first;
      }
    }

    else /* Erreur si la super classe n'existe pas */
    {
      fprintf(stderr, "Error : No %s class name found\n",
      getChild(class->super, 0)->u.str);
      setError(CONTEXT_ERROR);
    }
  }

    /* Vérification de l'existence des types de variables.
     * On ignore les variables de classes mère déjà vérifiées */
    for(itV = class->vars;
      itV != NIL(VarDecl) && itV->level != class->level;
      itV = itV->next);

    /* Adresse courante dans la pile pour la génération */
    int address = (class->superClass != NIL(Class) ?
    class->superClass->size : 1);
    for(; itV != NIL(VarDecl); itV = itV->next)
    {
      itV->typeClass = getClass(itV->type, classes);
      if(itV->typeClass == NIL(Class))
      {
        fprintf(stderr, "Error : Type %s undeclared for class variable %s\n",
            itV->type, itV->name);
        setError(CONTEXT_ERROR);
        break;
      }
      itV->address = address;
      address ++;
    }
    
    VarDeclP temp = NIL(VarDecl);
    VarDeclP special;
   /* Ajout du this */
    special = declVar("this", NULL, NIL(Tree));
    special->typeClass = class;
      
    /* Ajout du super */
    if(class->superClass != NIL(Class))
    {
      special = addVarDecl(special, declVar("super", NULL, NIL(Tree)));
      special->typeClass = class->superClass;
    }

    for(itV = class->vars; itV != NIL(VarDecl) && itV->level != class->level; itV = itV->next)
    {
      itV->isInClass = TRUE;

      if(itV != class->vars)
      {
        for(temp = class->vars; temp->next != itV; temp = temp->next);
        temp->next = NIL(VarDecl);
      }

      if(itV->aff != NIL(Tree))
      {
        VarDeclP previous;

        itV == class->vars ? (previous = NIL(VarDecl)) : (previous = class->vars);
        strToLvar(itV->aff,NIL(VarDecl),NIL(VarDecl),special,previous,voidClass);
        if(!checkTypeHeritage(getType(itV->aff,classes,class,NIL(VarDecl),special,previous,voidClass),itV->typeClass))
        {
          fprintf(stderr, "Error : Problem with the declaration of Class variable %s in class %s\n",
            itV->name, class->name);
          setError(CONTEXT_ERROR);
        }
      }
      
      if(itV != class->vars)
      {
        for(temp = class->vars; temp->next != NIL(VarDecl); temp = temp->next);
        temp->next = itV;
      }

      /* On check que la variable de classe ne soit pas declaree plusieurs fois */
      if(!checkUnicity(itV,class->vars,NIL(VarDecl),NIL(VarDecl),NIL(VarDecl),voidClass))
      {
        fprintf(stderr, "Error : Class variable %s declared multiple times in class %s\n",
            itV->name, class->name);
        setError(CONTEXT_ERROR);
      }
    }

    /* On check qu'aucune variable de classe ne s'appelle this, super ni result */
    if(checkDeclared("this",class->vars,NIL(VarDecl),NIL(VarDecl),NIL(VarDecl),NIL(Class)))
    {
      fprintf(stderr, "Error : Variable with name 'this' can't be used as a class variable in class %s\n",
        class->name);
      setError(CONTEXT_ERROR);
    }

    if(checkDeclared("super",class->vars,NIL(VarDecl),NIL(VarDecl),NIL(VarDecl),NIL(Class)))
    {
      fprintf(stderr, "Error : Variable with name 'super' can't be used as a class variable in class %s\n",
        class->name);
      setError(CONTEXT_ERROR);
    }

    if(checkDeclared("result",class->vars,NIL(VarDecl),NIL(VarDecl),NIL(VarDecl),NIL(Class)))
    {
      fprintf(stderr, "Error : Variable with name 'result' can't be used as a class variable in class %s\n",
        class->name);
      setError(CONTEXT_ERROR);
    }

    for(itV = class->vars; itV != NIL(VarDecl); itV = itV->next)
      itV->isInClass = TRUE;
    class->size = address;  

    /* Verification du constructeur */
    checkMethod(class->constructor, class, classes, voidClass);
    /* Verification de l'appel au constructeur de la superclass si elle existe */
    if(class->superClass != NIL(Class))
    {
      strToLvar(class->super,NIL(VarDecl),class->constructor->args,NIL(VarDecl),NIL(VarDecl),voidClass);
      if(!matchingParameters(class->super,classes,class, class->constructor->args,
        class->superClass->constructor->args,special,NIL(VarDecl),voidClass))
      {
        fprintf(stderr, "Error : Parameters for Class %s constructor are incorrect\n",
        class->superClass->name);
        setError(CONTEXT_ERROR);
      }
    }

    /* Verification des methodes de la classe */
    for(itM = class->methods; itM != NIL(Method); itM = itM->next)
    {
      itM->level = class->level;
      checkMethod(itM, class, classes, voidClass);
    }
    
    class->id = id;
    
    /* Verification de la classe suivante */
    checkClass(class->next, classes, id + 1, voidClass);
  }
}

/* Fonction qui permet de vérifier qu'une methode est valide */
void checkMethod(MethodP method, ClassP class, ClassP classes, ClassP voidClass)
{
  int address = -2;
  VarDeclP itV[3]; /* Itérateurs de déclaration de variable */
  MethodP itM; /* Itérateur de méthodes */
  ClassP it;

  /* Vérification de l'existence du type de retour */
  if(method->type != NULL && !getClass(method->type, classes))
  {
    fprintf(stderr, "Error : Type %s undeclared for method %s\n",
        method->type, method->name);
    setError(CONTEXT_ERROR);
  }

  /* Vérification de l'existence des types de variables */
  for(itV[0] = method->args; itV[0] != NIL(VarDecl); itV[0] = itV[0]->next)
  {
    itV[0]->typeClass = getClass(itV[0]->type, classes);
    itV[0]->level = class->level + 1;
    itV[0]->isInClass = FALSE;
    itV[0]->address = address;
    if(itV[0]->typeClass == NIL(Class))
    {
      fprintf(stderr, "Error : Type %s undeclared for argument %s\n",
          itV[0]->type, itV[0]->name);
      setError(CONTEXT_ERROR);
    }

    /* Verification de l'unicite des parametres declares */
    if(!checkUnicity(itV[0],NIL(VarDecl),method->args,NIL(VarDecl),NIL(VarDecl),voidClass))
    {
      fprintf(stderr, "Error : Method parameter %s declared multiple times in %s\n",
        itV[0]->name, method->name == NULL ? "constructor" : method->name);
      setError(CONTEXT_ERROR);
    }

    /* On check qu'aucun parametre ne s'appelle this, super ni result */
    if(checkDeclared("this",NIL(VarDecl),method->args,NIL(VarDecl),NIL(VarDecl), NIL(Class)))
    {
      fprintf(stderr, "Error : Variable with name 'this' can't be used as a parameter for method %s\n",
        method->name == NULL ? "constructor" : method->name);
      setError(CONTEXT_ERROR);
    }

    if(checkDeclared("super",NIL(VarDecl),method->args,NIL(VarDecl),NIL(VarDecl),NIL(Class)))
    {
      fprintf(stderr, "Error : Variable with name 'super' can't be used as a parameter for method %s\n",
        method->name == NULL ? "constructor" : method->name);
      setError(CONTEXT_ERROR);
    }

    if(checkDeclared("result",NIL(VarDecl),method->args,NIL(VarDecl),NIL(VarDecl),NIL(Class)))
    {
      fprintf(stderr, "Error : Variable with name 'result' can't be used as a parameter for method %s\n",
        method->name == NULL ? "constructor" : method->name);
      setError(CONTEXT_ERROR);
    }

    address--;
  }

  /* Verification de l'existence de la méthode redéfinie s'il y a le mot
  clé Override */
  bool exists;
  bool sameType;
  bool sameArgTypes;
  if(method->over)
  {
    if(class->superClass == NIL(Class)) /* S'il n'y a aucune classe mère */
    {
      fprintf(stderr, "Error : [Override] Method with name %s"
          "has no superclass\n", method->name);
      setError(CONTEXT_ERROR);
    }
    else
    {
      exists = FALSE;
      sameType = FALSE;
      sameArgTypes = FALSE;
      
      /* On cherche une méthode avec une signature (nom, type de retour et types
       d'arguments) identique dans l'une des classes mère */
      for(it = class->superClass; it != NIL(Class); it = it->superClass)
      {
        for(itM = it->methods; itM != NIL(Method); itM = itM->next)
        {
          if(itM != method && !strcmp(itM->name, method->name))
          {
            exists = TRUE; /* Nom identique */
            if(!((itM->type == NULL && method->type != NULL)
              || (itM->type != NULL && method->type == NULL)))
            {
             if((itM->type == NULL && method->type == NULL)
                || !strcmp(itM->type, method->type))
              {
                sameType = TRUE; /* Type de retour identique */
                itV[0] = method->args;
                itV[1] = itM->args;
                if(!((itV[0] == NIL(VarDecl) && itV[1] != NIL(VarDecl))
                  || (itV[0] != NIL(VarDecl) && itV[1] == NIL(VarDecl))))
                {
                  for(sameArgTypes = TRUE; itV[0] != NIL(VarDecl) && itV[1] != NIL(VarDecl);
                    itV[0] = itV[0]->next, itV[1] = itV[1]->next)
                  {
                    if(strcmp(itV[0]->type, itV[1]->type))
                      sameArgTypes = FALSE; /* Un des types d'argument est dfférent */
                    
                    if((itV[0]->next == NIL(VarDecl) && itV[1]->next != NIL(VarDecl))
                      || (itV[0]->next != NIL(VarDecl) && itV[1]->next == NIL(VarDecl)))
                      sameArgTypes = FALSE;
                  }
                }
              } 
            } 
          }
          if(exists && sameType && sameArgTypes)
           break;
        }
        if(exists && sameType && sameArgTypes)
          break;
      } 

      if(!exists) /* Si la fonction n'existe pas dans la classe mère */
      {
        fprintf(stderr, "Error : [Override] No method with name %s "
            "in SuperClass\n", method->name);
        setError(CONTEXT_ERROR);
      }

      else if(!sameType) /* Si elle n'a pas le même type de retour */
      {
        fprintf(stderr, "Error : [Override] Different return types for "
            "method %s and its SuperClass version\n",
            method->name);
        setError(CONTEXT_ERROR);
      }

      else if(!sameArgTypes) /* Si elle n'a pas les mêmes arguments */
      {
        fprintf(stderr, "Error : [Override] Method %s and its SuperClass "
            "version have different arguments.\n", method->name);
        setError(CONTEXT_ERROR);
      }
    }
  }

  /* On verifie qu'il n'y a pas de surcharge de cette methode dans la classe */
  for(itM = class->methods; itM != NIL(Method); itM = itM->next)
  {
    if(itM != method && itM->name != NULL && method->name != NULL && !strcmp(itM->name, method->name))
    {
      fprintf(stderr, "Error : Method %s declared multiple time in class %s but overload is forbidden\n", method->name, class->name);
      setError(CONTEXT_ERROR);
      break; 
    }
  }

  /* On verifie que la methode n'existe pas dans une classe mere avec le meme nom si on ne met pas override */
  if(method->over == FALSE && method->name != NULL)
  {
    for(it = class->superClass; it != NIL(Class); it = it->superClass)
    {
      for(itM = it->methods; itM != NIL(Method); itM = itM->next)
      {
        if(itM != method && itM->name != NULL && !strcmp(itM->name,method->name))
        {
          fprintf(stderr, "Error : Method %s has the same name as another method in SuperClass %s without override\n", method->name, it->name);
          setError(CONTEXT_ERROR);
          break;
        }
      }
    }
  }  
   
  /* Ajout du this */
  itV[2] = declVar("this", NULL, NIL(Tree));
  itV[2]->typeClass = class;
  
  /* Ajout du return */
  if(method->type != NULL)
  {
    itV[2] = addVarDecl(itV[2], declVar("result", method->type, NIL(Tree)));
    itV[2]->typeClass = getClass(method->type, classes);
  }
  /* Ajout du super */
  if(class->superClass != NIL(Class))
  {
    itV[2] = addVarDecl(itV[2], declVar("super", NULL, NIL(Tree)));
    itV[2]->typeClass = class->superClass;
  }


  /* Vérification du code de la méthode */
  checkBloc(method->bloc, class->vars, method->args, itV[2],
    NIL(VarDecl), voidClass, classes, class->level);

  if(method->bloc != NIL(Tree) && (method->bloc->nbChildren > 0))
  {
    if(method->bloc->op == E_IS && getType(method->bloc, classes, 
    class,method->args, itV[2], NIL(VarDecl), 
    voidClass) != voidClass)
    {
      fprintf(stderr, "Error in : Body of method %s in Class %s\n", 
            method->name, class->name);
      setError(CONTEXT_ERROR);
    }

    else if(method->bloc->op != E_IS 
    && !checkTypeHeritage(getType(method->bloc,classes,class,method->args,
    itV[2],NIL(VarDecl), voidClass),getClass(method->type, classes)))
    {
      fprintf(stderr, "Error in : Body of method %s in Class %s\n", 
              method->name, class->name);
      setError(CONTEXT_ERROR);
    }
  }
}

/* Fonction qui permet de vérifier qu'un bloc est valide */
void checkBloc(TreeP bloc, VarDeclP varsClass, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass, 
    ClassP classes, int level)
{
  int i = 0;
  int address = 0;
  if(bloc != NIL(Tree))
  {
    if(bloc->op == E_LVAR)
    {
      VarDeclP it;

      for(it = bloc->u.lvar; it != NIL(VarDecl); it = it->next)
      {        
        it->typeClass = getClass(it->type, classes);
        if(it->level == 0)
          it->level = level + 1;
        it->isInClass = FALSE;
        it->address = address;
        address++;

        if(!checkUnicity(it,NIL(VarDecl),argsMethod,special,varsBloc, voidClass))
        {
          fprintf(stderr, "Error : Variable %s is declared multiple times in bloc.\n",
              it->name);
          setError(CONTEXT_ERROR);
        }

        if(checkDeclared("this",NIL(VarDecl),NIL(VarDecl),NIL(VarDecl),varsBloc, NIL(Class)) != NIL(VarDecl))
        {
          fprintf(stderr, "Error : Variable with name 'this' can't be used as a local variable\n");
          setError(CONTEXT_ERROR);
        }

        if(checkDeclared("super",NIL(VarDecl),NIL(VarDecl),NIL(VarDecl),varsBloc, NIL(Class)) != NIL(VarDecl))
        {
          fprintf(stderr, "Error : Variable with name 'super' can't be used as a local variable\n");
          setError(CONTEXT_ERROR);
        }

        if(checkDeclared("result",NIL(VarDecl),NIL(VarDecl),NIL(VarDecl),varsBloc, NIL(Class)) != NIL(VarDecl))
        {
          fprintf(stderr, "Error : Variable with name 'result' can't be used as a local variable\n");
          setError(CONTEXT_ERROR);
        }

        if(it->aff != NIL(Tree) && it->level > level)
          checkBloc(it->aff, varsClass, argsMethod, special, varsBloc, voidClass, classes, level);
      }
    }

    else if(bloc->op == E_IDVAR || bloc->op == E_THIS 
          || bloc->op == E_RESULT || bloc->op == SUPER )
    {
      strToLvar(bloc,varsClass,argsMethod,special,varsBloc,voidClass);
    }     
    /* Si on est a la racine d'un bloc, on appelle la fonction recursivement */
    else if(bloc->op == E_IS)
    {
      VarDeclP it = NIL(VarDecl);
      int nextLevel;
      if(varsBloc != NIL(VarDecl))
        nextLevel = varsBloc->level;
      else
        nextLevel = level;
  
      if(getChild(bloc, 0) != NIL(Tree) && bloc->nbChildren == 2)
      {
        /* On ajoute les variables du sous bloc */
        for(it = getVarsBloc(bloc); it != NIL(VarDecl) && it->next != NIL(VarDecl); it = it->next);

        if(varsBloc != NIL(VarDecl) && it != NIL(VarDecl))
          it->next = varsBloc;
        
        int i;

        for(i = 0; i < bloc->nbChildren; i++)
          checkBloc(getChild(bloc, i), varsClass, argsMethod, special, getVarsBloc(bloc), voidClass, classes, nextLevel);

        if(varsBloc != NIL(VarDecl) && it != NIL(VarDecl))
          it->next = NIL(VarDecl);
      }
      else
        checkBloc(getChild(bloc, 0), varsClass, argsMethod, special, varsBloc, voidClass, classes, nextLevel);
    }

    else
    {
      for(i = 0; i < bloc->nbChildren; i++)
        checkBloc(getChild(bloc, i), varsClass, argsMethod, special, varsBloc, voidClass, classes, level);
    }
  }
}

/* Fonction qui verifie le bloc principal du programme */
void checkMain(TreeP main, ClassP classes,ClassP voidClass)
{
  if(main != NIL(Tree))
  {
    checkBloc(main, NIL(VarDecl), NIL(VarDecl),NIL(VarDecl),NIL(VarDecl),voidClass, classes, 0);
    if(getType(main, classes, voidClass, NIL(VarDecl), NIL(VarDecl), NIL(VarDecl), voidClass) != voidClass)
      printProblem("Error in : Main\n");
  }
}

/* Fonction qui renvoie TRUE si la variable dont le nom est name a deja ete declaree auparavant, FALSE sinon */
VarDeclP checkDeclared(char* name, VarDeclP varsClass, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  
  VarDeclP it;
  /* On verifie si la variable a ete declaree dans les variables de bloc,
  de classe, les arguments de la methode et les variables this / super / result */
  for(it = varsBloc; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(name,it->name))
      return it;
  }
  
  for(it = argsMethod; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(name,it->name))
      return it;
  }
  
  for(it = varsClass; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(name,it->name))
      return it;
  }

  for(it = special; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(name,it->name))
      return it;
  }
  return NIL(VarDecl);
}

/* Fonction qui renvoie TRUE si la variable a ete declaree une seule fois */
bool checkUnicity(VarDeclP var, VarDeclP varsClass, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  VarDeclP it;
  /* On verifie si la variable a ete declaree dans les variables de bloc,
  de classe, les arguments de la methode et les variables this / super / result */
  for(it = varsBloc; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(var->name,it->name) && var != it && var->level == it->level)
      return FALSE;
  }
  
  for(it = varsClass; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(var->name,it->name) && var != it && var->level == it->level)
      return FALSE;
  }

  for(it = argsMethod; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(var->name,it->name) && var != it)
      return FALSE;

  }

  for(it = special; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(var->name,it->name) && var != it)
      return FALSE;
  }

  return TRUE;
}


/* Fonction qui renvoie un bool pour indiquer si la classe current est une 
 classe fille de toCheck */
bool checkTypeHeritage(ClassP current, ClassP toCheck)
{
  ClassP tempClass;
  /* On va parcourir toutes les superclasses */
  for(tempClass = current; tempClass != NIL(Class); tempClass = tempClass->superClass)
  {
    if(tempClass == toCheck)
      return TRUE;     
  }
  return FALSE;
}

/* Fonction qui verifie que la selection est correcte par rapport a l'encapsulation */
bool checkSelection(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{

  /* Verifie qu'une séléction n'est pas réalisée sur un result */
  if(tree != NIL(Tree))
  {
    if(tree->nbChildren == 2 && getChild(tree,1)->op == E_RESULT)
      return FALSE;
  }
    
  /* Trouve la classe de l'expression */
  ClassP expClass = getType(getChild(tree,1), classes, class, argsMethod, special, varsBloc, voidClass);

  /* Si la classe est nulle ou que la classe de l'expression n'est pas la classe actuelle */
  if(expClass == NIL(Class) || !checkTypeHeritage(class,expClass)) 
    return FALSE; 

  /* Sinon on parcourt les variables de la classe, et on return TRUE si on trouve la variable qui correspond a l'id */
  VarDeclP it;
  for(it = expClass->vars; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(it->name,getChild(tree,0)->u.lvar->name))
      return TRUE;
  }

  return FALSE;
}

/* Fonction qui verifie qu'un IF THEN ELSE est possible */
bool checkITE(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  if(tree != NIL(Tree))
  {
    if(tree->nbChildren == 3 && getChild(tree,0)->op == E_RESULT)
      return FALSE;
  }

  if(getType(getChild(tree,0), classes, class, argsMethod, special, varsBloc, voidClass) != getClass("Integer", classes))
    return FALSE;
  
  int i;
  TreeP child = NIL(Tree);

  for(i = 1; i < 3; i++)
  {
    child = getChild(tree,i);
    if(child->op == E_IS)
    {
      if(getChild(child,0) != NIL(Tree))
      {
        if(getType(getChild(child,0),classes, class, argsMethod, special, varsBloc, voidClass) == NIL(Class))
          return FALSE;
      }
    }
    else
    {
      if(getType(child,classes, class, argsMethod, special, varsBloc, voidClass) == NIL(Class))
        return FALSE;
    }
  }

  return TRUE;
}

/* Fonction qui verifie que le send message (appel de fonction ) soit bien forme */
ClassP checkMessage(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  if(tree != NIL(Tree))
  {
    if(tree->nbChildren == 2 && getChild(tree,0)->op == E_RESULT)
      return FALSE;
  }
  
  /* Trouve la classe de l'expression */
  ClassP expClass = getType(getChild(tree,0), classes, class, argsMethod, special, varsBloc, voidClass);
  /* Trouve le nom de la methode qu'on appelle */
  char* methName = getChild(getChild(tree,1),0)->u.str;

  MethodP it;
  ClassP itC;
  
  for(itC = expClass; itC != NIL(Class); itC = itC->superClass)
  {
    for(it = itC->methods; it != NIL(Method); it = it->next)
    {
      /* Si on trouve une methode avec le meme nom que methName dans la classe de l'expression 
      et que les parametres correspondent avec ceux attendus modulo heritage */ 
      if(!strcmp(methName, it->name) 
      && matchingParameters(getChild(tree,1), classes, class, argsMethod, it->args, special, varsBloc, voidClass))
      {
        getChild(getChild(tree,1),0)->u.meth = it;
        if(it->type != NULL)
        {
          return getClass(it->type, classes);
        }
        else
          return voidClass;
      }
    }
  }
  return NIL(Class);
}

/* Fonction qui renvoie une classe qui indique si l'opérateur est utilisé entre 
 2 classes identiques */
ClassP checkOperator(TreeP tree, ClassP class, ClassP classes, ClassP currentClass, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  if(tree != NIL(Tree))
  {
    if(tree->nbChildren == 2 && (getChild(tree,0)->op == E_RESULT || getChild(tree,1)->op == E_RESULT))
      return NIL(Class);
  }
  
  if(tree->nbChildren == 2 
  && getType(getChild(tree,0), classes, currentClass, argsMethod, special, varsBloc, voidClass) == class 
  && getType(getChild(tree, 1), classes, currentClass, argsMethod, special, varsBloc, voidClass) == class)
    return class;
  else
    return NIL(Class);
}

/* Fonction qui verifie que la classe instanciee par New est bonne et que ses arguments sont du bon type */
ClassP checkNew(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  ClassP newClassInst = getClass(getChild(tree,0)->u.str, classes);
  if(newClassInst == NIL(Class))
    printProblem("Error : Can't call New : Class not found");

  else if(newClassInst == getClass("Integer",classes))
  {
    printProblem("Error : Can't call New on type Integer");
    return NIL(Class);
  }

  else if(newClassInst == getClass("String",classes))
  {
    printProblem("Error : Can't call New on type String");
    return NIL(Class);
  }

  if(!matchingParameters(tree,classes,class,argsMethod,newClassInst->constructor->args,special,varsBloc, voidClass))
  {
    printProblem("Error : Can't call New : parameters don't match with constructor arguments");
    return NIL(Class);
  }

  return newClassInst;
}

/* Fonction qui verifie que l'affectation est possible */
ClassP checkAff(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  if(tree != NIL(Tree))
  {
    if(tree->nbChildren == 2 && (getChild(tree,1)->op == E_RESULT))
      return FALSE;
  }
  /* Si l'on affecte une String a une autre */
  ClassP temp = checkOperator(tree, getClass("String", classes), classes, class, argsMethod, special, varsBloc, voidClass);
  if (temp != NIL(Class))
    return voidClass;
  else
  {
    /* Si l'on affecte un Integer a un autre */
    temp = checkOperator(tree, getClass("Integer", classes), classes, class, argsMethod, special, varsBloc, voidClass);
    if(temp != NIL(Class))
      return voidClass;
    /* Si l'on affecte une classe (herite ou non) a une autre */
    else if(tree->nbChildren == 2 
      && checkTypeHeritage(getType(getChild(tree,1), classes, class, argsMethod, special, varsBloc, voidClass)
      , getType(getChild(tree,0), classes, class, argsMethod, special, varsBloc, voidClass)))
      {
        if(getChild(tree, 0)->u.str != NIL(char) && 
          (!strcmp(getChild(tree, 0)->u.str, "this")
          || !strcmp(getChild(tree, 0)->u.str, "super")))
          return NIL(Class);
        return voidClass;
      }
        
    return NIL(Class);
  }
}

/* Fonction qui verifie que tous les feuilles d'un arbre ne renvoient pas un NIL(Class) */
bool checkChildren(int nb, TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  int i;
  for(i = 0; i < nb; i++)
  {
    if(getChild(tree, i) != NIL(Tree))
    {
      if(getType(getChild(tree,i), classes, class, argsMethod, special, varsBloc, voidClass) == NIL(Class))
        return FALSE;
    }
  }
  return TRUE;
}

/* Fonction qui renvoie une classe correspondant au nom de la variable 
 * rentree en parametre, en tenant compte du masquage */
ClassP checkVarsMasked(char* name, ClassP class, VarDeclP argsMethod, VarDeclP varsBloc, ClassP voidClass)
{
  int i, level = class->level;
  VarDeclP it;
  
  for(it = varsBloc; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(name, it->name))
      return it->typeClass;
  }
  
  for(it = argsMethod; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(name, it->name))
      return it->typeClass;
  }

  for(i = level; i >= 0; i--)
  {
    for(it = class->vars; it != NIL(VarDecl); it = it->next)
    {
      if(!strcmp(name, it->name) && it->level == i)
        return it->typeClass;
    }
  }
  return NIL(Class);
}

/* Fonction qui verifie les variables spéciales this super et result */
ClassP checkSpecial(char * name, VarDeclP specials)
{
  VarDeclP it;
  for(it = specials; it != NIL(VarDecl); it = it->next)
  {
    if(!strcmp(it->name,name))
      return it->typeClass;
  }

  printProblem("Error with : result, this or super");
  return NIL(Class);
}

/* Fonction qui renvoie la classe d'un arbre et NIL(Class) s'il y a une erreur */
ClassP getType(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  VarDeclP it = NIL(VarDecl);
  ClassP type = NIL(Class);

  if(tree == NIL(Tree))
    return NIL(Class);

  else
  {
    switch(tree->op)
    {
      /* Dans le cas d'une variable on renvoie la classe */
      case E_IDVAR:
      if(tree->u.lvar->type != NULL)
      {
        return checkVarsMasked(tree->u.lvar->name, 
                class, argsMethod, varsBloc, voidClass);
      }
      return NIL(Class);
      /* Dans le cas d'un Integer */
      case E_CONST:
        return getClass("Integer", classes);
      /* Dans le cas d'une String */
      case E_STR:
        type = getClass(tree->u.str, classes);
        if(type != NIL(Class))
          return type;
        else
          return getClass("String", classes);
      /* Dans le cas d'un opérateur pour Integer*/
      case E_NE:
      case E_EQ:
      case E_LT:
      case E_LE:
      case E_GT:
      case E_GE:
      case E_MULT:
      case E_DIV:
      case E_ADD:
      case E_MINUS:
        type = checkOperator(tree,getClass("Integer", classes), classes, class, argsMethod, special, varsBloc, voidClass);
        if(type == NIL(Class))
          printProblem("Error in : Operator");
        return type;
      /* Dans le cas d'un opérateur pour une String */
      case E_CONC:
        type = checkOperator(tree, getClass("String", classes), classes, class, argsMethod, special, varsBloc, voidClass);
        if(type == NIL(Class))
          printProblem("Error in : Operator");
        return type;
      /* Dans le cas d'une affectation */
      case E_AFF:
        type = checkAff(tree, classes, class, argsMethod, special, varsBloc, voidClass);
        if(type == NIL(Class))
          printProblem("Error in : AFF");
        return type;
      /* Dans le cas d'une instanciation */
      case E_NEW:
        return checkNew(tree, classes, class, argsMethod, special, varsBloc, voidClass);
      /* Dans le cas d'un IF THEN ELSE */
      case E_ITE:
        if(checkITE(tree, classes, class, argsMethod, special, varsBloc, voidClass))
          return voidClass;
        printProblem("Error in : IF THEN ELSE");
        return NIL(Class);
      /* Dans le cas d'un bloc */
      case E_IS:
        if(getChild(tree, 0) != NIL(Tree) && tree->nbChildren == 2)
        {
          /* On ajoute les variables du sous-bloc dans la variable temporaire */
          for(it = getVarsBloc(tree); it != NIL(VarDecl) && it->next != NIL(VarDecl); it = it->next);
          
          if(varsBloc != NIL(VarDecl) && it != NIL(VarDecl))
            it->next = varsBloc;
          
          if(checkChildren(tree->nbChildren, tree, classes, class, argsMethod, special, getVarsBloc(tree), voidClass))
          {
            /* On reactualise les variables de bloc */
            if(varsBloc != NIL(VarDecl) && it != NIL(VarDecl))
              it->next = NIL(VarDecl);

            return voidClass; 
          }
          else
          {
            if(varsBloc != NIL(VarDecl) && it != NIL(VarDecl))
              it->next = NIL(VarDecl);

            printProblem("Error in : Block");
            return NIL(Class);
          }
        }
        else
        {
          if(checkChildren(tree->nbChildren, tree, classes, class, argsMethod, special, getVarsBloc(tree), voidClass))
            return voidClass;
          printProblem("Error in : Block");
          return NIL(Class);
        }
      /* Dans le cas d'une liste de declaration de variable */
      case E_LVAR:
        for(it = tree->u.lvar; it != NIL(VarDecl); it = it->next)
        {
          if(it->aff != NIL(Tree)
            && !checkTypeHeritage(getType(it->aff, classes, class, argsMethod, special, varsBloc, voidClass), it->typeClass))
            {
              printProblem("Error in : Variable declaration");
              return NIL(Class);
            }
        }
        return voidClass;
      /* Dans le cas d'une liste de parametres */
      case E_LIST:
        if(getType(getChild(tree,0),classes, class, argsMethod, special, varsBloc, voidClass) == NIL(Class))
        {
          printProblem("Error in : List");
          return NIL(Class);
        }
        else if(tree->nbChildren == 2)
          return getType(getChild(tree,1),classes, class, argsMethod, special, varsBloc, voidClass); 
        else
          return voidClass;
      /* Dans le cas d'un appel de fonction ( Send Message ) */
      case E_SM:
        type = checkMessage(tree,classes,class,argsMethod,special,varsBloc,voidClass);
        if(type == NIL(Class))
           printProblem("Error in : Send Message");
        return type;
      /* Dans le cas d'une selection */
      case E_SEL:
        if(checkChildren(2, tree, classes, class, argsMethod, special, varsBloc, voidClass)
        && checkSelection(tree, classes, class, argsMethod, special, varsBloc, voidClass))
          return getType(getChild(tree,0),classes,getType(getChild(tree,1),
          classes, class, argsMethod, special, varsBloc, voidClass),argsMethod,special,varsBloc, voidClass);
        printProblem("Error in : Selection");
        return NIL(Class);
      /* Dans le cas d'un return */
      case E_RETU:
        return voidClass;
      /* Dans le cas d'un result */
      case E_RESULT:
        return checkSpecial("result", special);
      /* Dans le cas d'un this */
      case E_THIS:
        return checkSpecial("this", special);
      /* Dans le cas d'un super */
      case E_SUPER:
        return checkSpecial("super", special);
      /* Dans le cas d'une instruction */
      case E_INST:
        if(checkChildren(2, tree, classes, class, argsMethod, special, varsBloc, voidClass))
          return voidClass;
        printProblem("Error in : Instruction");
        return NIL(Class);
      /* Dans le cas d'une entite predefinie */
      case E_PREDEF:
        return class;
      /* Dans le cas d'un cast */
      case E_CAST:
        if(tree->nbChildren > 0 && getChild(tree,0)->op == E_RESULT)
        {
          printProblem("Error in : Cast");
          return NIL(Class);
        }
        type = getType(getChild(tree, 1),classes, class, argsMethod, special, varsBloc, voidClass);
        if(checkTypeHeritage(getType(getChild(tree, 0),classes, class, argsMethod, special, varsBloc, voidClass),type))
          return type;
        printProblem("Error in : Cast");
        return NIL(Class);
      default:
        return NIL(Class);
    } 
  }
}

/* Fonction qui convertie toutes les VarDecl dans l'arbre passe en parametre, afin de transformer 
leur champs u.str en u.lvar */
void strToLvar(TreeP tree,VarDeclP varsClass,VarDeclP argsMethod,VarDeclP special,VarDeclP varsBloc,ClassP voidClass)
{
  if(tree->op == E_IDVAR || tree->op == E_THIS 
    || tree->op == E_RESULT || tree->op == SUPER)
  {
    VarDeclP check = checkDeclared(tree->u.str, varsClass,
          argsMethod, special, varsBloc, voidClass);
      
      if(check == NIL(VarDecl))
      {
        fprintf(stderr, "Error : Variable %s "
                "undeclared previously\n", tree->u.str);
        setError(CONTEXT_ERROR);
      }
      else
        tree->u.lvar = check;
  }

  else
  {
    int i;
    for(i = 0; i < tree->nbChildren; i++)
    {
      if(getChild(tree,i) != NIL(Tree))
        strToLvar(getChild(tree,i),varsClass,argsMethod,special,varsBloc,voidClass);
    }
  }
}

/* Fonction qui renvoie la classe correspondant au nom fourni
 ou NULL si aucune classe ne correspond */
ClassP getClass(char* name, ClassP classes)
{
  if(name == NULL)
    return NIL(Class);

  ClassP it;
  for(it = classes; it != NIL(Class); it = it->next)
  {
    if(!strcmp(it->name, name))
      return it;
  }

  return NIL(Class);
}

/* Fonction qui verifie que l'on n'hérite pas de String ou Integer */
bool checkHeritage(TreeP super, ClassP classes)
{
  if(super != NIL(Tree))
    if(!strcmp(getChild(super, 0)->u.str, "Integer")
    || !strcmp(getChild(super, 0)->u.str, "String"))
    return FALSE;
  return TRUE;
}

/* Fonction qui renvoie toutes les VarDecl pour un bloc passe en parametre */
VarDeclP getVarsBloc(TreeP tree)
{
  if(tree == NIL(Tree) || tree->nbChildren != 2 || tree->op != E_IS)
    return NIL(VarDecl);

  TreeP declChamp = getChild(tree,0);
  return (declChamp->u.lvar == NIL(VarDecl) ? NIL(VarDecl) : declChamp->u.lvar);  
}

/* Fonction qui verifie que les arguments attendus par un constructeur et les arguments passes 
en parametre lors d'un new sont les même */ 
bool matchingParameters(TreeP tree, ClassP classes, ClassP class, VarDeclP argsMethod,
    VarDeclP expectedParam, VarDeclP special, VarDeclP varsBloc, ClassP voidClass)
{
  /* Aucun argument n'est fourni et aucun n'est attendu */
  if(expectedParam == NIL(VarDecl) 
    && tree != NIL(Tree)
    && (tree->op == E_NEW || tree->op == E_METH)
    && getChild(tree,1) == NIL(Tree))
     return TRUE;

   /* Si on attendait aucun argument et qu'on en fourni au moins 1, on retourne FALSE */
  if(tree == NIL(Tree) || (tree->op == E_LIST && expectedParam == NIL(VarDecl)))
    return FALSE;

  if(tree->op == E_NEW || tree->op == E_METH)
    return matchingParameters(getChild(tree,1),classes,class,argsMethod,expectedParam,special,varsBloc,voidClass);

  else if(tree->op == E_LIST)
  {
    if(tree->nbChildren > 0 && getChild(tree,0)->op == E_RESULT)
      return FALSE;
    /* Si le type de l'argument 1 et du parametre 1 sont identiques */
    if(checkTypeHeritage(getType(getChild(tree,0),classes,class,argsMethod,special,varsBloc,voidClass), getClass(expectedParam->type, classes)))
    {
      if(tree->nbChildren == 1)
      {
        if(expectedParam->next == NIL(VarDecl))
          return TRUE;
        else
          return FALSE;
      }
      else
        return matchingParameters(getChild(tree,1),classes,class,argsMethod,expectedParam->next,special,varsBloc,voidClass);
    }
    else
      return FALSE;
  }
  else
    return FALSE;
}
 
/* Fonction qui permet d'afficher une erreur */
void printProblem(char * problem)
{
  fprintf(stderr, "%s\n", problem);
  setError(CONTEXT_ERROR);
}
