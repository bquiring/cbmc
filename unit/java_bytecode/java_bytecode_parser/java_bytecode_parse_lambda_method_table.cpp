/*******************************************************************\

 Module: Unit tests for parsing generic classes

 Author: DiffBlue Limited. All rights reserved.

\*******************************************************************/

#include <algorithm>
#include <util/message.h>
#include <util/config.h>

#include <testing-utils/require_parse_tree.h>

#include <testing-utils/catch.hpp>
#include <java_bytecode/java_bytecode_parser.h>

#include <java_bytecode/java_bytecode_parse_tree.h>
#include <java_bytecode/java_types.h>

typedef java_bytecode_parse_treet::classt::lambda_method_handlet
  lambda_method_handlet;

SCENARIO(
  "lambda_method_handle_map with static lambdas",
  "[core][java_bytecode][java_bytecode_parse_lambda_method_handle]")
{
  null_message_handlert message_handler;
  GIVEN("A class with a static lambda variables")
  {
    java_bytecode_parse_treet parse_tree;
    java_bytecode_parse(
      "./java_bytecode/java_bytecode_parser/lambda_examples/"
      "StaticLambdas.class",
      parse_tree,
      message_handler);
    WHEN("Parsing that class")
    {
      REQUIRE(parse_tree.loading_successful);
      const java_bytecode_parse_treet::classt parsed_class =
        parse_tree.parsed_class;
      REQUIRE(parsed_class.attribute_bootstrapmethods_read);
      REQUIRE(parsed_class.lambda_method_handle_map.size() == 12);

      // Simple lambdas
      THEN(
        "There should be an entry for the lambda that has no parameters or "
        "returns and the method it references should have an appropriate "
        "descriptor")
      {
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, "()V");

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == "()V");
      }

      // Parameter lambdas
      THEN(
        "There should be an entry for the lambda that takes parameters and the "
        "method it references should have an appropriate descriptor")
      {
        std::string descriptor = "(ILjava/lang/Object;LDummyGeneric;)V";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);
      }
      THEN(
        "There should be an entry for the lambda that takes array parameters "
        "and the method it references should have an appropriate descriptor")
      {
        std::string descriptor = "([I[Ljava/lang/Object;[LDummyGeneric;)V";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);
      }

      // Return lambdas
      THEN(
        "There should be an entry for the lambda that returns a primitive and "
        "the method it references should have an appropriate descriptor")
      {
        std::string descriptor = "()I";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);
      }
      THEN(
        "There should be an entry for the lambda that returns a reference type "
        "and the method it references should have an appropriate descriptor")
      {
        std::string descriptor = "()Ljava/lang/Object;";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);
      }
      THEN(
        "There should be an entry for the lambda that returns a specialised "
        "generic type and the method it references should have an appropriate "
        "descriptor")
      {
        std::string descriptor = "()LDummyGeneric;";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);
      }

      // Array returning lambdas
      THEN(
        "There should be an entry for the lambda that returns an array of "
        "primitives and the method it references should have an appropriate "
        "descriptor")
      {
        std::string descriptor = "()[I";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);
      }
      THEN(
        "There should be an entry for the lambda that returns an array of "
        "reference types and the method it references should have an "
        "appropriate descriptor")
      {
        std::string descriptor = "()[Ljava/lang/Object;";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);
      }
      THEN(
        "There should be an entry for the lambda that returns an array of "
        "specialised generic types and the method it references should have an "
        "appropriate descriptor")
      {
        std::string descriptor = "()[LDummyGeneric;";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);
      }

      // Capturing lamdbas
      THEN(
        "There should be an entry for the lambda that returns a primitive and "
        "the method it references should have an appropriate descriptor")
      {
        std::string descriptor = "()I";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor, 1);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);

        const typet primitive_type = java_int_type();

        fieldref_exprt fieldref{
          primitive_type, "staticPrimitive", "java::StaticLambdas"};

        std::vector<require_parse_tree::expected_instructiont>
          expected_instructions{{"getstatic", {fieldref}}, {"ireturn", {}}};

        require_parse_tree::require_instructions_match_expectation(
          expected_instructions, lambda_method.instructions);
      }
      THEN(
        "There should be an entry for the lambda that returns a reference type "
        "and the method it references should have an appropriate descriptor")
      {
        std::string descriptor = "()Ljava/lang/Object;";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor, 1);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const auto lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);

        const reference_typet dummy_generic_reference_type =
          java_reference_type(symbol_typet{"java::java.lang.Object"});

        fieldref_exprt fieldref{dummy_generic_reference_type,
                                "staticReference",
                                "java::StaticLambdas"};

        std::vector<require_parse_tree::expected_instructiont>
          expected_instructions{{"getstatic", {fieldref}}, {"areturn", {}}};

        require_parse_tree::require_instructions_match_expectation(
          expected_instructions, lambda_method.instructions);
      }
      THEN(
        "There should be an entry for the lambda that returns a specialised "
        "generic type and the method it references should have an appropriate "
        "descriptor")
      {
        std::string descriptor = "()LDummyGeneric;";
        const lambda_method_handlet &lambda_entry =
          require_parse_tree::require_lambda_entry_for_descriptor(
            parsed_class, descriptor, 1);

        const irep_idt &lambda_impl_name = lambda_entry.lambda_method_name;

        const java_bytecode_parse_treet::methodt &lambda_method =
          require_parse_tree::require_method(parsed_class, lambda_impl_name);
        REQUIRE(id2string(lambda_method.descriptor) == descriptor);

        const reference_typet dummy_generic_reference_type =
          java_reference_type(symbol_typet{"java::DummyGeneric"});

        fieldref_exprt fieldref{dummy_generic_reference_type,
                                "staticSpecalisedGeneric",
                                "java::StaticLambdas"};

        std::vector<require_parse_tree::expected_instructiont>
          expected_instructions{{"getstatic", {fieldref}}, {"areturn", {}}};

        require_parse_tree::require_instructions_match_expectation(
          expected_instructions, lambda_method.instructions);
      }
    }
  }
}
