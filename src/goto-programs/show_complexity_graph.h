/*******************************************************************\

Module: Show the complexity call graph as a dot program

Author: Benjamin Quiring

\*******************************************************************/

/// \file
/// Show the complexity call graph, where nodes are colored based on a
/// "proof complexity" weight mechanism and have annotations describing
/// the features of different functions.

#ifndef CPROVER_COMPLEXITY_GRAPH_SHOW_COMPLEXITY_GRAPH
#define CPROVER_COMPLEXITY_GRAPH_SHOW_COMPLEXITY_GRAPH

#include <list>
#include <string>
#include <set>
#include <map>
#include <goto-programs/goto_program.h>
#include "metrics.h"
#include <util/options.h>
#include <util/ui_message.h>

class namespacet;
class abstract_goto_modelt;
class goto_functionst;
class ui_message_handlert;

#define HELP_SHOW_COMPLEXITY_GRAPH \
  " --show-complexity-graph        show goto control-flow-graph with nodes colored with proof complexity\n" \
  " --complexity-graph-root        provides a root for the complexity control-flow-graph\n" \
  " --complexity-graph-omit-function   omits a function from the complexity control-flow-graph\n" \
  " --complexity-graph-omit-function-pointers   omits function pointers from the complexity control-flow-graph\n"
// clang-format on

void show_complexity_graph(
  const optionst &options,
  const abstract_goto_modelt &,
  const std::string &path,
  message_handlert &message_handler);

void show_complexity_graph(
  const optionst &options,
  const abstract_goto_modelt &,
  const std::string &path,
  message_handlert &message_handler,
  const std::map<goto_programt::const_targett, symex_infot> &instr_symex_info);

void show_complexity_graph(
  const optionst &options,
  const abstract_goto_modelt &,
  const std::string &path,
  message_handlert &message_handler,
  const std::map<goto_programt::const_targett, symex_infot> &instr_symex_info,
  const std::map<goto_programt::const_targett, solver_infot> &instr_solver_info);

#endif // CPROVER_COMPLEXITY_GRAPH_SHOW_COMPLEXITY_GRAPH_H
