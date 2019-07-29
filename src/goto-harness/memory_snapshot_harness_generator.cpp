/******************************************************************\

Module: Harness to initialise memory from memory snapshot

Author: Daniel Poetzl

\******************************************************************/

#include <algorithm>

#include "memory_snapshot_harness_generator.h"
#include "memory_snapshot_harness_generator_options.h"

#include <goto-programs/goto_convert.h>

#include <json/json_parser.h>

#include <json-symtab-language/json_symbol_table.h>

#include <util/exception_utils.h>
#include <util/fresh_symbol.h>
#include <util/message.h>
#include <util/string2int.h>
#include <util/string_utils.h>
#include <util/symbol_table.h>

#include <linking/static_lifetime_init.h>

#include "goto_harness_generator_factory.h"

void memory_snapshot_harness_generatort::handle_option(
  const std::string &option,
  const std::list<std::string> &values)
{
  auto &require_exactly_one_value =
    harness_options_parser::require_exactly_one_value;
  if(recursive_initialization_config.handle_option(option, values))
  {
    // the option belongs to recursive initialization
  }
  else if(option == MEMORY_SNAPSHOT_HARNESS_TREAT_POINTER_AS_ARRAY_OPT)
  {
    recursive_initialization_config.pointers_to_treat_as_arrays.insert(
      values.begin(), values.end());
  }
  else if(option == MEMORY_SNAPSHOT_HARNESS_ASSOCIATED_ARRAY_SIZE_OPT)
  {
    for(auto const &array_size_pair : values)
    {
      try
      {
        std::string array;
        std::string size;
        split_string(array_size_pair, ':', array, size);
        // --associated-array-size implies --treat-pointer-as-array
        // but it is not an error to specify both, so we don't check
        // for duplicates here
        recursive_initialization_config.pointers_to_treat_as_arrays.insert(
          array);
        auto const inserted =
          recursive_initialization_config
            .array_name_to_associated_array_size_variable.emplace(array, size);
        if(!inserted.second)
        {
          throw invalid_command_line_argument_exceptiont{
            "can not have two associated array sizes for one array",
            "--" MEMORY_SNAPSHOT_HARNESS_ASSOCIATED_ARRAY_SIZE_OPT};
        }
      }
      catch(const deserialization_exceptiont &)
      {
        throw invalid_command_line_argument_exceptiont{
          "'" + array_size_pair +
            "' is in an invalid format for array size pair",
          "--" MEMORY_SNAPSHOT_HARNESS_ASSOCIATED_ARRAY_SIZE_OPT,
          "array_name:size_name, where both are the names of global "
          "variables"};
      }
    }
  }
  else if(option == MEMORY_SNAPSHOT_HARNESS_SNAPSHOT_OPT)
  {
    memory_snapshot_file = require_exactly_one_value(option, values);
  }
  else if(option == MEMORY_SNAPSHOT_HARNESS_INITIAL_GOTO_LOC_OPT)
  {
    initial_goto_location_line = require_exactly_one_value(option, values);
  }
  else if(option == MEMORY_SNAPSHOT_HARNESS_HAVOC_VARIABLES_OPT)
  {
    std::vector<std::string> havoc_candidates;
    split_string(values.front(), ',', havoc_candidates, true);
    for(const auto &candidate : havoc_candidates)
    {
      variables_to_havoc.insert(candidate);
    }
  }
  else if(option == MEMORY_SNAPSHOT_HARNESS_INITIAL_SOURCE_LOC_OPT)
  {
    initial_source_location_line = require_exactly_one_value(option, values);
  }
  else
  {
    throw invalid_command_line_argument_exceptiont(
      "unrecognized option for memory snapshot harness generator",
      "--" + option);
  }
}

void memory_snapshot_harness_generatort::validate_options(
  const goto_modelt &goto_model)
{
  if(memory_snapshot_file.empty())
  {
    throw invalid_command_line_argument_exceptiont(
      "option --memory_snapshot is required",
      "--harness-type initialise-from-memory-snapshot");
  }

  if(initial_source_location_line.empty() == initial_goto_location_line.empty())
  {
    throw invalid_command_line_argument_exceptiont(
      "choose either source or goto location to specify the entry point",
      "--initial-source/goto-location");
  }

  if(!initial_source_location_line.empty())
  {
    entry_location = initialize_entry_via_source(
      parse_source_location(initial_source_location_line),
      goto_model.goto_functions);
  }
  else
  {
    entry_location = initialize_entry_via_goto(
      parse_goto_location(initial_goto_location_line),
      goto_model.goto_functions);
  }

  const symbol_tablet &symbol_table = goto_model.symbol_table;

  const symbolt *called_function_symbol =
    symbol_table.lookup(entry_location.function_name);

  if(called_function_symbol == nullptr)
  {
    throw invalid_command_line_argument_exceptiont(
      "function `" + id2string(entry_location.function_name) +
        "` not found in the symbol table",
      "--initial-location");
  }
}

