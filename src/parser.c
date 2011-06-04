
//
// parser.c
//
// Copyright (c) 2011 TJ Holowaychuk <tj@vision-media.ca>
//

#include <stdio.h>
#include "parser.h"

// TODO: emit for codegen

#define next (luna_scan(self->lex), &self->lex->tok)
#define peek (self->la ? self->la : (self->la = next))
#define is(t) (peek->type == (LUNA_TOKEN_##t) ? 1 : 0)
#define accept(t) (peek->type == (LUNA_TOKEN_##t) ? next : 0)

// -DEBUG_PARSER

#ifdef EBUG_PARSER
#define debug(name) \
  fprintf(stderr, "\n\033[90m%s\033[0m\n", name); \
  luna_token_inspect(&self->lex->tok);
#else
#define debug(name)
#endif

// protos

static int block(luna_parser_t *self);

/*
 * Initialize with the given lexer.
 */

void
luna_parser_init(luna_parser_t *self, luna_lexer_t *lex) {
  self->lex = lex;
  self->la = NULL;
  self->error = NULL;
}

/*
 * Set error `msg` and return 0.
 */

static int
error(luna_parser_t *self, char *msg) {
  self->error = msg;
  return 0;
}

/*
 * newline*
 */

static void
whitespace(luna_parser_t *self) {
  while (accept(NEWLINE)) ;
}

/*
 *   id
 * | string
 * | int
 * | float
 */

static int
primitive_expr(luna_parser_t *self) {
  debug("primitive_expr");
  return accept(ID)
    || accept(STRING)
    || accept(INT)
    || accept(FLOAT)
    ;
}

/*
 * assignment_expr
 */

static int
expr(luna_parser_t *self) {
  debug("expr");
  return primitive_expr(self);
}

/*
 * expr
 */

static int
expr_stmt(luna_parser_t *self) {
  debug("expr_stmt");
  return expr(self);
}

/*
 *  ('if' | 'unless') expr block
 *  ('else' 'if' block)*
 *  ('else' block)?
 */

static int
if_stmt(luna_parser_t *self) {
  debug("if_stmt");

  accept(IF) || accept(UNLESS);

  if (!expr(self)) return error(self, "if missing condition");
  if (!block(self)) return error(self, "if missing block");

  // else
loop:
  if (accept(ELSE)) {
    // else if
    if (accept(IF)) {
      if (!expr(self)) return error(self, "else if missing condition");
      if (!block(self)) return error(self, "else if missing block");
      goto loop;
    } else if (!block(self)) {
      return error(self, "else missing block");
    }
  }

  return 1;
}

/*
 * ('while' | 'until')
 */

static int
while_stmt(luna_parser_t *self) {
  debug("while_stmt");
  accept(WHILE) || accept(UNTIL);
  if (!expr(self)) return error(self, "while missing condition");
  if (!block(self)) return error(self, "while missing block");
  return 1;
}

/*
 *   if_stmt
 * | while_stmt
 * | expr_stmt
 */

static int
stmt(luna_parser_t *self) {
  debug("stmt");
  if (is(IF) || is(UNLESS)) return if_stmt(self);
  if (is(WHILE) || is(UNTIL)) return while_stmt(self);
  return expr_stmt(self);
}

/*
 * INDENT ws (stmt ws)+ OUTDENT
 */

static int
block(luna_parser_t *self) {
  debug("block");
  if (!accept(INDENT)) return error(self, "block missing indentation");
  whitespace(self);
  do {
    if (!stmt(self)) return 0;
    whitespace(self);
  } while (!accept(OUTDENT));
  return 1;
}

/*
 * ws (stmt ws)*
 */

static int
program(luna_parser_t *self) {
  whitespace(self);
  debug("program");
  while (!accept(EOS)) {
    if (!stmt(self)) return 0;
    whitespace(self);
  }
  return 1;
}

/*
 * Parse input.
 */

int
luna_parse(luna_parser_t *self) {
  luna_lexer_t *lex = self->lex;
  return program(self);
}
