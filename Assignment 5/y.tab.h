/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    TYPE = 258,                    /* TYPE  */
    IF = 259,                      /* IF  */
    ELSE = 260,                    /* ELSE  */
    WHILE = 261,                   /* WHILE  */
    DO = 262,                      /* DO  */
    FOR = 263,                     /* FOR  */
    ID = 264,                      /* ID  */
    NUM = 265,                     /* NUM  */
    STR = 266,                     /* STR  */
    RETURN = 267,                  /* RETURN  */
    COUT = 268,                    /* COUT  */
    CIN = 269,                     /* CIN  */
    ENDL = 270,                    /* ENDL  */
    SHL = 271,                     /* SHL  */
    SHR = 272,                     /* SHR  */
    INC = 273,                     /* INC  */
    DEC = 274,                     /* DEC  */
    AND = 275,                     /* AND  */
    OR = 276,                      /* OR  */
    EQOP = 277,                    /* EQOP  */
    RELOP = 278,                   /* RELOP  */
    NOT = 279,                     /* NOT  */
    UMINUS = 280,                  /* UMINUS  */
    LOWER_THAN_ELSE = 281          /* LOWER_THAN_ELSE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define TYPE 258
#define IF 259
#define ELSE 260
#define WHILE 261
#define DO 262
#define FOR 263
#define ID 264
#define NUM 265
#define STR 266
#define RETURN 267
#define COUT 268
#define CIN 269
#define ENDL 270
#define SHL 271
#define SHR 272
#define INC 273
#define DEC 274
#define AND 275
#define OR 276
#define EQOP 277
#define RELOP 278
#define NOT 279
#define UMINUS 280
#define LOWER_THAN_ELSE 281

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
