/*******************************************************************\

Module: Guard Data Structure

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/// \file
/// Guard Data Structure

#ifndef CPROVER_ANALYSES_GUARD_EXPR_H
#define CPROVER_ANALYSES_GUARD_EXPR_H

#include <iosfwd>

#include <util/std_expr.h>

/// This is unused by this implementation of guards, but can be used by other
/// implementations of the same interface.
struct guard_expr_managert
{
};

class guard_exprt
{
public:
  /// Construct a BDD from an expression
  /// The \c guard_managert parameter is not used, but we keep it for uniformity
  /// with other implementations of guards which may use it.
  explicit guard_exprt(const exprt &e, guard_expr_managert &) : expr(e)
  {
  }

  guard_exprt &operator=(const guard_exprt &other)
  {
    expr = other.expr;
    return *this;
  }

  void add(const exprt &expr);

  void append(const guard_exprt &guard)
  {
    add(guard.as_expr());
  }

  exprt as_expr() const
  {
    return expr;
  }

  void guard_expr(exprt &dest) const;

  bool is_true() const
  {
    return expr.is_true();
  }

  bool is_false() const
  {
    return expr.is_false();
  }

  friend guard_exprt &operator-=(guard_exprt &g1, const guard_exprt &g2);
  friend guard_exprt &operator|=(guard_exprt &g1, const guard_exprt &g2);

private:
  exprt expr;
};

#endif // CPROVER_ANALYSES_GUARD_EXPR_H
