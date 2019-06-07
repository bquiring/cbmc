/*******************************************************************\

Module: Symbolic Execution

Author: Romain Brenguier, romain.brenguier@diffblue.com

\*******************************************************************/

/// \file
/// Renaming levels

#include "renaming_level.h"

#include <util/namespace.h>
#include <util/ssa_expr.h>
#include <util/symbol.h>

#include "goto_symex_state.h"

void get_variables(
  const symex_renaming_levelt &current_names,
  std::unordered_set<ssa_exprt, irep_hash> &vars)
{
  symex_renaming_levelt::viewt view;
  current_names.get_view(view);

  for(const auto &pair : view)
  {
    vars.insert(pair.second.first);
  }
}

renamedt<ssa_exprt, L0> symex_level0t::
operator()(ssa_exprt ssa_expr, const namespacet &ns, unsigned thread_nr) const
{
  // already renamed?
  if(!ssa_expr.get_level_0().empty())
    return renamedt<ssa_exprt, L0>{std::move(ssa_expr)};

  const irep_idt &obj_identifier = ssa_expr.get_object_name();

  // guards are not L0-renamed
  if(obj_identifier == goto_symex_statet::guard_identifier())
    return renamedt<ssa_exprt, L0>{std::move(ssa_expr)};

  const symbolt *s;
  const bool found_l0 = !ns.lookup(obj_identifier, s);
  INVARIANT(found_l0, "level0: failed to find " + id2string(obj_identifier));

  // don't rename shared variables or functions
  if(s->type.id() == ID_code || s->is_shared())
    return renamedt<ssa_exprt, L0>{std::move(ssa_expr)};

  // rename!
  ssa_expr.set_level_0(thread_nr);
  return renamedt<ssa_exprt, L0>{ssa_expr};
}

renamedt<ssa_exprt, L1> symex_level1t::
operator()(renamedt<ssa_exprt, L0> l0_expr) const
{
  if(
    !l0_expr.get().get_level_1().empty() ||
    !l0_expr.get().get_level_2().empty())
  {
    return renamedt<ssa_exprt, L1>{std::move(l0_expr.value())};
  }

  const irep_idt l0_name = l0_expr.get().get_l1_object_identifier();

  const auto r_opt = current_names.find(l0_name);

  if(!r_opt)
  {
    return renamedt<ssa_exprt, L1>{std::move(l0_expr.value())};
  }

  // rename!
  l0_expr.value().set_level_1(r_opt->get().second);
  return renamedt<ssa_exprt, L1>{std::move(l0_expr.value())};
}

renamedt<ssa_exprt, L2> symex_level2t::
operator()(renamedt<ssa_exprt, L1> l1_expr) const
{
  if(!l1_expr.get().get_level_2().empty())
    return renamedt<ssa_exprt, L2>{std::move(l1_expr.value())};
  l1_expr.value().set_level_2(current_count(l1_expr.get().get_identifier()));
  return renamedt<ssa_exprt, L2>{std::move(l1_expr.value())};
}

void symex_level1t::restore_from(const symex_renaming_levelt &other)
{
  symex_renaming_levelt::delta_viewt delta_view;
  other.get_delta_view(current_names, delta_view, false);

  for(const auto &delta_item : delta_view)
  {
    if(!delta_item.is_in_both_maps())
    {
      current_names.insert(delta_item.k, delta_item.m);
    }
    else
    {
      if(delta_item.m != delta_item.get_other_map_value())
      {
        current_names.replace(delta_item.k, delta_item.m);
      }
    }
  }
}

unsigned symex_level2t::current_count(const irep_idt &identifier) const
{
  const auto r_opt = current_names.find(identifier);
  return !r_opt ? 0 : r_opt->get().second;
}

exprt get_original_name(exprt expr)
{
  expr.type() = get_original_name(std::move(expr.type()));

  if(expr.id() == ID_symbol && expr.get_bool(ID_C_SSA_symbol))
    return to_ssa_expr(expr).get_original_expr();
  else
  {
    Forall_operands(it, expr)
      *it = get_original_name(std::move(*it));
    return expr;
  }
}

typet get_original_name(typet type)
{
  // rename all the symbols with their last known value

  if(type.id() == ID_array)
  {
    auto &array_type = to_array_type(type);
    array_type.subtype() = get_original_name(std::move(array_type.subtype()));
    array_type.size() = get_original_name(std::move(array_type.size()));
  }
  else if(type.id() == ID_struct || type.id() == ID_union)
  {
    struct_union_typet &s_u_type = to_struct_union_type(type);
    struct_union_typet::componentst &components = s_u_type.components();

    for(auto &component : components)
      component.type() = get_original_name(std::move(component.type()));
  }
  else if(type.id() == ID_pointer)
  {
    type.subtype() =
      get_original_name(std::move(to_pointer_type(type).subtype()));
  }
  return type;
}
