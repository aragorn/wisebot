/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EQ = 258,
     NEQ = 259,
     GT = 260,
     GT_EQ = 261,
     LT = 262,
     LT_EQ = 263,
     BIT_AND = 264,
     BIT_OR = 265,
     BIT_NOT = 266,
     PLUS = 267,
     MINUS = 268,
     MULTIPLY = 269,
     DIVIDE = 270,
     LOGICAL_AND = 271,
     LOGICAL_OR = 272,
     LOGICAL_NOT = 273,
     LPAREN = 274,
     RPAREN = 275,
     NAME = 276,
     VALUE = 277,
     NOT = 278,
     OPERAND_ERROR = 279,
     LLIST = 280,
     RLIST = 281,
     COMMA = 282,
     IN = 283,
     COMMON = 284
   };
#endif
#define EQ 258
#define NEQ 259
#define GT 260
#define GT_EQ 261
#define LT 262
#define LT_EQ 263
#define BIT_AND 264
#define BIT_OR 265
#define BIT_NOT 266
#define PLUS 267
#define MINUS 268
#define MULTIPLY 269
#define DIVIDE 270
#define LOGICAL_AND 271
#define LOGICAL_OR 272
#define LOGICAL_NOT 273
#define LPAREN 274
#define RPAREN 275
#define NAME 276
#define VALUE 277
#define NOT 278
#define OPERAND_ERROR 279
#define LLIST 280
#define RLIST 281
#define COMMA 282
#define IN 283
#define COMMON 284




/* Copy the first part of user declarations.  */
#line 16 "mod_qp_general_yacc2.y"

#include "common_core.h"
#include "mod_qp_general2.h"

