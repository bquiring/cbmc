/*******************************************************************\

Module: Conversion between exprt and miniBDD

Author: Michael Tautschnig, michael.tautschnig@qmul.ac.uk

\*******************************************************************/

/// \file
/// Conversion between exprt and miniBDD

#ifndef CPROVER_SOLVERS_PROP_BDD_EXPR_H
#define CPROVER_SOLVERS_PROP_BDD_EXPR_H

/*! \file solvers/prop/bdd_expr.h
 * \brief Binary decision diagram
 *
 * \author Michael Tautschnig, michael.tautschnig@qmul.ac.uk
 * \date   Sat, 02 Jan 2016 20:26:19 +0100
*/

#include <util/expr.h>

#include <solvers/bdd/bdd.h>

#include <unordered_map>

class namespacet;

/*! \brief TO_BE_DOCUMENTED
*/
class bdd_exprt
{
public:
  explicit bdd_exprt(const namespacet &_ns) : ns(_ns), root(bdd_mgr.bdd_true())
  {
  }

  void from_expr(const exprt &expr);
  exprt as_expr() const;

protected:
  const namespacet &ns;
  bdd_managert bdd_mgr;
  bddt root;

  typedef std::unordered_map<exprt, bddt, irep_hash> expr_mapt;
  expr_mapt expr_map;
  std::vector<exprt> node_map;

  bddt from_expr_rec(const exprt &expr);
  exprt as_expr(const bdd_nodet &r, bool complement) const;
};

#endif // CPROVER_SOLVERS_PROP_BDD_EXPR_H
