%{
#include "common_core.h"
#include "mod_qpp2.h"
#include "qpp_yacc.h"
extern int yylineno;
int  tokenpos = 0;
char linebuf[501];
void yyerror(const char* msg);

#define SET_OPERAND \
    yylval = new_operand(); \
    if ( yylval == NULL ) { \
        error("error while processing [%s]", yytext); \
        return OPERAND_ERROR; \
    }

#define SET_EXPR \
    yylval->operand_type = OPERAND_EXPR; \
    yylval->result = &yylval->o.expr.result; \
    strncpy( yylval->o.expr.operator_string, yytext, sizeof(yylval->o.expr.operator_string) );
%}

%option caseless yylineno
%array
%%

\n.*               { 
  strncpy(linebuf, yytext+1, 500); /* Save next line */
  linebuf[500] = '\0';
  tokenpos = 0;
  yyless(1); /* Flush back all but \n to input */
}


SELECT             { return SELECT; }
SEARCH             { return SEARCH; }
WHERE              { return WHERE;  }
LIMIT              { return LIMIT;  }
BY                 { return BY;     }
GROUP              { return GROUP;  }
COUNT              { return COUNT;  }
GROUP_BY           { return GROUP_BY;  }
COUNT_BY           { return COUNT_BY;  }
VIRTUAL_ID         { return VIRTUAL_ID;  }
ORDER              { return ORDER;  }
ORDER_BY           { return ORDER_BY;  }
ASC                { return ASC;    }
DESC               { return DESC;   }
IN                 { return IN;     }
IS                 { return IS;     }
NULL               { return NULLX;  }
BETWEEN            { return BETWEEN;  }
AND                { return AND;    }
OR                 { return OR;     }
NOT                { return NOT;    }

"="  |
"!=" |
"<>" |
"<"  |
">"  |
"<=" |
">="               { return COMPARISON; }
[-+*/:(),.;]       { return yytext[0]; }

  /* 이름 */
[A-Za-z][A-Za-z0-9_]*    { return NAME; }

  /* 숫자 */
[0-9]+             |
[0-9]+"."[0-9]*    |
"."[0-9]*          { return INTNUM; }

  /* 문자열 */
'[^'\n]*'          {
	int c = input();
	unput(c); /* just peeking */
	if (c != '\'') return STRING;
	else yymore();
}

'[^'\n]*$          { yyerror("Unterminated string"); }

[ \t\r\n]+         ; /* 공백 */
"--".*             ; /* 주석 */

.                  { yyerror("Invalid character"); }

%%

/* */

void yyerror(const char *s)
{
	fprintf(stderr, "line %d, char %d: %s\n%s\n",
            yylineno, tokenpos, s, linebuf);
    fprintf(stderr, "%*s\n", 1+tokenpos, "^");
}
