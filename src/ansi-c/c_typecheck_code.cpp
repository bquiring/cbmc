/*******************************************************************\

Module: C Language Type Checking

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/// \file
/// C Language Type Checking

#include "c_typecheck_base.h"

#include <util/c_types.h>
#include <util/config.h>
#include <util/expr_initializer.h>
#include <util/range.h>

#include "ansi_c_declaration.h"

void c_typecheck_baset::start_typecheck_code()
{
  case_is_allowed=break_is_allowed=continue_is_allowed=false;
}

void c_typecheck_baset::typecheck_code(codet &code)
{
  if(code.id()!=ID_code)
  {
    error().source_location = code.source_location();
    error() << "expected code, got " << code.pretty() << eom;
    throw 0;
  }

  code.type() = empty_typet();

  const irep_idt &statement=code.get_statement();

  if(statement==ID_expression)
    typecheck_expression(code);
  else if(statement==ID_label)
    typecheck_label(to_code_label(code));
  else if(statement==ID_switch_case)
    typecheck_switch_case(to_code_switch_case(code));
  else if(statement==ID_gcc_switch_case_range)
    typecheck_gcc_switch_case_range(to_code_gcc_switch_case_range(code));
  else if(statement==ID_block)
    typecheck_block(to_code_block(code));
  else if(statement==ID_decl_block)
  {
  }
  else if(statement==ID_ifthenelse)
    typecheck_ifthenelse(to_code_ifthenelse(code));
  else if(statement==ID_while)
    typecheck_while(to_code_while(code));
  else if(statement==ID_dowhile)
    typecheck_dowhile(to_code_dowhile(code));
  else if(statement==ID_for)
    typecheck_for(code);
  else if(statement==ID_switch)
    typecheck_switch(code);
  else if(statement==ID_break)
    typecheck_break(code);
  else if(statement==ID_goto)
    typecheck_goto(to_code_goto(code));
  else if(statement==ID_gcc_computed_goto)
    typecheck_gcc_computed_goto(code);
  else if(statement==ID_continue)
    typecheck_continue(code);
  else if(statement==ID_return)
    typecheck_return(to_code_return(code));
  else if(statement==ID_decl)
    typecheck_decl(code);
  else if(statement==ID_assign)
    typecheck_assign(code);
  else if(statement==ID_skip)
  {
  }
  else if(statement==ID_asm)
    typecheck_asm(to_code_asm(code));
  else if(statement==ID_start_thread)
    typecheck_start_thread(code);
  else if(statement==ID_gcc_local_label)
    typecheck_gcc_local_label(code);
  else if(statement==ID_msc_try_finally)
  {
    assert(code.operands().size()==2);
    typecheck_code(to_code(code.op0()));
    typecheck_code(to_code(code.op1()));
  }
  else if(statement==ID_msc_try_except)
  {
    assert(code.operands().size()==3);
    typecheck_code(to_code(code.op0()));
    typecheck_expr(code.op1());
    typecheck_code(to_code(code.op2()));
  }
  else if(statement==ID_msc_leave)
  {
    // fine as is, but should check that we
    // are in a 'try' block
  }
  else if(statement==ID_static_assert)
  {
    assert(code.operands().size()==2);
    typecheck_expr(code.op0());
    typecheck_expr(code.op1());
  }
  else if(statement==ID_CPROVER_try_catch ||
          statement==ID_CPROVER_try_finally)
  {
    assert(code.operands().size()==2);
    typecheck_code(to_code(code.op0()));
    typecheck_code(to_code(code.op1()));
  }
  else if(statement==ID_CPROVER_throw)
  {
    assert(code.operands().empty());
  }
  else if(statement==ID_assume ||
          statement==ID_assert)
  {
    // These are not generated by the C/C++ parsers,
    // but we allow them for the benefit of other users
    // of the typechecker.
    assert(code.operands().size()==1);
    typecheck_expr(code.op0());
  }
  else
  {
    error().source_location = code.source_location();
    error() << "unexpected statement: " << statement << eom;
    throw 0;
  }
}

void c_typecheck_baset::typecheck_asm(code_asmt &code)
{
  const irep_idt flavor = code.get_flavor();

  if(flavor==ID_gcc)
  {
    // These have 5 operands.
    // The first operand is a string.
    // Operands 1, 2, 3, 4 are lists of expressions.

    // Operand 1: OutputOperands
    // Operand 2: InputOperands
    // Operand 3: Clobbers
    // Operand 4: GotoLabels

    auto &code_asm_gcc = to_code_asm_gcc(code);

    typecheck_expr(code_asm_gcc.asm_text());

    // the operands are lists of expressions
    for(auto &op : ranget<exprt::operandst::iterator>(
          code_asm_gcc.operands().begin() + 1, code_asm_gcc.operands().end()))
    {
      for(auto &expr : op.operands())
        typecheck_expr(expr);
    }
  }
  else if(flavor==ID_msc)
  {
    assert(code.operands().size()==1);
    typecheck_expr(code.op0());
  }
}

void c_typecheck_baset::typecheck_assign(codet &code)
{
  if(code.operands().size()!=2)
  {
    error().source_location = code.source_location();
    error() << "assignment statement expected to have two operands"
            << eom;
    throw 0;
  }

  typecheck_expr(code.op0());
  typecheck_expr(code.op1());

  implicit_typecast(code.op1(), code.op0().type());
}

void c_typecheck_baset::typecheck_block(code_blockt &code)
{
  for(auto &c : code.statements())
    typecheck_code(c);

  // do decl-blocks

  code_blockt new_ops;
  new_ops.statements().reserve(code.statements().size());

  for(auto &code_op : code.statements())
  {
    if(code_op.is_not_nil())
      new_ops.add(std::move(code_op));
  }

  code.statements().swap(new_ops.statements());
}

void c_typecheck_baset::typecheck_break(codet &code)
{
  if(!break_is_allowed)
  {
    error().source_location = code.source_location();
    error() << "break not allowed here" << eom;
    throw 0;
  }
}

void c_typecheck_baset::typecheck_continue(codet &code)
{
  if(!continue_is_allowed)
  {
    error().source_location = code.source_location();
    error() << "continue not allowed here" << eom;
    throw 0;
  }
}

void c_typecheck_baset::typecheck_decl(codet &code)
{
  // this comes with 1 operand, which is a declaration
  if(code.operands().size()!=1)
  {
    error().source_location = code.source_location();
    error() << "decl expected to have 1 operand" << eom;
    throw 0;
  }

  // op0 must be declaration
  if(code.op0().id()!=ID_declaration)
  {
    error().source_location = code.source_location();
    error() << "decl statement expected to have declaration as operand"
            << eom;
    throw 0;
  }

  ansi_c_declarationt declaration;
  declaration.swap(code.op0());

  if(declaration.get_is_static_assert())
  {
    assert(declaration.operands().size()==2);
    codet new_code(ID_static_assert);
    new_code.add_source_location()=code.source_location();
    new_code.operands().swap(declaration.operands());
    code.swap(new_code);
    typecheck_code(code);
    return; // done
  }

  typecheck_declaration(declaration);

  std::list<codet> new_code;

  // iterate over declarators

  for(const auto &d : declaration.declarators())
  {
    irep_idt identifier = d.get_name();

    // look it up
    symbol_tablet::symbolst::const_iterator s_it=
      symbol_table.symbols.find(identifier);

    if(s_it==symbol_table.symbols.end())
    {
      error().source_location = code.source_location();
      error() << "failed to find decl symbol '" << identifier
              << "' in symbol table" << eom;
      throw 0;
    }

    const symbolt &symbol=s_it->second;

    // This must not be an incomplete type, unless it's 'extern'
    // or a typedef.
    if(!symbol.is_type &&
       !symbol.is_extern &&
       !is_complete_type(symbol.type))
    {
      error().source_location=symbol.location;
      error() << "incomplete type not permitted here" << eom;
      throw 0;
    }

    // see if it's a typedef
    // or a function
    // or static
    if(symbol.is_type ||
       symbol.type.id()==ID_code ||
       symbol.is_static_lifetime)
    {
      // we ignore
    }
    else
    {
      code_declt decl(symbol.symbol_expr());
      decl.add_source_location() = symbol.location;
      decl.symbol().add_source_location() = symbol.location;

      // add initializer, if any
      if(symbol.value.is_not_nil())
      {
        decl.operands().resize(2);
        decl.op1() = symbol.value;
      }

      new_code.push_back(decl);
    }
  }

  // stash away any side-effects in the declaration
  new_code.splice(new_code.begin(), clean_code);

  if(new_code.empty())
  {
    source_locationt source_location=code.source_location();
    code=code_skipt();
    code.add_source_location()=source_location;
  }
  else if(new_code.size()==1)
  {
    code.swap(new_code.front());
  }
  else
  {
    // build a decl-block
    auto code_block=code_blockt::from_list(new_code);
    code_block.set_statement(ID_decl_block);
    code.swap(code_block);
  }
}

bool c_typecheck_baset::is_complete_type(const typet &type) const
{
  if(type.id() == ID_array)
  {
    if(to_array_type(type).size().is_nil())
      return false;
    return is_complete_type(type.subtype());
  }
  else if(type.id()==ID_struct || type.id()==ID_union)
  {
    const auto &struct_union_type = to_struct_union_type(type);

    if(struct_union_type.is_incomplete())
      return false;

    for(const auto &c : struct_union_type.components())
      if(!is_complete_type(c.type()))
        return false;
  }
  else if(type.id()==ID_vector)
    return is_complete_type(type.subtype());
  else if(type.id() == ID_struct_tag || type.id() == ID_union_tag)
  {
    return is_complete_type(follow(type));
  }

  return true;
}

void c_typecheck_baset::typecheck_expression(codet &code)
{
  if(code.operands().size()!=1)
  {
    error().source_location = code.source_location();
    error() << "expression statement expected to have one operand"
            << eom;
    throw 0;
  }

  exprt &op=code.op0();
  typecheck_expr(op);
}

void c_typecheck_baset::typecheck_for(codet &code)
{
  if(code.operands().size()!=4)
  {
    error().source_location = code.source_location();
    error() << "for expected to have four operands" << eom;
    throw 0;
  }

  // the "for" statement has an implicit block around it,
  // since code.op0() may contain declarations
  //
  // we therefore transform
  //
  //   for(a;b;c) d;
  //
  // to
  //
  //   { a; for(;b;c) d; }
  //
  // if config.ansi_c.for_has_scope

  if(!config.ansi_c.for_has_scope ||
     code.op0().is_nil())
  {
    if(code.op0().is_not_nil())
      typecheck_code(to_code(code.op0()));

    if(code.op1().is_nil())
      code.op1()=true_exprt();
    else
    {
      typecheck_expr(code.op1());
      implicit_typecast_bool(code.op1());
    }

    if(code.op2().is_not_nil())
      typecheck_expr(code.op2());

    if(code.op3().is_not_nil())
    {
      // save & set flags
      bool old_break_is_allowed=break_is_allowed;
      bool old_continue_is_allowed=continue_is_allowed;

      break_is_allowed=continue_is_allowed=true;

      // recursive call
      if(to_code(code.op3()).get_statement()==ID_decl_block)
      {
        code_blockt code_block;
        code_block.add_source_location()=code.op3().source_location();

        code_block.add(std::move(to_code(code.op3())));
        code.op3().swap(code_block);
      }
      typecheck_code(to_code(code.op3()));

      // restore flags
      break_is_allowed=old_break_is_allowed;
      continue_is_allowed=old_continue_is_allowed;
    }
  }
  else
  {
    code_blockt code_block;
    code_block.add_source_location()=code.source_location();
    if(to_code(code.op3()).get_statement()==ID_block)
    {
      code_block.set(
        ID_C_end_location,
        to_code_block(to_code(code.op3())).end_location());
    }
    else
    {
      code_block.set(
        ID_C_end_location,
        code.op3().source_location());
    }

    code_block.reserve_operands(2);
    code_block.add(std::move(to_code(code.op0())));
    code.op0().make_nil();
    code_block.add(std::move(code));
    code.swap(code_block);
    typecheck_code(code); // recursive call
  }

  typecheck_spec_expr(code, ID_C_spec_loop_invariant);
}

void c_typecheck_baset::typecheck_label(code_labelt &code)
{
  // record the label
  labels_defined[code.get_label()]=code.source_location();

  typecheck_code(code.code());
}

void c_typecheck_baset::typecheck_switch_case(code_switch_caset &code)
{
  if(code.operands().size()!=2)
  {
    error().source_location = code.source_location();
    error() << "switch_case expected to have two operands" << eom;
    throw 0;
  }

  typecheck_code(code.code());

  if(code.is_default())
  {
    if(!case_is_allowed)
    {
      error().source_location = code.source_location();
      error() << "did not expect default label here" << eom;
      throw 0;
    }
  }
  else
  {
    if(!case_is_allowed)
    {
      error().source_location = code.source_location();
      error() << "did not expect `case' here" << eom;
      throw 0;
    }

    exprt &case_expr=code.case_op();
    typecheck_expr(case_expr);
    implicit_typecast(case_expr, switch_op_type);
    make_constant(case_expr);
  }
}

void c_typecheck_baset::typecheck_gcc_switch_case_range(
  code_gcc_switch_case_ranget &code)
{
  if(!case_is_allowed)
  {
    error().source_location = code.source_location();
    error() << "did not expect `case' here" << eom;
    throw 0;
  }

  typecheck_expr(code.lower());
  typecheck_expr(code.upper());
  implicit_typecast(code.lower(), switch_op_type);
  implicit_typecast(code.upper(), switch_op_type);
  make_constant(code.lower());
  make_constant(code.upper());
  typecheck_code(code.code());
}

void c_typecheck_baset::typecheck_gcc_local_label(codet &)
{
  // these are just declarations, e.g.,
  // __label__ here, there;
}

void c_typecheck_baset::typecheck_goto(code_gotot &code)
{
  // we record the label used
  labels_used[code.get_destination()]=code.source_location();
}

void c_typecheck_baset::typecheck_gcc_computed_goto(codet &code)
{
  if(code.operands().size()!=1)
  {
    error().source_location = code.source_location();
    error() << "computed-goto expected to have one operand" << eom;
    throw 0;
  }

  exprt &dest=code.op0();

  if(dest.id()!=ID_dereference)
  {
    error().source_location = dest.source_location();
    error() << "computed-goto expected to have dereferencing operand"
            << eom;
    throw 0;
  }

  typecheck_expr(to_unary_expr(dest).op());
  dest.type() = void_type();
}

void c_typecheck_baset::typecheck_ifthenelse(code_ifthenelset &code)
{
  if(code.operands().size()!=3)
  {
    error().source_location = code.source_location();
    error() << "ifthenelse expected to have three operands" << eom;
    throw 0;
  }

  exprt &cond=code.cond();

  typecheck_expr(cond);

  #if 0
  if(cond.id()==ID_sideeffect &&
     cond.get(ID_statement)==ID_assign)
  {
    warning("warning: assignment in if condition");
  }
  #endif

  implicit_typecast_bool(cond);

  if(code.then_case().get_statement() == ID_decl_block)
  {
    code_blockt code_block({code.then_case()});
    code_block.add_source_location()=code.then_case().source_location();
    code.then_case() = code_block;
  }

  typecheck_code(code.then_case());

  if(!code.else_case().is_nil())
  {
    if(code.else_case().get_statement() == ID_decl_block)
    {
      code_blockt code_block({code.else_case()});
      code_block.add_source_location()=code.else_case().source_location();
      code.else_case() = code_block;
    }

    typecheck_code(code.else_case());
  }
}

void c_typecheck_baset::typecheck_start_thread(codet &code)
{
  if(code.operands().size()!=1)
  {
    error().source_location = code.source_location();
    error() << "start_thread expected to have one operand" << eom;
    throw 0;
  }

  typecheck_code(to_code(code.op0()));
}

void c_typecheck_baset::typecheck_return(code_returnt &code)
{
  if(code.has_return_value())
  {
    typecheck_expr(code.return_value());

    if(return_type.id() == ID_empty)
    {
      // gcc doesn't actually complain, it just warns!
      if(code.return_value().type().id() != ID_empty)
      {
        warning().source_location=code.source_location();

        warning() << "function has return void ";
        warning() << "but a return statement returning ";
        warning() << to_string(code.return_value().type());
        warning() << eom;

        code.return_value() = typecast_exprt(code.return_value(), return_type);
      }
    }
    else
      implicit_typecast(code.return_value(), return_type);
  }
  else if(
    return_type.id() != ID_empty && return_type.id() != ID_constructor &&
    return_type.id() != ID_destructor)
  {
    // gcc doesn't actually complain, it just warns!
    warning().source_location = code.source_location();
    warning() << "non-void function should return a value" << eom;

    code.return_value() =
      side_effect_expr_nondett(return_type, code.source_location());
  }
}

void c_typecheck_baset::typecheck_switch(codet &code)
{
  // we expect a code_switcht, but might return either a code_switcht or a
  // code_blockt, hence don't use code_switcht in the interface
  code_switcht &code_switch = to_code_switch(code);

  typecheck_expr(code_switch.value());

  // this needs to be promoted
  implicit_typecast_arithmetic(code_switch.value());

  // save & set flags

  bool old_case_is_allowed(case_is_allowed);
  bool old_break_is_allowed(break_is_allowed);
  typet old_switch_op_type(switch_op_type);

  switch_op_type = code_switch.value().type();
  break_is_allowed=case_is_allowed=true;

  typecheck_code(code_switch.body());

  if(code_switch.body().get_statement() == ID_block)
  {
    // Collect all declarations before the first case, if there is any case
    // (including a default one).
    code_blockt wrapping_block;

    code_blockt &body_block = to_code_block(code_switch.body());
    for(auto &statement : body_block.statements())
    {
      if(statement.get_statement() == ID_switch_case)
        break;
      else if(statement.get_statement() == ID_decl)
      {
        if(statement.operands().size() == 1)
        {
          wrapping_block.add(code_skipt());
          wrapping_block.statements().back().swap(statement);
        }
        else
        {
          PRECONDITION(statement.operands().size() == 2);
          wrapping_block.add(statement);
          wrapping_block.statements().back().operands().pop_back();
          statement.set_statement(ID_assign);
        }
      }
    }

    if(!wrapping_block.statements().empty())
    {
      wrapping_block.add(std::move(code));
      code.swap(wrapping_block);
    }
  }

  // restore flags
  case_is_allowed=old_case_is_allowed;
  break_is_allowed=old_break_is_allowed;
  switch_op_type=old_switch_op_type;
}

void c_typecheck_baset::typecheck_while(code_whilet &code)
{
  if(code.operands().size()!=2)
  {
    error().source_location = code.source_location();
    error() << "while expected to have two operands" << eom;
    throw 0;
  }

  typecheck_expr(code.cond());
  implicit_typecast_bool(code.cond());

  // save & set flags
  bool old_break_is_allowed(break_is_allowed);
  bool old_continue_is_allowed(continue_is_allowed);

  break_is_allowed=continue_is_allowed=true;

  if(code.body().get_statement()==ID_decl_block)
  {
    code_blockt code_block({code.body()});
    code_block.add_source_location()=code.body().source_location();
    code.body() = code_block;
  }
  typecheck_code(code.body());

  // restore flags
  break_is_allowed=old_break_is_allowed;
  continue_is_allowed=old_continue_is_allowed;

  typecheck_spec_expr(code, ID_C_spec_loop_invariant);
}

void c_typecheck_baset::typecheck_dowhile(code_dowhilet &code)
{
  if(code.operands().size()!=2)
  {
    error().source_location = code.source_location();
    error() << "do while expected to have two operands" << eom;
    throw 0;
  }

  typecheck_expr(code.cond());
  implicit_typecast_bool(code.cond());

  // save & set flags
  bool old_break_is_allowed(break_is_allowed);
  bool old_continue_is_allowed(continue_is_allowed);

  break_is_allowed=continue_is_allowed=true;

  if(code.body().get_statement()==ID_decl_block)
  {
    code_blockt code_block({code.body()});
    code_block.add_source_location()=code.body().source_location();
    code.body() = code_block;
  }
  typecheck_code(code.body());

  // restore flags
  break_is_allowed=old_break_is_allowed;
  continue_is_allowed=old_continue_is_allowed;

  typecheck_spec_expr(code, ID_C_spec_loop_invariant);
}

void c_typecheck_baset::typecheck_spec_expr(codet &code, const irep_idt &spec)
{
  if(code.find(spec).is_not_nil())
  {
    exprt &constraint = static_cast<exprt &>(code.add(spec));

    typecheck_expr(constraint);
    implicit_typecast_bool(constraint);
  }
}

void c_typecheck_baset::typecheck_assigns(
  const code_typet &function_declarator,
  const irept &contract)
{
  exprt assigns = static_cast<const exprt &>(contract.find(ID_C_spec_assigns));

  // Make sure there is an assigns clause to check
  if(assigns.is_not_nil())
  {
    for(const auto &curr_param : function_declarator.parameters())
    {
      if(curr_param.id() == ID_declaration)
      {
        const ansi_c_declarationt &param_declaration =
          to_ansi_c_declaration(curr_param);

        for(const auto &decl : param_declaration.declarators())
        {
          typecheck_assigns(decl, assigns);
        }
      }
    }
  }
}

void c_typecheck_baset::typecheck_assigns(
  const ansi_c_declaratort &declarator,
  const exprt &assigns)
{
  for(exprt curr_op : assigns.operands())
  {
    if(curr_op.id() != ID_symbol)
    {
      continue;
    }
    const symbol_exprt &symbol_op = to_symbol_expr(curr_op);

    if(symbol_op.get(ID_C_base_name) == declarator.get_base_name())
    {
      error().source_location = declarator.source_location();
      error() << "Formal parameter " << declarator.get_name() << " without "
              << "dereference appears in ASSIGNS clause. Assigning this "
              << "parameter will never affect the state of the calling context."
              << " Did you mean to write *" << declarator.get_name() << "?"
              << eom;
      throw 0;
    }
  }
}

void c_typecheck_baset::typecheck_assigns_exprs(
  codet &code,
  const irep_idt &spec)
{
  if(code.find(spec).is_not_nil())
  {
    exprt &constraint = static_cast<exprt &>(code.add(spec));

    typecheck_expr(constraint);
  }
}
