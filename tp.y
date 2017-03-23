/* attention: NEW est defini dans tp.h Utilisez un autre token */
%token IS CLASS VAR EXTENDS DEF OVERRIDE RETURN AS IF THEN ELSE AFF
  ADD SUB CONC MUL QUO NEW2 THIS SUPER RESULT
%token<S> Id LITSTR TYPE
%token<I> Cste
%token<C> RelOp

%left RelOp
%left ADD SUB CONC
%left MUL QUO
%nonassoc unary
%left '.'

%type <pT> blockOpt AFFOpt block listInstructionsOpt listInstructions
  instruction expression Identificateur selection cible sendMessage
  idMethode instanciation listParamOpt listParam extendsOpt
%type <pC> classLOpt listClass declClass
%type <pV> listParamDeclOpt listParamDecl
  listDeclChampsOpt listDeclChamps declChamp
%type <pM> listDeclMethodeOpt listDeclMethode declMethode 
%type <I> overrideOpt
%type <S> returnOpt


%{
#include "tp.h"
#include "tp_y.h"

extern int yylex();
extern void yyerror(char *);
%}

%%
Prog : classLOpt block
{
  launchProgram($1, $2);
}
;

classLOpt: listClass { $$ = $1; }
| { $$ = NIL(Class); }
;

listClass: declClass listClass { $$ = addClass($2, $1); }
| declClass { $$ =  addClass(NIL(Class), $1); }
;

declClass: CLASS TYPE '(' listParamDeclOpt ')' extendsOpt blockOpt
         IS '{' listDeclChampsOpt listDeclMethodeOpt '}'
{
$$ = makeClass($2, $6, makeMethod(NIL(char), NIL(char), FALSE, $4, $7),
$10, $11); }
;


listParamDeclOpt: listParamDecl { $$ = $1; }
| { $$ = NIL(VarDecl); }
;

listParamDecl: Id ':' TYPE ',' listParamDecl
             { $$ = addVarDecl($5,declVar($1, $3, NIL(Tree))); }
| Id ':' TYPE { $$ = declVar($1, $3, NIL(Tree)); }
;

listParamOpt: listParam { $$ = $1; }
| { $$ = NIL(Tree); }
;


listParam: expression ',' listParam { $$ = makeTree(E_LIST, 2, $1, $3); }
| expression { $$ = makeTree(E_LIST, 1, $1); }
;


extendsOpt: EXTENDS TYPE '(' listParamOpt ')'
          { $$ = makeTree(E_METH, 2, makeLeafStr(E_IDMETH, $2), $4); }
| { $$ = NIL(Tree); }
;

blockOpt: block { $$ = $1; }
| { $$ = NIL(Tree); }
;

listDeclChampsOpt: listDeclChamps { $$ = $1; }
| { $$ = NIL(VarDecl); }
;

listDeclChamps: declChamp listDeclChamps
              { $$ = addVarDecl($2, $1); }
| declChamp { $$ = $1; }
;

declChamp: VAR Id ':' TYPE AFFOpt ';'
              { $$ = declVar($2, $4, $5); }
;

AFFOpt: AFF expression { $$ = $2; }
| { $$ = NIL(Tree); }
;

listDeclMethodeOpt: listDeclMethode { $$ = $1; }
| { $$ = NIL(Method); }
;

listDeclMethode: declMethode listDeclMethode { $$ =  addMethod($2, $1); } 
| declMethode  { $$ =   $1; }
;

declMethode: overrideOpt DEF Id '(' listParamDeclOpt ')' ':' TYPE
           AFF expression { $$ = makeMethod($3, $8, $1, $5, $10); }
| overrideOpt DEF Id '(' listParamDeclOpt ')' returnOpt IS block
  { $$ = makeMethod($3, $7, $1, $5, $9); }
;

overrideOpt: OVERRIDE { $$ = TRUE; }
| { $$ = FALSE; }
;

returnOpt: ':' TYPE { $$ = $2; }
| { $$ = NIL(char); }
;

block: '{' listInstructionsOpt '}' { $$ = makeTree(E_IS,1,$2); }
| '{' listDeclChamps IS listInstructions '}'
  { $$ = makeTree(E_IS, 2, makeLeafLVar(E_LVAR, $2), $4); }
;

listInstructionsOpt: listInstructions { $$ = $1; }
| { $$ = NIL(Tree); }
;

listInstructions: instruction listInstructions
                { $$ = makeTree(E_INST, 2, $1, $2); }
| instruction { $$ = $1; }
;

instruction: expression';'
| IF expression THEN instruction ELSE instruction
  { $$ = makeTree(E_ITE, 3, $2, $4, $6); }
| block { $$ = $1; }
| RETURN ';' { $$ = makeLeafStr(E_RETU, NIL(char)); }
| cible AFF expression ';' { $$ = makeTree(E_AFF, 2, $1, $3); }
;

expression: Identificateur { $$ = $1; }
| Cste { $$ = makeLeafInt(E_CONST, $1);}
| selection { $$ = $1; }
| '(' expression ')' { $$ = $2; }
| '(' AS TYPE ':' expression ')' { $$ = makeTree(E_CAST, 2, $5, makeLeafStr(E_STR, $3)); }
| instanciation { $$ = $1; }
| sendMessage { $$ = $1; }
| expression RelOp expression { $$ = makeTree($2, 2, $1, $3); }
| expression ADD expression { $$ = makeTree(E_ADD, 2, $1, $3); }
| expression SUB expression { $$ = makeTree(E_MINUS, 2, $1, $3); }
| expression MUL expression { $$ = makeTree(E_MULT, 2, $1, $3); }
| expression QUO expression { $$ = makeTree(E_MULT, 2, $1, $3); }
| expression CONC expression { $$ = makeTree(E_CONC, 2, $1, $3); }
| ADD expression %prec unary { $$ = makeTree(E_ADD, 2, makeLeafInt(E_CONST, 0), $2); }
| SUB expression %prec unary
  { $$ = makeTree(E_MINUS, 2, makeLeafInt(E_CONST, 0), $2); }
| LITSTR { $$ = makeLeafStr(E_STR, $1); }
;

Identificateur: Id { $$ = makeLeafStr(E_IDVAR, $1);}
| THIS { $$ = makeLeafStr(E_THIS, "this"); }
| SUPER { $$ = makeLeafStr(E_SUPER, "super"); }
| RESULT { $$ = makeLeafStr(E_RESULT, "result"); }
;

selection: expression '.' Id
         { $$ = makeTree(E_SEL, 2, makeLeafStr(E_IDVAR, $3), $1); }
;

cible: Identificateur { $$ = $1; }
|selection { $$ = $1; }
;

instanciation:  NEW2 TYPE '(' listParamOpt ')'
             { $$ = makeTree(E_NEW, 2, makeLeafStr(E_STR, $2), $4); }
;

sendMessage: expression '.' idMethode { $$ = makeTree(E_SM, 2, $1, $3); }
;

idMethode: Id '(' listParamOpt ')'
         { $$ = makeTree(E_METH, 2, makeLeafStr(E_IDMETH, $1), $3); }
;