void memory_snapshot_harness_generatort::add_init_section(
  const symbol_exprt &func_init_done_var,
  goto_modelt &goto_model) const
{
  goto_functionst &goto_functions = goto_model.goto_functions;

  goto_functiont &goto_function =
    goto_functions.function_map[entry_location.function_name];

  goto_programt &goto_program = goto_function.body;

  const goto_programt::const_targett start_it =
    goto_program.instructions.begin();

  auto ins_it1 = goto_program.insert_before(
    start_it,
    goto_programt::make_goto(goto_program.const_cast_target(start_it)));
  ins_it1->guard = func_init_done_var;

  auto ins_it2 = goto_program.insert_after(
    ins_it1,
    goto_programt::make_assignment(
      code_assignt(func_init_done_var, true_exprt())));

  goto_program.compute_location_numbers();
  goto_program.insert_after(
    ins_it2,
    goto_programt::make_goto(
      goto_program.const_cast_target(entry_location.start_instruction)));
}

const symbolt &memory_snapshot_harness_generatort::fresh_symbol_copy(
  const symbolt &snapshot_symbol,
  symbol_tablet &symbol_table) const
{
  symbolt &tmp_symbol = get_fresh_aux_symbol(
    snapshot_symbol.type,
    "", // no prefix name
    id2string(snapshot_symbol.base_name),
    snapshot_symbol.location,
    snapshot_symbol.mode,
    symbol_table);
  tmp_symbol.is_static_lifetime = true;
  tmp_symbol.value = snapshot_symbol.value;

  return tmp_symbol;
}

size_t memory_snapshot_harness_generatort::pointer_depth(const typet &t) const
{
  if(t.id() != ID_pointer)
    return 0;
  else
    return pointer_depth(t.subtype()) + 1;
}

bool memory_snapshot_harness_generatort::refers_to(
  const exprt &expr,
  const irep_idt &name) const
{
  if(expr.id() == ID_symbol)
    return to_symbol_expr(expr).get_identifier() == name;
  return std::any_of(
    expr.operands().begin(),
    expr.operands().end(),
    [this, name](const exprt &subexpr) { return refers_to(subexpr, name); });
}

code_blockt memory_snapshot_harness_generatort::add_assignments_to_globals(
  const symbol_tablet &snapshot,
  goto_modelt &goto_model) const
{
  recursive_initializationt recursive_initialization{
    recursive_initialization_config, goto_model};

  using snapshot_pairt = std::pair<irep_idt, symbolt>;
  std::vector<snapshot_pairt> ordered_snapshot_symbols;
  for(auto pair : snapshot)
  {
    const auto name = id2string(pair.first);
    if(name.find(CPROVER_PREFIX) != 0)
      ordered_snapshot_symbols.push_back(pair);
  }

  // sort the snapshot symbols so that the non-pointer symbols are first, then
  // pointers, then pointers-to-pointers, etc. so that we don't assign
  // uninitialized values
  std::stable_sort(
    ordered_snapshot_symbols.begin(),
    ordered_snapshot_symbols.end(),
    [this](const snapshot_pairt &left, const snapshot_pairt &right) {
      if(refers_to(right.second.value, left.first))
        return true;
      else
        return pointer_depth(left.second.symbol_expr().type()) <
               pointer_depth(right.second.symbol_expr().type());
    });

  code_blockt code;
  for(const auto &pair : ordered_snapshot_symbols)
  {
    const symbolt &snapshot_symbol = pair.second;
    symbol_tablet &symbol_table = goto_model.symbol_table;

    auto should_get_fresh = [&symbol_table](const symbolt &symbol) {
      return symbol_table.lookup(symbol.base_name) == nullptr &&
             !symbol.is_type;
    };
    const symbolt &fresh_or_snapshot_symbol =
      should_get_fresh(snapshot_symbol)
        ? fresh_symbol_copy(snapshot_symbol, symbol_table)
        : snapshot_symbol;

    if(!fresh_or_snapshot_symbol.is_static_lifetime)
      continue;

    if(variables_to_havoc.count(fresh_or_snapshot_symbol.base_name) == 0)
    {
      code.add(code_assignt{fresh_or_snapshot_symbol.symbol_expr(),
                            fresh_or_snapshot_symbol.value});
    }
    else
    {
      recursive_initialization.initialize(
        fresh_or_snapshot_symbol.symbol_expr(), 0, {}, code);
    }
  }
  return code;
}

