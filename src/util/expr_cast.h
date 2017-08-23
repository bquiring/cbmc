/*
  Author: Nathan Phillips <Nathan.Phillips@diffblue.com>
*/

#ifndef CPROVER_UTIL_EXPR_CAST_H
#define CPROVER_UTIL_EXPR_CAST_H

/// \file util/expr_cast.h
/// \brief Templated functions to cast to specific exprt-derived classes

#include <typeinfo>
#include "invariant.h"
#include "expr.h"


/// \brief Check whether a reference to a generic \ref exprt is of a specific derived class
///   Implement template specializations of this function to enable casting
/// \tparam T The exprt-derived class to check for
/// \param base Reference to a generic \ref exprt
/// \return true if \a base is of type \a T
template<typename T> bool check_expr_type(const exprt &base);

/// \brief Check whether a reference to a generic \ref exprt is of a specific derived class
///   Implement template specializations of this function to enable casting
/// \tparam T The exprt-derived class to check for
/// \param base Reference to a generic \ref exprt
/// \return true if \a base is of type \a T
template<typename T> void validate_expr(const T &value) { }

template<typename T> struct remove_constt;
template<typename T> struct remove_constt<const T> { using type=T; };
template<typename T> struct ptr_typet;
template<typename T> struct ptr_typet<T *> { using type=T; };
template<typename T> struct ref_typet;
template<typename T> struct ref_typet<T &> { using type=T; };


/// \brief Cast a constant pointer to a generic exprt to a specific derived class
/// \tparam T The exprt-derived class to cast to
/// \param base Pointer to a generic \ref exprt
/// \return Pointer to object of type \a T or null if \a base is not an instance of \a T
template<typename T>
T expr_dynamic_cast(const exprt *base)
{
  return expr_dynamic_cast<
    T,
    typename remove_constt<typename ptr_typet<T>::type>::type,
    const exprt>(base);
}

/// \brief Cast a pointer to a generic exprt to a specific derived class
/// \tparam T The exprt-derived class to cast to
/// \param base Pointer to a generic \ref exprt
/// \return Pointer to object of type \a T or null if \a base is not an instance of \a T
template<typename T>
T expr_dynamic_cast(exprt *base)
{
  return expr_dynamic_cast<T, typename ptr_typet<T>::type, exprt>(base);
}

/// \brief Cast a pointer to a generic exprt to a specific derived class
/// \tparam T The pointer or const pointer type to \a TUnderlying to cast to
/// \tparam TUnderlying An exprt-derived class type
/// \param base Pointer to a generic \ref exprt
/// \return Pointer to object of type \a TUnderlying
///   or null if \a base is not an instance of \a TUnderlying
template<typename T, typename TUnderlying, typename TExpr>
T expr_dynamic_cast(TExpr *base)
{
  static_assert(
    std::is_base_of<exprt, TUnderlying>::value,
    "The template argument T must be derived from exprt.");
  if(base == nullptr)
    return nullptr;
  if(!check_expr_type<TUnderlying>(*base))
    return nullptr;
  T value=static_cast<T>(base);
  validate_expr<TUnderlying>(*value);
  return value;
}

/// \brief Cast a constant reference to a generic exprt to a specific derived class
/// \tparam T The exprt-derived class to cast to
/// \param base Reference to a generic \ref exprt
/// \return Reference to object of type \a T
/// \throw std::bad_cast If \a base is not an instance of \a T
template<typename T>
T expr_dynamic_cast(const exprt &base)
{
  return expr_dynamic_cast<
    T,
    typename remove_constt<typename ref_typet<T>::type>::type,
    const exprt>(base);
}

/// \brief Cast a reference to a generic exprt to a specific derived class
/// \tparam T The exprt-derived class to cast to
/// \param base Reference to a generic \ref exprt
/// \return Reference to object of type \a T
/// \throw std::bad_cast If \a base is not an instance of \a T
template<typename T>
T expr_dynamic_cast(exprt &base)
{
  return expr_dynamic_cast<T, typename ref_typet<T>::type, exprt>(base);
}

/// \brief Cast a reference to a generic exprt to a specific derived class
/// \tparam T The reference or const reference type to \a TUnderlying to cast to
/// \tparam TUnderlying An exprt-derived class type
/// \param base Reference to a generic \ref exprt
/// \return Reference to object of type \a T
/// \throw std::bad_cast If \a base is not an instance of \a TUnderlying
template<typename T, typename TUnderlying, typename TExpr>
T expr_dynamic_cast(TExpr &base)
{
  static_assert(
    std::is_base_of<exprt, TUnderlying>::value,
    "The template argument T must be derived from exprt.");
  if(!check_expr_type<TUnderlying>(base))
    throw std::bad_cast();
  T value=static_cast<T>(base);
  validate_expr<TUnderlying>(value);
  return value;
}


inline void validate_operands(
  const exprt &value,
  exprt::operandst::size_type number,
  const char *message,
  bool allowMore=false)
{
  DATA_INVARIANT(
    allowMore
      ? value.operands().size()==number
      : value.operands().size()>=number,
    message);
}

#endif  // CPROVER_UTIL_EXPR_CAST_H
