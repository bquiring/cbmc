/*******************************************************************\

 Module: Java Bytecode Language Conversion

 Author: Diffblue Ltd.

\*******************************************************************/
#ifndef CPROVER_JAVA_BYTECODE_SELECT_POINTER_TYPE_H
#define CPROVER_JAVA_BYTECODE_SELECT_POINTER_TYPE_H

/// \file
/// Handle selection of correct pointer type (for example changing abstract
/// classes to concrete versions).
#include <vector>

#include "java_types.h"
#include <util/optional.h>
#include <util/std_types.h>

typedef std::unordered_map<irep_idt, std::vector<reference_typet>>
  generic_parameter_specialization_mapt;
typedef std::set<irep_idt> generic_parameter_recursion_trackingt;

class namespacet;

class select_pointer_typet
{
  optionalt<pointer_typet> get_recursively_instantiated_type(
    const irep_idt &,
    const generic_parameter_specialization_mapt &,
    generic_parameter_recursion_trackingt &,
    const size_t) const;
  optionalt<pointer_typet> get_recursively_instantiated_type(
    const irep_idt &parameter_name,
    const generic_parameter_specialization_mapt &visited) const;

public:
  virtual ~select_pointer_typet() = default;

  /// Select what type should be used for a given pointer type. For the base
  /// class we just use the supplied type. Derived classes can override this
  /// behaviour to provide more sophisticated type selection. Generic
  /// parameters are replaced with their concrete type.
  /// \param pointer_type: The pointer type replace
  /// \param generic_parameter_specialization_map map of types for all generic
  /// parameters in the current scope
  /// \param ns Namespace for type lookups
  /// \return A pointer type where the subtype may have been modified
  virtual pointer_typet convert_pointer_type(
    const pointer_typet &pointer_type,
    const generic_parameter_specialization_mapt
      &generic_parameter_specialization_map,
    const namespacet &ns) const;

  virtual std::set<symbol_typet> get_parameter_alternative_types(
    const irep_idt &function_name,
    const irep_idt &parameter_name,
    const namespacet &ns) const;

  /// Specialize generic parameters in a pointer type based on the current map
  /// of parameters -> types. We specialize generics if the pointer is a java
  /// generic parameter or an array with generic parameters (java generic types
  /// are specialized recursively, their concrete types are already stored in
  /// the map and will be retrieved when needed e.g., to initialize fields).
  /// Example:
  /// - generic type: T
  /// - map: T -> U; U -> String
  /// - result: String
  ///
  /// - generic type: T[]
  /// - map: T -> U; U -> String
  /// - result: String
  /// \param pointer_type pointer to be specialized
  /// \param generic_parameter_specialization_map map of types for all generic
  /// parameters in the current scope
  /// \return pointer type where generic parameters are replaced with concrete
  /// types, if set in the current scope
  pointer_typet specialize_generics(
    const pointer_typet &pointer_type,
    const generic_parameter_specialization_mapt
      &generic_parameter_specialization_map,
    generic_parameter_recursion_trackingt &visited) const;
};

#endif // CPROVER_JAVA_BYTECODE_SELECT_POINTER_TYPE_H
