/*******************************************************************\

Module: Nondeterministically initializes global scope variables, except for
 constants (such as string literals, final fields) and internal variables
 (such as CPROVER and symex variables, language specific internal
 variables).

Author: Daniel Kroening, Michael Tautschnig

Date: November 2011

\*******************************************************************/

/// \file
/// Nondeterministically initializes global scope variables, except for
/// constants (such as string literals, final fields) and internal variables
/// (such as CPROVER and symex variables, language specific internal
/// variables).

#ifndef CPROVER_GOTO_INSTRUMENT_NONDET_STATIC_H
#define CPROVER_GOTO_INSTRUMENT_NONDET_STATIC_H

class goto_modelt;
class namespacet;
class goto_functionst;
class symbol_exprt;

bool is_nondet_initializable_static(
  const symbol_exprt &sym,
  const namespacet &ns);

void nondet_static(
  const namespacet &ns,
  goto_functionst &goto_functions);

void nondet_static(goto_modelt &);

#endif // CPROVER_GOTO_INSTRUMENT_NONDET_STATIC_H
