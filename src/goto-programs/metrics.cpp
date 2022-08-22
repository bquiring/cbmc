/*******************************************************************\

Module: Show the goto functions as a dot program

Author: Benjamin Quiring

\*******************************************************************/

/// \file
/// Compute metrics for the CFG

#include "metrics.h"

#include <math.h>
#include <iostream>

#include <goto_model.h>
#include <pointer_expr.h>

int num_func_pointer_calls (const std::vector<std::vector<goto_programt::const_targett>> &instructions) {
  // number of loops = number of backward jumps
  int count = 0;
  for (const auto &insts : instructions) {
    for (const auto &target : insts) {
      if(target->is_function_call()) {
        count += (target->call_function().id() == ID_dereference);
      }
    }
  }

  return count;
}

int num_loops (const std::vector<std::vector<goto_programt::const_targett>> &instructions) {
  // number of loops = number of backward jumps
  // TODO: look at goto_program is_backwards_goto()

  std::set<int> seen;
  int num_loops = 0;

  for (const auto &insts : instructions) {
    for (const auto &target : insts) {
      if (target->is_target()) {
        seen.insert (target->target_number);
      }
      if(target->is_goto())
      {
        for (auto gt_it = target->targets.begin(); gt_it != target->targets.end(); gt_it++) {
          if (seen.find ((*gt_it)->target_number) != seen.end()) {
            num_loops = num_loops + 1;
          }
        }
      }
    }
  }

  return num_loops;
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
int function_size (const std::vector<std::vector<goto_programt::const_targett>> &instructions) {
  int size = 0;
  for (const auto &insts : instructions) {
    for (const auto &target : insts) {
      if(target->is_function_call()) {
        size += 1 + expr_size (target->call_function());
        for (const auto &arg : target->call_arguments()) {
          size += expr_size (arg);
        }
      } else if (target->is_assign()) {
        size += expr_size (target->assign_lhs());
        size += expr_size (target->assign_rhs());
      } else if (target->is_assume()) {
        size += expr_size (target->condition());
      } else if (target->is_set_return_value()) {
        size += expr_size (target->return_value());
      } 
    }
  }
  return size;
}

int num_complex_user_ops_expr (const exprt &e) {
  int sum = 0;

  if (e.id() == ID_dereference || // pointer dereference
      e.id() == ID_pointer_offset || // pointer dereference with offset?
      e.id() == ID_field_address || // struct field selection
      e.id() == ID_index) { // indexing an array
    sum += 1;
  } 

  if (e.has_operands()) {
    for (const exprt &oper : e.operands()) {
      sum += num_complex_user_ops_expr (oper);
    }
  }

  return sum;
}

int num_complex_user_ops (const std::vector<std::vector<goto_programt::const_targett>> &instructions) {
  int count = 0;
  int count2 = 0;
  std::set<std::string> to_look_for;
  to_look_for.insert ("malloc");
  to_look_for.insert ("realloc");
  to_look_for.insert ("free");
  to_look_for.insert ("memcpy");
  to_look_for.insert ("memmove");
  to_look_for.insert ("memcmp");

  for (const auto &insts : instructions) {
    for (const auto &target : insts) {
      if (target->is_function_call()) {

        if (target->call_function().id() == ID_symbol) {
          std::string s = id2string(to_symbol_expr(target->call_function()).get_identifier());
          if (to_look_for.find (s) != to_look_for.end()) {
            count2 += 1;
          }
        }

        count += num_complex_user_ops_expr (target->call_lhs());
        for (const auto &oper : target->call_arguments()) {
          count += num_complex_user_ops_expr (oper);
        }
      } else if (target->is_assign()) {
        count += num_complex_user_ops_expr (target->assign_lhs());
        count += num_complex_user_ops_expr (target->assign_rhs());
      } else if (target->is_assert()) {
        count += num_complex_user_ops_expr (target->condition());
      } else if (target->is_assume()) {
        count += num_complex_user_ops_expr (target->condition());
      } else if (target->is_set_return_value()) {
        count += num_complex_user_ops_expr (target->return_value());
      }
    }
  }
  return count + count2;
}

int num_complex_cbmc_ops_expr (const exprt &e) {
  int sum = 0;

  const irep_idt &e_id = e.id();
  if (e_id == ID_byte_extract_big_endian ||
      e_id == ID_byte_extract_little_endian ||
      e_id == ID_byte_update_big_endian ||
      e_id == ID_byte_update_little_endian ||
      e_id == ID_allocate) {
    sum += 1;
  }

  if (e.has_operands()) {
    for (const exprt &oper : e.operands()) {
      sum += num_complex_cbmc_ops_expr (oper);
    }
  }

  return sum;
}

int num_complex_cbmc_ops (const std::vector<std::vector<goto_programt::const_targett>> &instructions) {
  int count = 0;
  for (const auto &insts : instructions) {
    for (const auto &target : insts) {
      if (target->is_function_call()) {
        count += num_complex_cbmc_ops_expr (target->call_lhs());
        for (const auto &oper : target->call_arguments()) {
          count += num_complex_cbmc_ops_expr (oper);
        }
      } else if (target->is_assign()) {
        count += num_complex_cbmc_ops_expr (target->assign_lhs());
        count += num_complex_cbmc_ops_expr (target->assign_rhs());
      } else if (target->is_assert()) {
        count += num_complex_cbmc_ops_expr (target->condition());
      } else if (target->is_assume()) {
        count += num_complex_cbmc_ops_expr (target->condition());
      } else if (target->is_set_return_value()) {
        count += num_complex_cbmc_ops_expr (target->return_value());
      }
    }
  }
  return count;
}

const double func_metricst::avg_time_per_symex_step () const {
  if (symex_info.steps == 0) {
    return 0.0;
  }
  return (symex_info.duration / (double)symex_info.steps);
}

const void func_metricst::dump_html (std::ostream &out) const {
  std::string endline = " <br/> ";
  int avg_time_per_step = (int)avg_time_per_symex_step()/10000;
  out << "complex user ops: " << num_complex_user_ops << endline
      << "complex CBMC ops: " << num_complex_cbmc_ops << endline
      << "overall function size: " << function_size << endline
      << "function pointer calls: " << num_func_pointer_calls << endline
      << "loops: " << num_loops;

  if (use_symex_info) {
    out << endline
        << "symex steps: " << symex_info.steps << endline
        << "symex duration (ms): " << (int)(symex_info.duration / 1000000.0) << endline
        << "symex avg time per step: " << avg_time_per_step;
  }
  // if (use_solver_info) {
  //   out << endline
  //       << "solver clauses: " << solver_info.clauses / 1000 << "k" << endline
  //       << "solver literals: " << solver_info.literals / 1000 << "k" << endline
  //       << "solver variables: " << solver_info.variables / 1000 << "k";
  // }
  if (use_solver_info) {
    out << endline
        << "solver clauses: " << solver_info.clauses << endline
        << "solver literals: " << solver_info.literals << endline
        << "solver variables: " << solver_info.variables;
  }
}