void memory_snapshot_harness_generatort::add_call_with_nondet_arguments(
  const symbolt &called_function_symbol,
  code_blockt &code) const
{
  const code_typet &code_type = to_code_type(called_function_symbol.type);

  code_function_callt::argumentst arguments;

  for(const code_typet::parametert &parameter : code_type.parameters())
  {
    arguments.push_back(side_effect_expr_nondett{
      parameter.type(), called_function_symbol.location});
  }

  code.add(code_function_callt{called_function_symbol.symbol_expr(),
                               std::move(arguments)});
}

void memory_snapshot_harness_generatort::
  insert_harness_function_into_goto_model(
    goto_modelt &goto_model,
    const symbolt &function) const
{
  const auto r = goto_model.symbol_table.insert(function);
  CHECK_RETURN(r.second);

  auto function_iterator_pair = goto_model.goto_functions.function_map.emplace(
    function.name, goto_functiont{});

  CHECK_RETURN(function_iterator_pair.second);

  goto_functiont &harness_function = function_iterator_pair.first->second;
  harness_function.type = to_code_type(function.type);

  goto_convert(
    to_code_block(to_code(function.value)),
    goto_model.symbol_table,
    harness_function.body,
    message_handler,
    function.mode);

  harness_function.body.add(goto_programt::make_end_function());
}

void memory_snapshot_harness_generatort::get_memory_snapshot(
  const std::string &file,
  symbol_tablet &snapshot) const
{
  jsont json;

  const bool r = parse_json(memory_snapshot_file, message_handler, json);

  if(r)
  {
    throw deserialization_exceptiont("failed to read JSON memory snapshot");
  }

  if(json.is_array())
  {
    // since memory-analyzer produces an array JSON we need to search it
    // to find the first JSON object that is a symbol table
    const auto &jarr = to_json_array(json);
    for(auto const &arr_element : jarr)
    {
      if(!arr_element.is_object())
        continue;
      const auto &json_obj = to_json_object(arr_element);
      const auto it = json_obj.find("symbolTable");
      if(it != json_obj.end())
      {
        symbol_table_from_json(json_obj, snapshot);
        return;
      }
    }
    throw deserialization_exceptiont(
      "JSON memory snapshot does not contain symbol table");
  }
  else
  {
    // throws a deserialization_exceptiont or an
    // incorrect_goto_program_exceptiont
    // on failure to read JSON symbol table
    symbol_table_from_json(json, snapshot);
  }
}

void memory_snapshot_harness_generatort::generate(
  goto_modelt &goto_model,
  const irep_idt &harness_function_name)
{
  symbol_tablet snapshot;
  get_memory_snapshot(memory_snapshot_file, snapshot);

  symbol_tablet &symbol_table = goto_model.symbol_table;
  goto_functionst &goto_functions = goto_model.goto_functions;

  const symbolt *called_function_symbol =
    symbol_table.lookup(entry_location.function_name);

  // introduce a symbol for a Boolean variable to indicate the point at which
  // the function initialisation is completed
  auto &func_init_done_symbol = get_fresh_aux_symbol(
    bool_typet(),
    id2string(entry_location.function_name),
    "func_init_done",
    source_locationt::nil(),
    called_function_symbol->mode,
    symbol_table);
  func_init_done_symbol.is_static_lifetime = true;
  func_init_done_symbol.value = false_exprt();
  symbol_exprt func_init_done_var = func_init_done_symbol.symbol_expr();

  add_init_section(func_init_done_var, goto_model);

  code_blockt harness_function_body =
    add_assignments_to_globals(snapshot, goto_model);

  harness_function_body.add(code_assignt{func_init_done_var, false_exprt{}});

  add_call_with_nondet_arguments(
    *called_function_symbol, harness_function_body);

  // Create harness function symbol

  symbolt harness_function_symbol;
  harness_function_symbol.name = harness_function_name;
  harness_function_symbol.base_name = harness_function_name;
  harness_function_symbol.pretty_name = harness_function_name;

  harness_function_symbol.is_lvalue = true;
  harness_function_symbol.mode = called_function_symbol->mode;
  harness_function_symbol.type = code_typet({}, empty_typet());
  harness_function_symbol.value = harness_function_body;

  // Add harness function to goto model and symbol table
  insert_harness_function_into_goto_model(goto_model, harness_function_symbol);

  goto_functions.update();
}

