/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     SELECT = 258,
     SEARCH = 259,
     WHERE = 260,
     LIMIT = 261,
     BY = 262,
     GROUP = 263,
     COUNT = 264,
     GROUP_BY = 265,
     COUNT_BY = 266,
     VIRTUAL_ID = 267,
     ORDER = 268,
     ORDER_BY = 269,
     ASC = 270,
     DESC = 271,
     IN = 272,
     IS = 273,
     NULLX = 274,
     COMPARISON = 275,
     BETWEEN = 276,
     FIELD = 277,
     TERM = 278,
     AND = 279,
     OR = 280,
     NOT = 281,
     BOOLEAN = 282,
     WITHIN = 283,
     PHRASE = 284,
     NOOP = 285,
     LPAREN = 286,
     RPAREN = 287,
     QSTRING = 288,
     STRING = 289,
     NAME = 290,
     INTNUM = 291,
     FUNCTION_NAME = 292,
     NON_EMPTY = 293,
     TEST = 294,
     UMINUS = 295
   };
#endif
/* Tokens.  */
#define SELECT 258
#define SEARCH 259
#define WHERE 260
#define LIMIT 261
#define BY 262
#define GROUP 263
#define COUNT 264
#define GROUP_BY 265
#define COUNT_BY 266
#define VIRTUAL_ID 267
#define ORDER 268
#define ORDER_BY 269
#define ASC 270
#define DESC 271
#define IN 272
#define IS 273
#define NULLX 274
#define COMPARISON 275
#define BETWEEN 276
#define FIELD 277
#define TERM 278
#define AND 279
#define OR 280
#define NOT 281
#define BOOLEAN 282
#define WITHIN 283
#define PHRASE 284
#define NOOP 285
#define LPAREN 286
#define RPAREN 287
#define QSTRING 288
#define STRING 289
#define NAME 290
#define INTNUM 291
#define FUNCTION_NAME 292
#define NON_EMPTY 293
#define TEST 294
#define UMINUS 295




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

