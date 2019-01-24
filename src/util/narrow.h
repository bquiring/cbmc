/*******************************************************************\

Module: Narrowing conversion functions

Author: Diffblue Ltd.

\*******************************************************************/

#ifndef CPROVER_UTIL_NARROW_H
#define CPROVER_UTIL_NARROW_H

#include <type_traits>

#include "invariant.h"

/// Alias for static_cast intended to be used for numeric casting
/// Rationale: Easier to grep than static_cast
template <typename output_type, typename input_type>
output_type narrow_cast(input_type value)
{
  static_assert(
    std::is_arithmetic<input_type>::value &&
      std::is_arithmetic<output_type>::value,
    "narrow_cast is intended only for numeric conversions");
  return static_cast<output_type>(value);
}

/// Run-time checked narrowing cast. Raises an invariant if the input
/// value cannot be converted to the output value without data loss
///
/// Template accepts a single argument - the return type. Input type
/// is deduced from the function argument and shouldn't be specified
template <typename output_type, typename input_type>
output_type narrow(input_type input)
{
  const auto output = static_cast<output_type>(input);
  INVARIANT(static_cast<input_type>(output) == input, "Data loss detected");
  return output;
}

#endif // CPROVER_UTIL_NARROW_H