#define yyerror __yyerror
void __yyerror(char* msg);
extern int yylex(void);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 155 "mod_qp_general_yacc2.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  20
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   113

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  30
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  7
/* YYNRULES -- Number of rules. */
#define YYNRULES  30
/* YYNRULES -- Number of states. */
#define YYNSTATES  60

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   284

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     7,    11,    14,    18,    20,    24,    28,
      32,    36,    40,    44,    48,    52,    56,    58,    62,    64,
      68,    72,    75,    79,    83,    87,    91,    95,    97,    99,
     101
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      31,     0,    -1,    31,    16,    31,    -1,    31,    17,    31,
      -1,    18,    31,    -1,    19,    31,    20,    -1,    32,    -1,
      35,     3,    35,    -1,    35,     4,    35,    -1,    35,     5,
      35,    -1,    35,     6,    35,    -1,    35,     7,    35,    -1,
      35,     8,    35,    -1,    33,    28,    33,    -1,    33,    29,
      33,    -1,    25,    34,    26,    -1,    35,    -1,    34,    27,
      35,    -1,    35,    -1,    35,     9,    35,    -1,    35,    10,
      35,    -1,    11,    35,    -1,    35,    12,    35,    -1,    35,
      13,    35,    -1,    35,    14,    35,    -1,    35,    15,    35,
      -1,    19,    35,    20,    -1,    36,    -1,    21,    -1,    22,
      -1,    24,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned char yyrline[] =
{
       0,    27,    27,    35,    43,    50,    51,    54,    60,    66,
      72,    78,    84,    90,    96,   104,   105,   125,   129,   149,
     155,   161,   166,   172,   178,   184,   190,   191,   194,   195,
     196
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "EQ", "NEQ", "GT", "GT_EQ", "LT",
  "LT_EQ", "BIT_AND", "BIT_OR", "BIT_NOT", "PLUS", "MINUS", "MULTIPLY",
  "DIVIDE", "LOGICAL_AND", "LOGICAL_OR", "LOGICAL_NOT", "LPAREN", "RPAREN",
  "NAME", "VALUE", "NOT", "OPERAND_ERROR", "LLIST", "RLIST", "COMMA", "IN",
  "COMMON", "$accept", "logi_expr", "cmp_expr", "list", "_list",
  "calc_expr", "operand", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    30,    31,    31,    31,    31,    31,    32,    32,    32,
      32,    32,    32,    32,    32,    33,    33,    34,    34,    35,
      35,    35,    35,    35,    35,    35,    35,    35,    36,    36,
      36
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     3,     2,     3,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     3,     1,     3,
       3,     2,     3,     3,     3,     3,     3,     1,     1,     1,
       1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,    28,    29,    30,     0,     0,     6,
       0,    16,    27,     0,    21,     4,     0,    16,     0,    18,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     5,    26,
      15,     0,     2,     3,    13,    16,    14,     7,     8,     9,
      10,    11,    12,    19,    20,    22,    23,    24,    25,    17
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     8,     9,    10,    18,    11,    12
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -16
static const yysigned_char yypact[] =
{
      43,    59,    43,    43,   -16,   -16,   -16,    59,     3,   -16,
     -15,    81,   -16,    59,   -16,   -16,    39,    38,    12,    88,
     -16,    43,    43,    52,    52,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    -5,   -16,   -16,
     -16,    59,   -16,     0,   -16,    88,   -16,    88,    88,    88,
      88,    88,    88,   -16,    -8,    95,    95,    97,    97,    88
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -16,    15,   -16,    89,   -16,    -1,   -16
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      14,    31,    17,    20,    31,    32,    19,    33,    34,    35,
      36,     0,    37,    23,    24,    39,    21,    15,    16,    21,
      22,     0,    45,    45,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    42,    43,    40,    41,
      59,    25,    26,    27,    28,    29,    30,    31,    32,     0,
      33,    34,    35,    36,     1,    21,    22,     0,    39,    38,
       0,     2,     3,     1,     4,     5,     0,     6,     7,     0,
       1,    13,     0,     4,     5,     0,     6,     7,    13,     0,
       4,     5,     0,     6,    25,    26,    27,    28,    29,    30,
      31,    32,     0,    33,    34,    35,    36,    31,    32,     0,
      33,    34,    35,    36,    31,    32,    31,    32,     0,    35,
      36,     0,    44,    46
};

static const yysigned_char yycheck[] =
{
       1,     9,     3,     0,     9,    10,     7,    12,    13,    14,
      15,    -1,    13,    28,    29,    20,    16,     2,     3,    16,
      17,    -1,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    21,    22,    26,    27,
      41,     3,     4,     5,     6,     7,     8,     9,    10,    -1,
      12,    13,    14,    15,    11,    16,    17,    -1,    20,    20,
      -1,    18,    19,    11,    21,    22,    -1,    24,    25,    -1,
      11,    19,    -1,    21,    22,    -1,    24,    25,    19,    -1,
      21,    22,    -1,    24,     3,     4,     5,     6,     7,     8,
       9,    10,    -1,    12,    13,    14,    15,     9,    10,    -1,
      12,    13,    14,    15,     9,    10,     9,    10,    -1,    14,
      15,    -1,    23,    24
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    11,    18,    19,    21,    22,    24,    25,    31,    32,
      33,    35,    36,    19,    35,    31,    31,    35,    34,    35,
       0,    16,    17,    28,    29,     3,     4,     5,     6,     7,
       8,     9,    10,    12,    13,    14,    15,    35,    20,    20,
      26,    27,    31,    31,    33,    35,    33,    35,    35,    35,
      35,    35,    35,    35,    35,    35,    35,    35,    35,    35
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;


  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 27 "mod_qp_general_yacc2.y"
    {
				parser_result.root_operand = yyvsp[-1];
				yyval = yyvsp[-1];
				yyval->value_type = VALUE_BOOLEAN;
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				yyval->o.expr.exec_func = expr_logical_and;
			}
    break;

  case 3:
#line 35 "mod_qp_general_yacc2.y"
    {
				parser_result.root_operand = yyvsp[-1];
				yyval = yyvsp[-1];
				yyval->value_type = VALUE_BOOLEAN;
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				yyval->o.expr.exec_func = expr_logical_or;
			}
    break;

  case 4:
#line 43 "mod_qp_general_yacc2.y"
    {
				parser_result.root_operand = yyvsp[-1];
				yyval = yyvsp[-1];
				yyval->value_type = VALUE_BOOLEAN;
				yyval->o.expr.operand1 = yyvsp[0];
				yyval->o.expr.exec_func = expr_logical_not;
			}
    break;

  case 5:
#line 50 "mod_qp_general_yacc2.y"
    { yyval = yyvsp[-1]; }
    break;

  case 6:
#line 51 "mod_qp_general_yacc2.y"
    { parser_result.root_operand = yyvsp[0]; yyval = yyvsp[0]; }
    break;

  case 7:
#line 54 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_eq_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 8:
#line 60 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_neq_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 9:
#line 66 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_gt_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 10:
#line 72 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_gteq_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 11:
#line 78 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_lt_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 12:
#line 84 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_lteq_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 13:
#line 90 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_in_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 14:
#line 96 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_common_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 15:
#line 104 "mod_qp_general_yacc2.y"
    { yyval = yyvsp[-1]; }
    break;

  case 16:
#line 105 "mod_qp_general_yacc2.y"
    {
			yyval = get_new_operand();
			if ( yyval == NULL ) {
				error("error while making list");
				YYERROR;
			}

			yyval->operand_type = OPERAND_LIST;
			yyval->value_type = VALUE_LIST;
			yyval->o.list = get_new_list();
			if ( yyval->o.list == NULL ) {
				error("error while making list");
				YYERROR;
			}
			yyval->result = NULL; // obsolete. already NULL;

			append_to_list(yyval->o.list, yyvsp[0]);
		}
    break;

  case 17:
#line 125 "mod_qp_general_yacc2.y"
    {
			yyval = yyvsp[-2];
			append_to_list(yyval->o.list, yyvsp[0]);
		}
    break;

  case 18:
#line 129 "mod_qp_general_yacc2.y"
    {
			yyval = get_new_operand();
			if ( yyval == NULL ) {
				error("error while making list");
				YYERROR;
			}

			yyval->operand_type = OPERAND_LIST;
			yyval->value_type = VALUE_LIST;
			yyval->o.list = get_new_list();
			if ( yyval->o.list == NULL ) {
				error("error while making list");
				YYERROR;
			}
			yyval->result = NULL; // obsolete. already NULL;

			append_to_list(yyval->o.list, yyvsp[0]);
		}
    break;

  case 19:
#line 149 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_bitand_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 20:
#line 155 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_bitor_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 21:
#line 161 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[0];
				if ( expr_bitnot_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 22:
#line 166 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_plus_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 23:
#line 172 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_minus_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 24:
#line 178 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_multiply_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 25:
#line 184 "mod_qp_general_yacc2.y"
    {
				yyval = yyvsp[-1];
				yyval->o.expr.operand1 = yyvsp[-2];
				yyval->o.expr.operand2 = yyvsp[0];
				if ( expr_divide_set(yyval) != SUCCESS ) YYERROR;
			}
    break;

  case 26:
#line 190 "mod_qp_general_yacc2.y"
    { yyval = yyvsp[-1]; }
    break;

  case 27:
#line 191 "mod_qp_general_yacc2.y"
    { yyval = yyvsp[0]; }
    break;

  case 28:
#line 194 "mod_qp_general_yacc2.y"
    { yyval = yyvsp[0]; }
    break;

  case 29:
#line 195 "mod_qp_general_yacc2.y"
    { yyval = yyvsp[0]; }
    break;

  case 30:
#line 196 "mod_qp_general_yacc2.y"
    { YYERROR; }
    break;


    }

/* Line 1010 of yacc.c.  */
#line 1368 "mod_qp_general_yacc.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 199 "mod_qp_general_yacc2.y"


void __yyerror(char* s)
{
	error("%s\n", s);
}

int __yyparse()
{
	return yyparse();
}


