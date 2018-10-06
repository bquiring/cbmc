/*******************************************************************\

Module: C++ Language Conversion

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/// \file
/// C++ Language Conversion

#include "convert_float_literal.h"

#include <cassert>

#include <util/arith_tools.h>
#include <util/c_types.h>
#include <util/config.h>
#include <util/ieee_float.h>
#include <util/std_expr.h>
#include <util/std_types.h>
#include <util/string2int.h>

#include <ansi-c/gcc_types.h>

#include "parse_float.h"

/// build an expression from a floating-point literal
/// \returns either a constant_exprt or a complex_exprt
exprt convert_float_literal(const std::string &src)
{
  parse_floatt parsed_float(src);

  // In ANSI-C, float literals are double by default,
  // unless marked with 'f'.
  // All of these can be complex as well.
  // This can be overridden with
  // config.ansi_c.single_precision_constant.

  floatbv_typet type;

  if(parsed_float.is_float)
    type = float_type();
  else if(parsed_float.is_long)
    type = long_double_type();
  else if(parsed_float.is_float16)
    type = gcc_float16_type();
  else if(parsed_float.is_float32)
    type = gcc_float32_type();
  else if(parsed_float.is_float32x)
    type = gcc_float32x_type();
  else if(parsed_float.is_float64)
    type = gcc_float64_type();
  else if(parsed_float.is_float64x)
    type = gcc_float64x_type();
  else if(parsed_float.is_float80)
  {
    type = ieee_float_spect(64, 15).to_type();
    type.set(ID_C_c_type, ID_long_double);
  }
  else if(parsed_float.is_float128)
    type = gcc_float128_type();
  else if(parsed_float.is_float128x)
    type = gcc_float128x_type();
  else
  {
    // default
    if(config.ansi_c.single_precision_constant)
      type = float_type(); // default
    else
      type = double_type(); // default
  }

  if(parsed_float.is_decimal)
  {
    // TODO - should set ID_gcc_decimal32/ID_gcc_decimal64/ID_gcc_decimal128,
    // but these aren't handled anywhere
  }

  ieee_floatt a(type);

  if(parsed_float.exponent_base==10)
    a.from_base10(parsed_float.significand, parsed_float.exponent);
  else if(parsed_float.exponent_base==2) // hex
    a.build(parsed_float.significand, parsed_float.exponent);
  else
    UNREACHABLE;

  constant_exprt result(integer2bv(a.pack(), a.spec.width()), type);

  if(parsed_float.is_imaginary)
  {
    const complex_typet complex_type(type);
    return complex_exprt(from_integer(0, type), result, complex_type);
  }

  return result;
}
