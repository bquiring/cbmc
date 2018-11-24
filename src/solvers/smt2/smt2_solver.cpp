/*******************************************************************\

Module: SMT2 Solver that uses boolbv and the default SAT solver

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include "smt2_parser.h"

#include "smt2_format.h"

#include <fstream>
#include <iostream>

#include <util/message.h>
#include <util/namespace.h>
#include <util/replace_symbol.h>
#include <util/simplify_expr.h>
#include <util/symbol_table.h>

#include <solvers/sat/satcheck.h>
#include <solvers/flattening/boolbv.h>

class smt2_solvert:public smt2_parsert
{
public:
  smt2_solvert(std::istream &_in, decision_proceduret &_solver)
    : smt2_parsert(_in), solver(_solver), status(NOT_SOLVED)
  {
  }

protected:
  decision_proceduret &solver;

  void command(const std::string &) override;
  void define_constants(const exprt &);
  void expand_function_applications(exprt &);

  std::set<irep_idt> constants_done;

  enum
  {
    NOT_SOLVED,
    SAT,
    UNSAT
  } status;
};

void smt2_solvert::define_constants(const exprt &expr)
{
  for(const auto &op : expr.operands())
    define_constants(op);

  if(expr.id()==ID_symbol)
  {
    irep_idt identifier=to_symbol_expr(expr).get_identifier();

    // already done?
    if(constants_done.find(identifier)!=constants_done.end())
      return;

    constants_done.insert(identifier);

    auto f_it=id_map.find(identifier);

    if(f_it!=id_map.end())
    {
      const auto &f=f_it->second;

      if(f.type.id()!=ID_mathematical_function &&
         f.definition.is_not_nil())
      {
        exprt def=f.definition;
        expand_function_applications(def);
        define_constants(def); // recursive!
        solver.set_to_true(equal_exprt(expr, def));
      }
    }
  }
}

void smt2_solvert::expand_function_applications(exprt &expr)
{
  for(exprt &op : expr.operands())
    expand_function_applications(op);

  if(expr.id()==ID_function_application)
  {
    auto &app=to_function_application_expr(expr);

    // look it up
    irep_idt identifier=app.function().get_identifier();
    auto f_it=id_map.find(identifier);

    if(f_it!=id_map.end())
    {
      const auto &f=f_it->second;

      DATA_INVARIANT(f.type.id()==ID_mathematical_function,
        "type of function symbol must be mathematical_function_type");

      const auto f_type=
        to_mathematical_function_type(f.type);

      DATA_INVARIANT(f_type.domain().size()==
                     app.arguments().size(),
                     "number of function parameters");

      replace_symbolt replace_symbol;

      std::map<irep_idt, exprt> parameter_map;
      for(std::size_t i=0; i<f_type.domain().size(); i++)
      {
        const auto &var = f_type.domain()[i];
        const symbol_exprt s(var.get_identifier(), var.type());
        replace_symbol.insert(s, app.arguments()[i]);
      }

      exprt body=f.definition;
      replace_symbol(body);
      expand_function_applications(body);
      expr=body;
    }
  }
}

void smt2_solvert::command(const std::string &c)
{
  try
  {
    if(c=="assert")
    {
      exprt e=expression();
      if(e.is_not_nil())
      {
        expand_function_applications(e);
        define_constants(e);
        solver.set_to_true(e);
      }
    }
    else if(c=="check-sat")
    {
      switch(solver())
      {
      case decision_proceduret::resultt::D_SATISFIABLE:
        std::cout << "sat\n";
        status = SAT;
        break;

      case decision_proceduret::resultt::D_UNSATISFIABLE:
        std::cout << "unsat\n";
        status = UNSAT;
        break;

      case decision_proceduret::resultt::D_ERROR:
        std::cout << "error\n";
        status = NOT_SOLVED;
      }
    }
    else if(c=="check-sat-assuming")
    {
      std::cout << "not yet implemented\n";
    }
    else if(c=="display")
    {
      // this is a command that Z3 appears to implement
      exprt e=expression();
      if(e.is_not_nil())
      {
        std::cout << e.pretty() << '\n'; // need to do an 'smt2_format'
      }
    }
    else if(c == "get-value")
    {
      std::vector<exprt> ops;

      if(next_token() != OPEN)
        throw "get-value expects list as argument";

      while(peek() != CLOSE && peek() != END_OF_FILE)
        ops.push_back(expression()); // any term

      if(next_token() != CLOSE)
        throw "get-value expects ')' at end of list";

      if(status != SAT)
        throw "model is not available";

      std::vector<exprt> values;
      values.reserve(ops.size());

      for(const auto &op : ops)
      {
        if(op.id() != ID_symbol)
          throw "get-value expects symbol";

        const auto &identifier = to_symbol_expr(op).get_identifier();

        const auto id_map_it = id_map.find(identifier);

        if(id_map_it == id_map.end())
          throw "unexpected symbol " + id2string(identifier);

        exprt value;

        if(id_map_it->second.definition.is_nil())
          value = solver.get(op);
        else
          value = solver.get(id_map_it->second.definition);

        if(value.is_nil())
          throw "no value for " + id2string(identifier);

        values.push_back(value);
      }

      std::cout << '(';

      for(std::size_t op_nr = 0; op_nr < ops.size(); op_nr++)
      {
        if(op_nr != 0)
          std::cout << "\n ";

        std::cout << '(' << smt2_format(ops[op_nr]) << ' '
                  << smt2_format(values[op_nr]) << ')';
      }

      std::cout << ")\n";
    }
    else if(c=="simplify")
    {
      // this is a command that Z3 appears to implement
      exprt e=expression();
      if(e.is_not_nil())
      {
        const symbol_tablet symbol_table;
        const namespacet ns(symbol_table);
        exprt e_simplified=simplify_expr(e, ns);
        std::cout << e.pretty() << '\n'; // need to do an 'smt2_format'
      }
    }
    #if 0
    // TODO:
    | ( declare-const hsymboli hsorti )
    | ( declare-datatype hsymboli hdatatype_deci)
    | ( declare-datatypes ( hsort_deci n+1 ) ( hdatatype_deci n+1 ) )
    | ( declare-fun hsymboli ( hsorti ??? ) hsorti )
    | ( declare-sort hsymboli hnumerali )
    | ( define-fun hfunction_def i )
    | ( define-fun-rec hfunction_def i )
    | ( define-funs-rec ( hfunction_deci n+1 ) ( htermi n+1 ) )
    | ( define-sort hsymboli ( hsymboli ??? ) hsorti )
    | ( echo hstringi )
    | ( exit )
    | ( get-assertions )
    | ( get-assignment )
    | ( get-info hinfo_flag i )
    | ( get-model )
    | ( get-option hkeywordi )
    | ( get-proof )
    | ( get-unsat-assumptions )
    | ( get-unsat-core )
    | ( pop hnumerali )
    | ( push hnumerali )
    | ( reset )
    | ( reset-assertions )
    | ( set-info hattributei )
    | ( set-option hoptioni )
    #endif  
    else
      smt2_parsert::command(c);
  }
  catch(const char *error)
  {
    std::cout << "error: " << error << '\n';
  }
  catch(const std::string &error)
  {
    std::cout << "error: " << error << '\n';
  }
}

class smt2_message_handlert : public message_handlert
{
public:
  void print(unsigned level, const std::string &message) override
  {
    message_handlert::print(level, message);

    if(level < 4) // errors
      std::cout << "(error \"" << message << "\")\n";
    else
      std::cout << "; " << message << '\n';
  }

  void print(unsigned level, const xmlt &xml) override
  {
  }

  void print(unsigned level, const jsont &json) override
  {
  }

  void flush(unsigned) override
  {
    std::cout << std::flush;
  }
};

int solver(std::istream &in)
{
  symbol_tablet symbol_table;
  namespacet ns(symbol_table);

  smt2_message_handlert message_handler;
  messaget message(message_handler);

  // this is our default verbosity
  message_handler.set_verbosity(messaget::M_STATISTICS);

  satcheckt satcheck;
  boolbvt boolbv(ns, satcheck);
  satcheck.set_message_handler(message_handler);
  boolbv.set_message_handler(message_handler);

  smt2_solvert smt2_solver(in, boolbv);
  smt2_solver.set_message_handler(message_handler);

  smt2_solver.parse();

  if(!smt2_solver)
    return 20;
  else
    return 0;
}

int main(int argc, const char *argv[])
{
  if(argc==1)
    return solver(std::cin);

  if(argc!=2)
  {
    std::cerr << "usage: smt2_solver file\n";
    return 1;
  }

  std::ifstream in(argv[1]);
  if(!in)
  {
    std::cerr << "failed to open " << argv[1] << '\n';
    return 1;
  }

  return solver(in);
}
