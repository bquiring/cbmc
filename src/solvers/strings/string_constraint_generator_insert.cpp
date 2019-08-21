/*******************************************************************\

Module: Generates string constraints for the family of insert Java functions

Author: Romain Brenguier, romain.brenguier@diffblue.com

\*******************************************************************/

/// \file
/// Generates string constraints for the family of insert Java functions

#include "string_constraint_generator.h"
#include "string_refinement_invariant.h"

#include <util/deprecate.h>

/// Add axioms ensuring the result `res` corresponds to `s1` where we
/// inserted `s2` at position `offset`.
/// We write offset' for `max(0, min(res.length, offset))`.
/// These axioms are:
/// 1. res.length = s1.length + s2.length
/// 2. forall i < offset' . res[i] = s1[i]
/// 3. forall i < s2.length. res[i + offset'] = s2[i]
/// 4. forall i in [offset', s1.length). res[i + s2.length] = s1[i]
/// This is equivalent to
/// `res=concat(substring(s1, 0, offset'), concat(s2, substring(s1, offset')))`.
/// \param fresh_symbol: generator of fresh symbols
/// \param res: array of characters expression
/// \param s1: array of characters expression
/// \param s2: array of characters expression
/// \param offset: integer expression
/// \param array_pool: pool of arrays representing strings
/// \return an expression expression which is different from zero if there is
///   an exception to signal
std::pair<exprt, string_constraintst> add_axioms_for_insert(
  symbol_generatort &fresh_symbol,
  const array_string_exprt &res,
  const array_string_exprt &s1,
  const array_string_exprt &s2,
  const exprt &offset,
  array_poolt &array_pool)
{
  PRECONDITION(offset.type() == s1.length_type());

  string_constraintst constraints;
  const typet &index_type = s1.length_type();
  const exprt offset1 = maximum(
    from_integer(0, index_type),
    minimum(array_pool.get_or_create_length(s1), offset));

  // Axiom 1.
  constraints.existential.push_back(
    length_constraint_for_insert(res, s1, s2, array_pool));

  // Axiom 2.
  constraints.universal.push_back([&] { // NOLINT
    const symbol_exprt i = fresh_symbol("QA_insert1", index_type);
    return string_constraintt(i, offset1, equal_exprt(res[i], s1[i]));
  }());

  // Axiom 3.
  constraints.universal.push_back([&] { // NOLINT
    const symbol_exprt i = fresh_symbol("QA_insert2", index_type);
    return string_constraintt(
      i,
      zero_if_negative(array_pool.get_or_create_length(s2)),
      equal_exprt(res[plus_exprt(i, offset1)], s2[i]));
  }());

  // Axiom 4.
  constraints.universal.push_back([&] { // NOLINT
    const symbol_exprt i = fresh_symbol("QA_insert3", index_type);
    return string_constraintt(
      i,
      offset1,
      zero_if_negative(array_pool.get_or_create_length(s1)),
      equal_exprt(
        res[plus_exprt(i, array_pool.get_or_create_length(s2))], s1[i]));
  }());

  return {from_integer(0, get_return_code_type()), std::move(constraints)};
}

/// Add axioms ensuring the length of `res` corresponds to that of `s1` where we
/// inserted `s2`.
exprt length_constraint_for_insert(
  const array_string_exprt &res,
  const array_string_exprt &s1,
  const array_string_exprt &s2,
  array_poolt &array_pool)
{
  return equal_exprt(
    array_pool.get_or_create_length(res),
    plus_exprt(
      array_pool.get_or_create_length(s1),
      array_pool.get_or_create_length(s2)));
}

/// Insertion of a string in another at a specific index
///
// NOLINTNEXTLINE
/// \copybrief add_axioms_for_insert(symbol_generatort &fresh_symbol, const array_string_exprt &, const array_string_exprt &, const array_string_exprt &, const exprt &, array_poolt &)
// NOLINTNEXTLINE
/// \link add_axioms_for_insert(symbol_generatort &fresh_symbol, const array_string_exprt&,const array_string_exprt&,const array_string_exprt&,const exprt&,array_poolt &)
///   (More...) \endlink
///
/// If `start` and `end` arguments are given then `substring(s2, start, end)`
/// is considered instead of `s2`.
/// \param fresh_symbol: generator of fresh symbols
/// \param f: function application with arguments integer `|res|`, character
///   pointer `&res[0]`, refined_string `s1`, refined_string`s2`, integer
///   `offset`, optional integer `start` and optional integer `end`
/// \param array_pool: pool of arrays representing strings
/// \return an integer expression which is different from zero if there is an
///   exception to signal
std::pair<exprt, string_constraintst> add_axioms_for_insert(
  symbol_generatort &fresh_symbol,
  const function_application_exprt &f,
  array_poolt &array_pool)
{
  PRECONDITION(f.arguments().size() == 5);
  const array_string_exprt s1 = get_string_expr(array_pool, f.arguments()[2]);
  const array_string_exprt s2 = get_string_expr(array_pool, f.arguments()[4]);
  const array_string_exprt res =
    array_pool.find(f.arguments()[1], f.arguments()[0]);
  const exprt &offset = f.arguments()[3];
  return add_axioms_for_insert(fresh_symbol, res, s1, s2, offset, array_pool);
}