memory_snapshot_harness_generatort::entry_goto_locationt
memory_snapshot_harness_generatort::parse_goto_location(
  const std::string &cmdl_option)
{
  std::vector<std::string> start;
  split_string(cmdl_option, ':', start, true);

  if(
    start.empty() || start.front().empty() ||
    (start.size() == 2 && start.back().empty()) || start.size() > 2)
  {
    throw invalid_command_line_argument_exceptiont(
      "invalid initial location specification", "--initial-location");
  }

  if(start.size() == 2)
  {
    const auto location_number = string2optional_unsigned(start.back());
    CHECK_RETURN(location_number.has_value());
    return entry_goto_locationt{start.front(), *location_number};
  }
  else
  {
    return entry_goto_locationt{start.front()};
  }
}

memory_snapshot_harness_generatort::entry_source_locationt
memory_snapshot_harness_generatort::parse_source_location(
  const std::string &cmdl_option)
{
  std::string initial_file_string;
  std::string initial_line_string;
  split_string(
    cmdl_option, ':', initial_file_string, initial_line_string, true);

  if(initial_file_string.empty() || initial_line_string.empty())
  {
    throw invalid_command_line_argument_exceptiont(
      "invalid initial location specification", "--initial-file-line");
  }

  const auto line_number = string2optional_unsigned(initial_line_string);
  CHECK_RETURN(line_number.has_value());
  return entry_source_locationt{initial_file_string, *line_number};
}

memory_snapshot_harness_generatort::entry_locationt
memory_snapshot_harness_generatort::initialize_entry_via_goto(
  const entry_goto_locationt &entry_goto_location,
  const goto_functionst &goto_functions)
{
  PRECONDITION(!entry_goto_location.function_name.empty());
  const irep_idt &function_name = entry_goto_location.function_name;

  // by function(+location): search for the function then jump to n-th
  // location, then check the number
  const auto &goto_function =
    goto_functions.function_map.find(entry_goto_location.function_name);
  if(
    goto_function != goto_functions.function_map.end() &&
    goto_function->second.body_available())
  {
    const auto &goto_program = goto_function->second.body;

    const auto corresponding_instruction =
      entry_goto_location.find_first_corresponding_instruction(
        goto_program.instructions);

    if(corresponding_instruction != goto_program.instructions.end())
      return entry_locationt{function_name, corresponding_instruction};
  }
  throw invalid_command_line_argument_exceptiont(
    "could not find the specified entry point", "--initial-goto-location");
}

memory_snapshot_harness_generatort::entry_locationt
memory_snapshot_harness_generatort::initialize_entry_via_source(
  const entry_source_locationt &entry_source_location,
  const goto_functionst &goto_functions)
{
  PRECONDITION(!entry_source_location.file_name.empty());

  // by line: iterate over all instructions until source location match
  for(const auto &entry : goto_functions.function_map)
  {
    const auto &goto_function = entry.second;
    // if !body_available() then body.instruction.empty() and that's fine
    const auto &goto_program = goto_function.body;

    const auto corresponding_instruction =
      entry_source_location.find_first_corresponding_instruction(
        goto_program.instructions);

    if(corresponding_instruction != goto_program.instructions.end())
      return entry_locationt{entry.first, corresponding_instruction};
  }
  throw invalid_command_line_argument_exceptiont(
    "could not find the specified entry point", "--initial-source-location");
}

goto_programt::const_targett memory_snapshot_harness_generatort::
  entry_goto_locationt::find_first_corresponding_instruction(
    const goto_programt::instructionst &instructions) const
{
  if(!location_number.has_value())
    return instructions.begin();

  return std::find_if(
    instructions.begin(),
    instructions.end(),
    [this](const goto_programt::instructiont &instruction) {
      return *location_number == instruction.location_number;
    });
}

goto_programt::const_targett memory_snapshot_harness_generatort::
  entry_source_locationt::find_first_corresponding_instruction(
    const goto_programt::instructionst &instructions) const
{
  return std::find_if(
    instructions.begin(),
    instructions.end(),
    [this](const goto_programt::instructiont &instruction) {
      return instruction.source_location.get_file() == file_name &&
             safe_string2unsigned(id2string(
               instruction.source_location.get_line())) >= line_number;
    });
}
