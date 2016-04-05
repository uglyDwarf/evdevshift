%{
  #include "parser.h"

%}

%option noyywrap
%option nounput
%option noinput
%option nodefault
%option warn

EOL             (\r|\n|\n\r|\r\n)
STRING          [a-zA-Z_][a-zA-Z0-9_]*
NUMBER          [0-9]+
LBRACE          \{
RBRACE          \}
LPAREN          \(
RPAREN          \)
COMA            ,
EQ              =
COMMENT         \/\/.*

%%

button      {return TOKEN_BUTTON;}
axis        {return TOKEN_AXIS;}
if          {return TOKEN_IF;}
device      {return TOKEN_DEVICE;}
grab        {return TOKEN_GRAB;}
{EQ}        {return TOKEN_EQ;}
{LPAREN}    {return TOKEN_LPAREN;}
{RPAREN}    {return TOKEN_RPAREN;}
{LBRACE}    {return TOKEN_LBRACE;}
{RBRACE}    {return TOKEN_RBRACE;}
{COMA}      {return TOKEN_COMA;}

{EOL}       {++edslineno;}
[[:blank:]]+      /*eat whitespaces*/

{NUMBER}    {
              edslval.num = atoi(yytext);
              return TOKEN_NUMBER;
            }

{STRING}    {
              edslval.str = strdup(yytext);
              return TOKEN_STRING;
            }
["][^"]*["] {
              size_t len = strlen(yytext);
              //skip the first and leave out the last
              edslval.str = strndup(yytext + 1, len - 2);
              return TOKEN_STRING;
            }
{COMMENT}

. {printf("Unmatched character '%s' at line %d.\n", edstext, edslineno);}


%%
