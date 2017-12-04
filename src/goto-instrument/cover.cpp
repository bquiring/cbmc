/*******************************************************************\

Module: Coverage Instrumentation

Author: Daniel Kroening

Date: May 2016

\*******************************************************************/

/// \file
/// Coverage Instrumentation

#include "cover.h"

#include <util/config.h>
#include <util/message.h>
#include <util/make_unique.h>

#include "cover_basic_blocks.h"
#include "cover_filter.h"
#include "cover_instrument.h"

/// Applies instrumenters to given goto program
/// \param goto_program: the goto program
/// \param instrumenters: the instrumenters
/// \param message_handler: a message handler
void instrument_cover_goals(
  goto_programt &goto_program,
  const cover_instrumenterst &instrumenters,
  message_handlert &message_handler)
{
  cover_basic_blockst basic_blocks(goto_program);
  basic_blocks.select_unique_java_bytecode_indices(
    goto_program, message_handler);
  basic_blocks.report_block_anomalies(goto_program, message_handler);

  instrumenters(goto_program, basic_blocks);
}

/// Instruments goto program for a given coverage criterion
/// \param symbol_table: the symbol table
/// \param goto_program: the goto program
/// \param criterion: the coverage criterion
/// \param message_handler: a message handler
void instrument_cover_goals(
  const symbol_tablet &symbol_table,
  goto_programt &goto_program,
  coverage_criteriont criterion,
  message_handlert &message_handler)
{
  goal_filterst goal_filters;
  goal_filters.add(util_make_unique<internal_goals_filtert>(message_handler));

  cover_instrumenterst instrumenters;
  instrumenters.add_from_criterion(criterion, symbol_table, goal_filters);

  instrument_cover_goals(goto_program, instrumenters, message_handler);
}

/// Create and add an instrumenter based on the given criterion
/// \param criterion: the coverage criterion
/// \param symbol_table: the symbol table
/// \param goal_filters: goal filters to discard certain goals
void cover_instrumenterst::add_from_criterion(
  coverage_criteriont criterion,
  const symbol_tablet &symbol_table,
  const goal_filterst &goal_filters)
{
  switch(criterion)
  {
  case coverage_criteriont::LOCATION:
    instrumenters.push_back(
      util_make_unique<cover_location_instrumentert>(
        symbol_table, goal_filters));
    break;
  case coverage_criteriont::BRANCH:
    instrumenters.push_back(
      util_make_unique<cover_branch_instrumentert>(symbol_table, goal_filters));
    break;
  case coverage_criteriont::DECISION:
    instrumenters.push_back(
      util_make_unique<cover_decision_instrumentert>(
        symbol_table, goal_filters));
    break;
  case coverage_criteriont::CONDITION:
    instrumenters.push_back(
      util_make_unique<cover_condition_instrumentert>(
        symbol_table, goal_filters));
    break;
  case coverage_criteriont::PATH:
    instrumenters.push_back(
      util_make_unique<cover_path_instrumentert>(symbol_table, goal_filters));
    break;
  case coverage_criteriont::MCDC:
    instrumenters.push_back(
      util_make_unique<cover_mcdc_instrumentert>(symbol_table, goal_filters));
    break;
  case coverage_criteriont::ASSERTION:
    instrumenters.push_back(
      util_make_unique<cover_assertion_instrumentert>(
        symbol_table, goal_filters));
    break;
  case coverage_criteriont::COVER:
    instrumenters.push_back(
      util_make_unique<cover_cover_instrumentert>(symbol_table, goal_filters));
  }
}

/// Parses a coverage criterion
/// \param criterion_string: a string
/// \return a coverage criterion or throws an exception
coverage_criteriont
parse_coverage_criterion(const std::string &criterion_string)
{
  coverage_criteriont c;

  if(criterion_string == "assertion" || criterion_string == "assertions")
    c = coverage_criteriont::ASSERTION;
  else if(criterion_string == "path" || criterion_string == "paths")
    c = coverage_criteriont::PATH;
  else if(criterion_string == "branch" || criterion_string == "branches")
    c = coverage_criteriont::BRANCH;
  else if(criterion_string == "location" || criterion_string == "locations")
    c = coverage_criteriont::LOCATION;
  else if(criterion_string == "decision" || criterion_string == "decisions")
    c = coverage_criteriont::DECISION;
  else if(criterion_string == "condition" || criterion_string == "conditions")
    c = coverage_criteriont::CONDITION;
  else if(criterion_string == "mcdc")
    c = coverage_criteriont::MCDC;
  else if(criterion_string == "cover")
    c = coverage_criteriont::COVER;
  else
  {
    std::stringstream s;
    s << "unknown coverage criterion " << '\'' << criterion_string << '\'';
    throw s.str();
  }

  return c;
}

