/*******************************************************************\

Module: Symbolic Execution

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/// \file
/// Symbolic Execution

#include "goto_symex.h"

#include <cassert>
#include <memory>

#include <util/exception_utils.h>
#include <util/expr_util.h>
#include <util/make_unique.h>
#include <util/mathematical_expr.h>
#include <util/replace_symbol.h>
#include <util/std_expr.h>
#include <util/string2int.h>
#include <util/symbol_table.h>

#include <analyses/dirty.h>

symex_configt::symex_configt(const optionst &options)
  : max_depth(options.get_unsigned_int_option("depth")),
    doing_path_exploration(options.is_set("paths")),
    allow_pointer_unsoundness(
      options.get_bool_option("allow-pointer-unsoundness")),
    constant_propagation(options.get_bool_option("propagation")),
    self_loops_to_assumptions(
      options.get_bool_option("self-loops-to-assumptions")),
    simplify_opt(options.get_bool_option("simplify")),
    unwinding_assertions(options.get_bool_option("unwinding-assertions")),
    partial_loops(options.get_bool_option("partial-loops")),
    debug_level(unsafe_string2int(options.get_option("debug-level"))),
    run_validation_checks(options.get_bool_option("validate-ssa-equation"))
{
}

void symex_transition(
  goto_symext::statet &state,
  goto_programt::const_targett to,
  bool is_backwards_goto)
{
  if(!state.call_stack().empty())
  {
    // initialize the loop counter of any loop we are newly entering
    // upon this transition; we are entering a loop if
    // 1. the transition from state.source.pc to "to" is not a backwards goto
    // or
    // 2. we are arriving from an outer loop
    framet &frame = state.top();
    const goto_programt::instructiont &instruction=*to;
    for(const auto &i_e : instruction.incoming_edges)
      if(i_e->is_goto() && i_e->is_backwards_goto() &&
         (!is_backwards_goto ||
          state.source.pc->location_number>i_e->location_number))
      {
        const auto loop_id =
          goto_programt::loop_id(state.source.function_id, *i_e);
        frame.loop_iterations[loop_id].count = 0;
      }
  }

  state.source.pc=to;
}

void symex_transition(goto_symext::statet &state)
{
  goto_programt::const_targett next = state.source.pc;
  ++next;
  symex_transition(state, next, false);
}

void goto_symext::symex_assert(
  const goto_programt::instructiont &instruction,
  statet &state)
{
  exprt condition = instruction.get_condition();
  clean_expr(condition, state, false);

  // we are willing to re-write some quantified expressions
  if(has_subexpr(condition, ID_exists) || has_subexpr(condition, ID_forall))
  {
    // have negation pushed inwards as far as possible
    do_simplify(condition);
    rewrite_quantifiers(condition, state);
  }

  // now rename, enables propagation
  exprt l2_condition = state.rename(std::move(condition), ns);

  // now try simplifier on it
  do_simplify(l2_condition);

  std::string msg = id2string(instruction.source_location.get_comment());
  if(msg == "")
    msg = "assertion";

  vcc(l2_condition, msg, state);
}

void goto_symext::vcc(
  const exprt &condition,
  const std::string &msg,
  statet &state)
{
  state.total_vccs++;
  path_segment_vccs++;

  if(condition.is_true())
    return;

  exprt guarded_condition = condition;
  state.guard.guard_expr(guarded_condition);

  state.remaining_vccs++;
  target.assertion(state.guard.as_expr(), guarded_condition, msg, state.source);
}

void goto_symext::symex_assume(statet &state, const exprt &cond)
{
  exprt simplified_cond=cond;

  clean_expr(simplified_cond, state, false);
  simplified_cond = state.rename(std::move(simplified_cond), ns);
  do_simplify(simplified_cond);

  symex_assume_l2(state, simplified_cond);
}

void goto_symext::symex_assume_l2(statet &state, const exprt &cond)
{
  if(cond.is_true())
    return;

  if(state.threads.size()==1)
  {
    exprt tmp = cond;
    state.guard.guard_expr(tmp);
    target.assumption(state.guard.as_expr(), tmp, state.source);
  }
  // symex_target_equationt::convert_assertions would fail to
  // consider assumptions of threads that have a thread-id above that
  // of the thread containing the assertion:
  // T0                     T1
  // x=0;                   assume(x==1);
  // assert(x!=42);         x=42;
  else
    state.guard.add(cond);

  if(state.atomic_section_id!=0 &&
     state.guard.is_false())
    symex_atomic_end(state);
}

void goto_symext::rewrite_quantifiers(exprt &expr, statet &state)
{
  if(expr.id()==ID_forall)
  {
    // forall X. P -> P
    // we keep the quantified variable unique by means of L2 renaming
    auto &quant_expr = to_quantifier_expr(expr);
    symbol_exprt tmp0 =
      to_symbol_expr(to_ssa_expr(quant_expr.symbol()).get_original_expr());
    symex_decl(state, tmp0);
    exprt tmp = quant_expr.where();
    rewrite_quantifiers(tmp, state);
    quant_expr.swap(tmp);
  }
  else if(expr.id() == ID_or || expr.id() == ID_and)
  {
    for(auto &op : expr.operands())
      rewrite_quantifiers(op, state);
  }
}

static void
switch_to_thread(goto_symex_statet &state, const unsigned int thread_nb)
{
  PRECONDITION(state.source.thread_nr < state.threads.size());
  PRECONDITION(thread_nb < state.threads.size());

  // save PC
  state.threads[state.source.thread_nr].pc = state.source.pc;
  state.threads[state.source.thread_nr].atomic_section_id =
    state.atomic_section_id;

  // get new PC
  state.source.thread_nr = thread_nb;
  state.source.pc = state.threads[thread_nb].pc;

  state.guard = state.threads[thread_nb].guard;
}

void goto_symext::symex_threaded_step(
  statet &state, const get_goto_functiont &get_goto_function)
{
  symex_step(get_goto_function, state);

  _total_vccs = state.total_vccs;
  _remaining_vccs = state.remaining_vccs;

  if(should_pause_symex)
    return;

  // is there another thread to execute?
  if(state.call_stack().empty() &&
     state.source.thread_nr+1<state.threads.size())
  {
    unsigned t=state.source.thread_nr+1;
#if 0
    std::cout << "********* Now executing thread " << t << '\n';
#endif
    switch_to_thread(state, t);
    symex_transition(state, state.source.pc, false);
  }
}

void goto_symext::symex_with_state(
  statet &state,
  const get_goto_functiont &get_goto_function,
  symbol_tablet &new_symbol_table)
{
  // resets the namespace to only wrap a single symbol table, and does so upon
  // destruction of an object of this type; instantiating the type is thus all
  // that's needed to achieve a reset upon exiting this method
  struct reset_namespacet
  {
    explicit reset_namespacet(namespacet &ns) : ns(ns)
    {
    }

    ~reset_namespacet()
    {
      const symbol_tablet &st = ns.get_symbol_table();
      ns = namespacet(st);
    }

    namespacet &ns;
  };

  // We'll be using ns during symbolic execution and it needs to know
  // about the names minted in `state`, so make it point both to
  // `state`'s symbol table and the symbol table of the original
  // goto-program.
  ns = namespacet(outer_symbol_table, state.symbol_table);

  // whichever way we exit this method, reset the namespace back to a sane state
  // as state.symbol_table might go out of scope
  reset_namespacet reset_ns(ns);

  PRECONDITION(state.top().end_of_function->is_end_function());

  symex_threaded_step(state, get_goto_function);
  if(should_pause_symex)
    return;
  while(!state.call_stack().empty())
  {
    state.has_saved_jump_target = false;
    state.has_saved_next_instruction = false;
    symex_threaded_step(state, get_goto_function);
    if(should_pause_symex)
      return;
  }

  // Clients may need to construct a namespace with both the names in
  // the original goto-program and the names generated during symbolic
  // execution, so return the names generated through symbolic execution
  // through `new_symbol_table`.
  new_symbol_table = state.symbol_table;
}

void goto_symext::resume_symex_from_saved_state(
  const get_goto_functiont &get_goto_function,
  const statet &saved_state,
  symex_target_equationt *const saved_equation,
  symbol_tablet &new_symbol_table)
{
  // saved_state contains a pointer to a symex_target_equationt that is
  // almost certainly stale. This is because equations are owned by bmcts,
  // and we construct a new bmct for every path that we execute. We're on a
  // new path now, so the old bmct and the equation that it owned have now
  // been deallocated. So, construct a new state from the old one, and make
  // its equation member point to the (valid) equation passed as an argument.
  statet state(saved_state, saved_equation);

  // Do NOT do the same initialization that `symex_with_state` does for a
  // fresh state, as that would clobber the saved state's program counter
  symex_with_state(
      state,
      get_goto_function,
      new_symbol_table);
}

std::unique_ptr<goto_symext::statet> goto_symext::initialize_entry_point_state(
  const get_goto_functiont &get_goto_function)
{
  const irep_idt entry_point_id = goto_functionst::entry_point();

  const goto_functionst::goto_functiont *start_function;
  try
  {
    start_function = &get_goto_function(entry_point_id);
  }
  catch(const std::out_of_range &)
  {
    throw unsupported_operation_exceptiont("the program has no entry point");
  }

  // create and prepare the state
  auto state = util_make_unique<statet>(
    symex_targett::sourcet(entry_point_id, start_function->body));
  CHECK_RETURN(!state->threads.empty());
  CHECK_RETURN(!state->call_stack().empty());

  goto_programt::const_targett limit =
    std::prev(start_function->body.instructions.end());
  state->top().end_of_function = limit;
  state->top().calling_location.pc = state->top().end_of_function;
  state->top().hidden_function = start_function->is_hidden();

  state->symex_target = &target;

  state->run_validation_checks = symex_config.run_validation_checks;

  // initialize support analyses
  auto emplace_safe_pointers_result =
    path_storage.safe_pointers.emplace(entry_point_id, local_safe_pointerst{});
  if(emplace_safe_pointers_result.second)
    emplace_safe_pointers_result.first->second(start_function->body);

  state->dirty.populate_dirty_for_function(entry_point_id, *start_function);

  // make the first step onto the instruction pointed to by the initial program
  // counter
  symex_transition(*state, state->source.pc, false);

  return state;
}

void goto_symext::symex_from_entry_point_of(
  const get_goto_functiont &get_goto_function,
  symbol_tablet &new_symbol_table)
{
  auto state = initialize_entry_point_state(get_goto_function);

  symex_with_state(*state, get_goto_function, new_symbol_table);
}

void goto_symext::initialize_path_storage_from_entry_point_of(
  const get_goto_functiont &get_goto_function,
  symbol_tablet &new_symbol_table)
{
  auto state = initialize_entry_point_state(get_goto_function);

  path_storaget::patht entry_point_start(target, *state);
  entry_point_start.state.saved_target = state->source.pc;
  entry_point_start.state.has_saved_next_instruction = true;

  path_storage.push(entry_point_start);
}

goto_symext::get_goto_functiont
goto_symext::get_goto_function(abstract_goto_modelt &goto_model)
{
  return [&goto_model](
           const irep_idt &id) -> const goto_functionst::goto_functiont & {
    return goto_model.get_goto_function(id);
  };
}

/// do just one step
void goto_symext::symex_step(
  const get_goto_functiont &get_goto_function,
  statet &state)
{
  #if 0
  std::cout << "\ninstruction type is " << state.source.pc->type << '\n';
  std::cout << "Location: " << state.source.pc->source_location << '\n';
  std::cout << "Guard: " << format(state.guard.as_expr()) << '\n';
  std::cout << "Code: " << format(state.source.pc->code) << '\n';
  #endif

  PRECONDITION(!state.threads.empty());
  PRECONDITION(!state.call_stack().empty());

  const goto_programt::instructiont &instruction=*state.source.pc;

  if(!symex_config.doing_path_exploration)
    merge_gotos(state);

  // depth exceeded?
  if(symex_config.max_depth != 0 && state.depth > symex_config.max_depth)
    state.guard.add(false_exprt());
  state.depth++;

  // actually do instruction
  switch(instruction.type)
  {
  case SKIP:
    if(!state.guard.is_false())
      target.location(state.guard.as_expr(), state.source);
    symex_transition(state);
    break;

  case END_FUNCTION:
    // do even if state.guard.is_false() to clear out frame created
    // in symex_start_thread
    symex_end_of_function(state);
    symex_transition(state);
    break;

  case LOCATION:
    if(!state.guard.is_false())
      target.location(state.guard.as_expr(), state.source);
    symex_transition(state);
    break;

  case GOTO:
    if(!state.guard.is_false())
      symex_goto(state);
    else
      symex_transition(state);
    break;

  case ASSUME:
    if(!state.guard.is_false())
      symex_assume(state, instruction.get_condition());
    symex_transition(state);
    break;

  case ASSERT:
    if(!state.guard.is_false())
      symex_assert(instruction, state);
    symex_transition(state);
    break;

  case RETURN:
    // This case should have been removed by return-value removal
    UNREACHABLE;
    break;

  case ASSIGN:
    if(!state.guard.is_false())
      symex_assign(state, instruction.get_assign());

    symex_transition(state);
    break;

  case FUNCTION_CALL:
    if(!state.guard.is_false())
    {
      symex_function_call(
        get_goto_function, state, instruction.get_function_call());
    }
    else
      symex_transition(state);
    break;

  case OTHER:
    if(!state.guard.is_false())
      symex_other(state);
    symex_transition(state);
    break;

  case DECL:
    if(!state.guard.is_false())
      symex_decl(state);
    symex_transition(state);
    break;

  case DEAD:
    symex_dead(state);
    symex_transition(state);
    break;

  case START_THREAD:
    symex_start_thread(state);
    symex_transition(state);
    break;

  case END_THREAD:
    // behaves like assume(0);
    if(!state.guard.is_false())
      state.guard.add(false_exprt());
    symex_transition(state);
    break;

  case ATOMIC_BEGIN:
    symex_atomic_begin(state);
    symex_transition(state);
    break;

  case ATOMIC_END:
    symex_atomic_end(state);
    symex_transition(state);
    break;

  case CATCH:
    symex_catch(state);
    symex_transition(state);
    break;

  case THROW:
    symex_throw(state);
    symex_transition(state);
    break;

  case NO_INSTRUCTION_TYPE:
    throw unsupported_operation_exceptiont("symex got NO_INSTRUCTION");

  default:
    UNREACHABLE;
  }
}
