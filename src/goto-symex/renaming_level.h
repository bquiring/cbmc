/*******************************************************************\

Module: Symbolic Execution

Author: Romain Brenguier, romain.brenguier@diffblue.com

\*******************************************************************/

/// \file
/// Renaming levels

#ifndef CPROVER_GOTO_SYMEX_RENAMING_LEVEL_H
#define CPROVER_GOTO_SYMEX_RENAMING_LEVEL_H

#include <map>
#include <unordered_set>

#include <util/expr_iterator.h>
#include <util/irep.h>
#include <util/range.h>
#include <util/sharing_map.h>
#include <util/simplify_expr.h>
#include <util/ssa_expr.h>

#include "renamed.h"

/// Wrapper for a \c current_names map, which maps each identifier to an SSA
/// expression and a counter.
/// This is extended by the different symex_level structures which are used
/// during symex to ensure static single assignment (SSA) form.
struct symex_renaming_levelt
{
  symex_renaming_levelt() = default;
  virtual ~symex_renaming_levelt() = default;
  symex_renaming_levelt &
  operator=(const symex_renaming_levelt &other) = default;
  symex_renaming_levelt &operator=(symex_renaming_levelt &&other) = default;
  symex_renaming_levelt(const symex_renaming_levelt &other) = default;
  symex_renaming_levelt(symex_renaming_levelt &&other) = default;

  /// Map identifier to ssa_exprt and counter
  typedef sharing_mapt<irep_idt, std::pair<ssa_exprt, unsigned>> current_namest;
  current_namest current_names;

  /// Counter corresponding to an identifier
  unsigned current_count(const irep_idt &identifier) const
  {
    const auto r_opt = current_names.find(identifier);
    return !r_opt ? 0 : r_opt->get().second;
  }

  /// Add the \c ssa_exprt of current_names to vars
  void get_variables(std::unordered_set<ssa_exprt, irep_hash> &vars) const
  {
    current_namest::viewt view;
    current_names.get_view(view);

    for(const auto &pair : view)
    {
      vars.insert(pair.second.first);
    }
  }
};

/// Functor to set the level 0 renaming of SSA expressions.
/// Level 0 corresponds to threads.
/// The renaming is built for one particular interleaving.
struct symex_level0t : public symex_renaming_levelt
{
  renamedt<ssa_exprt, L0> operator()(
    ssa_exprt ssa_expr,
    const namespacet &ns,
    unsigned thread_nr) const;

  symex_level0t() = default;
  ~symex_level0t() override = default;
};

/// Functor to set the level 1 renaming of SSA expressions.
/// Level 1 corresponds to function frames.
/// This is to preserve locality in case of recursion
struct symex_level1t : public symex_renaming_levelt
{
  renamedt<ssa_exprt, L1> operator()(renamedt<ssa_exprt, L0> l0_expr) const;

  /// Insert the content of \p other into this renaming
  void restore_from(const current_namest &other);

  symex_level1t() = default;
  ~symex_level1t() override = default;
};

/// Functor to set the level 2 renaming of SSA expressions.
/// Level 2 corresponds to SSA.
/// This is to ensure each variable is only assigned once.
struct symex_level2t : public symex_renaming_levelt
{
  renamedt<ssa_exprt, L2> operator()(renamedt<ssa_exprt, L1> l1_expr) const;

  symex_level2t() = default;
  ~symex_level2t() override = default;
  symex_level2t &operator=(const symex_level2t &other) = default;
  symex_level2t &operator=(symex_level2t &&other) = default;
  symex_level2t(const symex_level2t &other) = default;
  symex_level2t(symex_level2t &&other) = default;
};

/// Undo all levels of renaming
exprt get_original_name(exprt expr);

/// Undo all levels of renaming
typet get_original_name(typet type);

#endif // CPROVER_GOTO_SYMEX_RENAMING_LEVEL_H
