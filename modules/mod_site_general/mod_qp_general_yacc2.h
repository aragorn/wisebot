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




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



