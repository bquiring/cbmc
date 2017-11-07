/*******************************************************************\

 Module: Generate Java Generic Type - Instantiate a generic class with
         concrete type information.

 Author: DiffBlue Limited. All rights reserved.

\*******************************************************************/
#ifndef CPROVER_JAVA_BYTECODE_GENERATE_JAVA_GENERIC_TYPE_H
#define CPROVER_JAVA_BYTECODE_GENERATE_JAVA_GENERIC_TYPE_H

#include <util/message.h>
#include <util/symbol_table.h>
#include <util/std_types.h>
#include <java_bytecode/java_types.h>

class generate_java_generic_typet
{
public:
  generate_java_generic_typet(
    message_handlert &message_handler);

  symbolt operator()(
    const java_generic_typet &existing_generic_type,
    symbol_tablet &symbol_table) const;
private:
  irep_idt build_generic_tag(
    const java_generic_typet &existing_generic_type,
    const java_class_typet &original_class) const;

  message_handlert &message_handler;
};

#endif // CPROVER_JAVA_BYTECODE_GENERATE_JAVA_GENERIC_TYPE_H
