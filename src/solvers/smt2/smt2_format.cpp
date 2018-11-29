/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include "smt2_format.h"

#include <util/arith_tools.h>
#include <util/std_expr.h>
#include <util/std_types.h>

std::ostream &smt2_format_rec(std::ostream &out, const typet &type)
{
  if(type.id() == ID_unsignedbv)
    out << "(_ BitVec " << to_unsignedbv_type(type).get_width() << ')';
  else if(type.id() == ID_bool)
    out << "Bool";
  else if(type.id() == ID_integer)
    out << "Int";
  else if(type.id() == ID_real)
    out << "Real";
  else
    out << "? " << type.id();

  return out;
}

std::ostream &smt2_format_rec(std::ostream &out, const exprt &expr)
{
  if(expr.id() == ID_constant)
  {
    const auto &value = to_constant_expr(expr).get_value();

    const typet &expr_type = expr.type();

    if(expr_type.id() == ID_unsignedbv)
    {
      const std::size_t width = to_unsignedbv_type(expr_type).get_width();

      const auto value = numeric_cast_v<mp_integer>(expr);

      out << "(_ bv" << value << " " << width << ")";
    }
    else if(expr_type.id() == ID_bool)
    {
      if(expr.is_true())
        out << "true";
      else if(expr.is_false())
        out << "false";
      else
        DATA_INVARIANT(false, "unknown Boolean constant");
    }
    else if(expr_type.id() == ID_integer)
    {
      out << value;
    }
    else if(expr_type.id() == ID_string)
    {
      out << '"';

      for(const auto &c : value)
      {
        // " is the only escape sequence
        if(c == '"')
          out << '"' << '"';
        else
          out << c;
      }

      out << '"';
    }
    else
      DATA_INVARIANT(false, "unhandled constant: " + expr_type.id_string());
  }
  else if(expr.id() == ID_symbol)
  {
    out << to_symbol_expr(expr).get_identifier();
  }
  else
    out << "? " << expr.id();

  return out;
}
