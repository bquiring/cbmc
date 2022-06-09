/*******************************************************************\

Module: Show the goto functions as a dot program

Author: Benjamin Quiring

\*******************************************************************/

/// \file
/// Compute metrics for the CFG

#include "proof_cfg_metrics.h"

#include <math.h>

#include "goto_model.h"
#include "goto_program.h"
#include "pointer_expr.h"

int num_loops (const goto_programt &goto_program) {
  // number of loops = number of backward jumps
  // TODO: look at goto_program is_backwards_goto()

  std::set<int> seen;
  int num_loops = 0;

  forall_goto_program_instructions(target, goto_program) {
    if (target->is_target()) {
      seen.insert (target->target_number);
    }
    if(target->is_function_call())
    {
      for (auto gt_it = target->targets.begin(); gt_it != target->targets.end(); gt_it++) {
        if (seen.find ((*gt_it)->target_number) != seen.end()) {
          num_loops = num_loops + 1;
        }
      }
    }
  }

  return num_loops;
}

int outdegree (const goto_programt &goto_program) {
  int count = 0;
  forall_goto_program_instructions(target, goto_program) {
    if(target->is_function_call())
    {
      count = count + 1;
    }
  }
  return count;
}

// TODO inefficient to traverse graph for every function
int indegree (const symbolt &symbol, 
              const namespacet &ns, 
              const goto_functionst &goto_functions) {
  int indegree = 0;

  const auto funs = goto_functions.sorted();

  for (const auto &fun : funs) {
    const bool has_body = fun->second.body_available();
    if (has_body) {
      const goto_programt &body = fun->second.body;
      forall_goto_program_instructions(target, body) {
        if(target->is_function_call())
        {
          // only look at real function calls, not function pointer calls
          if (target->call_function().id() != ID_dereference) {
            const irep_idt call = ns.lookup(to_symbol_expr(target->call_function())).name;
            if (call == symbol.name) {
              indegree = indegree + 1;
            }
          }
        }
      }
    }
  }
  return indegree;
}

// compute an integer size for an expr
int expr_size (const exprt e) {
  if (e.has_operands()) {
    const exprt::operandst &ops = e.operands();
    int size = 1;
    for (const auto &op : ops) {
      size = size + expr_size (op);
    }
    return size;
  } else {
    return 0;
  }
}

// compute an integer size for a function body
// the size of a function body is the sum of the sizes of all expression right-hand sides,
// excluding assertions and assumptions.
// the size of an expression is equal to the number of non-trivial subexpressions, i.e.
// the number of nodes that have operands.
int function_size (const goto_programt &goto_program) {
  int size = 0;
  forall_goto_program_instructions(target, goto_program) {
    if(target->is_function_call()) {
      const exprt &f = target->call_function();
      size = size + 1;
      size = size + expr_size (f);
      const exprt::operandst &args = target->call_arguments();
      for (const auto &arg : args) {
        size = size + expr_size (arg);
      }
    } else if (target->is_set_return_value()) {
      const exprt &rhs = target->return_value();
      size = size + expr_size (rhs);
    } else if (target->is_assign()) {
      const exprt &rhs = target->assign_rhs();
      size = size + expr_size (rhs);
    }
  }
  return size;
}

int num_complex_ops (const goto_programt &goto_program) {
  int count = 0;
  forall_goto_program_instructions(target, goto_program) {
    if (target->is_function_call()) {
      const exprt &lhs = target->call_lhs();
      if (lhs.has_operands()) {
        count = count + 1;
      }
    } else if (target->is_assign()) {
      const exprt &lhs = target->assign_lhs();
      if (lhs.has_operands()) {
        count = count + 1;
      }
    }
  }
  return count;
}

