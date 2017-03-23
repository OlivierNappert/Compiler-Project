Chiffre [0-9]
Entier {Chiffre}+
Lower	[a-z]
Upper	[A-Z]
Lettre	[a-zA-Z]
LC 	({Lettre}|{Chiffre})
Delim   [ \t\n\r]
Symbole [&+\-*/();:,.{}]
BS      \\.
PasBSG [^\\"]
Guill		"\""
Star  		"*"
NotStar		[^*]
NotStarPar      [^*/]

%{
#include <string.h>
#include <strings.h>
#include "tp.h"
#include "tp_y.h"

extern char *strdup(char *);
extern YYSTYPE yylval;
extern int yylineno;
extern int yylex();

int keyWordOrId()
{
   if(!strcmp(yytext, "if")) return IF;
   if(!strcmp(yytext, "then")) return THEN;
   if(!strcmp(yytext, "else")) return ELSE;
   if(!strcmp(yytext, "return")) return RETURN;
   if(!strcmp(yytext, "as")) return AS;
   if(!strcmp(yytext, "class")) return CLASS;
   if(!strcmp(yytext, "is")) return IS;
   if(!strcmp(yytext, "extends")) return EXTENDS;
   if(!strcmp(yytext, "def")) return DEF;
   if(!strcmp(yytext, "override")) return OVERRIDE;
   if(!strcmp(yytext, "var")) return VAR;
   if(!strcmp(yytext, "new")) return NEW2;
   if(!strcmp(yytext, "this")) return THIS;
   if(!strcmp(yytext, "result")) return RESULT;
   if(!strcmp(yytext, "super")) return SUPER;

   yylval.S = strdup(yytext);
   return(Id);
}

void lexError(char c)
{
   fprintf(stderr, "Error : skipping invalid character '%c' on line %d\n", c, yylineno);
   setError(LEXICAL_ERROR);
}
%}

%%
{Entier} 	{ yylval.I = atoi(yytext); return(Cste); }
{Upper}{LC}*    { yylval.S = strdup(yytext); return TYPE; }
{Lettre}{LC}*   { return keyWordOrId(); }
{Delim}+        { }
":="		{ return(AFF); }
"+"	        { return(ADD); }
"-"	        { return(SUB); }
"*"	        { return(MUL); }
"/"	        { return(QUO); }
"&"	        { return(CONC); }
"="		{ yylval.C = E_EQ; return(RelOp); }
"<>"		{ yylval.C = E_NE; return(RelOp); }
"<="		{ yylval.C = E_LE; return(RelOp); }
">="		{ yylval.C = E_GE; return(RelOp); }
">"		{ yylval.C = E_GT; return(RelOp); }
"<"		{ yylval.C = E_LT; return(RelOp); }
{Symbole}	{ return(yytext[0]); }
"/"{Star}({NotStar}|{Star}+{NotStarPar})*{Star}+"/" { }
{Guill}({PasBSG}|{BS})*{Guill}  { yylval.S = strdup(yytext); return LITSTR; }
.	        { lexError(yytext[0]); }