/// Applies instrumenters to given goto functions
/// \param goto_functions: the goto functions
/// \param instrumenters: the instrumenters
/// \param function_filters: function filters to discard certain functions
/// \param message_handler: a message handler
void instrument_cover_goals(
  goto_functionst &goto_functions,
  const cover_instrumenterst &instrumenters,
  const function_filterst &function_filters,
  message_handlert &message_handler)
{
  Forall_goto_functions(f_it, goto_functions)
  {
    if(!function_filters(f_it->first, f_it->second))
      continue;

    instrument_cover_goals(f_it->second.body, instrumenters, message_handler);
  }
}

/// Instruments goto functions based on given command line options
/// \param cmdline: the command line
/// \param symbol_table: the symbol table
/// \param goto_functions: the goto functions
/// \param message_handler: a message handler
bool instrument_cover_goals(
  const cmdlinet &cmdline,
  const symbol_tablet &symbol_table,
  goto_functionst &goto_functions,
  message_handlert &message_handler)
{
  messaget msg(message_handler);

  function_filterst function_filters;
  function_filters.add(
    util_make_unique<internal_functions_filtert>(message_handler));

  goal_filterst goal_filters;
  goal_filters.add(util_make_unique<internal_goals_filtert>(message_handler));

  cover_instrumenterst instrumenters;

  std::list<std::string> criteria_strings=cmdline.get_values("cover");
  bool keep_assertions=false;

  for(const auto &criterion_string : criteria_strings)
  {
    try
    {
      coverage_criteriont c = parse_coverage_criterion(criterion_string);

      if(c == coverage_criteriont::ASSERTION)
        keep_assertions = true;

      instrumenters.add_from_criterion(c, symbol_table, goal_filters);
    }
    catch(const std::string &e)
    {
      msg.error() << e << messaget::eom;
      return true;
    }
  }

  if(keep_assertions && criteria_strings.size()>1)
  {
    msg.error() << "assertion coverage cannot currently be used together with "
                << "other coverage criteria" << messaget::eom;
    return true;
  }

  msg.status() << "Rewriting existing assertions as assumptions"
               << messaget::eom;

  if(!keep_assertions)
  {
    // turn assertions (from generic checks) into assumptions
    Forall_goto_functions(f_it, goto_functions)
    {
      goto_programt &body=f_it->second.body;
      Forall_goto_program_instructions(i_it, body)
      {
        if(i_it->is_assert())
          i_it->type=goto_program_instruction_typet::ASSUME;
      }
    }
  }

  // cover entry point function only
  std::string cover_include_pattern =
    cmdline.get_value("cover-include-pattern");
  if(cmdline.isset("cover-function-only"))
  {
    std::regex special_characters(
      "\\.|\\\\|\\*|\\+|\\?|\\{|\\}|\\[|\\]|\\(|\\)|\\^|\\$|\\|");
    cover_include_pattern =
      ".*" + std::regex_replace(config.main, special_characters, "\\$&") + ".*";
  }
  if(!cover_include_pattern.empty())
  {
    function_filters.add(
      util_make_unique<include_pattern_filtert>(
        message_handler, cover_include_pattern));
  }

  if(cmdline.isset("no-trivial-tests"))
    function_filters.add(
      util_make_unique<trivial_functions_filtert>(message_handler));

  msg.status() << "Instrumenting coverage goals" << messaget::eom;

  instrument_cover_goals(
    goto_functions, instrumenters, function_filters, message_handler);

  function_filters.report_anomalies();
  goal_filters.report_anomalies();

  if(cmdline.isset("cover-traces-must-terminate"))
  {
    // instrument an additional goal in CPROVER_START. This will rephrase
    // the reachability problem  by asking BMC to provide a solution that
    // satisfies a goal while getting to the end of the program-under-test.
    const auto sf_it=
      goto_functions.function_map.find(goto_functions.entry_point());
    if(sf_it==goto_functions.function_map.end())
    {
      msg.error() << "cover-traces-must-terminate: invalid entry point ["
                  << goto_functions.entry_point() << "]" << messaget::eom;
      return true;
    }

    cover_instrument_end_of_function(sf_it->first, sf_it->second.body);
  }

  goto_functions.update();
  return false;
}

/// Instruments a goto model based on given command line options
/// \param cmdline: the command line
/// \param goto_model: the goto model
/// \param message_handler: a message handler
bool instrument_cover_goals(
  const cmdlinet &cmdline,
  goto_modelt &goto_model,
  message_handlert &message_handler)
{
  return instrument_cover_goals(
    cmdline,
    goto_model.symbol_table,
    goto_model.goto_functions,
    message_handler);
}
