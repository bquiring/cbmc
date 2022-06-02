/*******************************************************************\

Module: Show the goto functions as a dot program

Author: Benjamin Quiring

\*******************************************************************/

/// \file
/// Show goto functions

#include "show_goto_proof_cfg.h"

#include <util/ui_message.h>
#include <util/format_expr.h>
#include <math.h>

#include "goto_model.h"
#include "goto_program.h"

int number_function_calls (const goto_programt::instructionst& instructions) {
  int count = 0;
  for (const auto &instruction : instructions) {
    switch (instruction.type())
    {
      // we may want to do something for GOTOs later
    case GOTO:
    case INCOMPLETE_GOTO:
      break;
    case FUNCTION_CALL:
      count = count + 1;
      break;
    case NO_INSTRUCTION_TYPE:
    case OTHER:
    case SET_RETURN_VALUE:
    case DECL:
    case DEAD:
    case ASSIGN:
    case ASSUME:
    case ASSERT:
    case SKIP:
    case END_FUNCTION:
    case LOCATION:
    case THROW:
    case CATCH:
    case ATOMIC_BEGIN:
    case ATOMIC_END:
    case START_THREAD:
    case END_THREAD:
      break;
    }

  }
  return count;
}

// the score metric associated with the function.
// a large score means the associated proof should be more difficult.
int score (const symbolt &symbol, const goto_programt::instructionst& instructions) {
  return number_function_calls (instructions);
}

void normalize_scores (std::map<irep_idt, int> &scores) {
  int max = 0;
  for (auto it = scores.begin(); it != scores.end(); it++) {
    max = std::max (max, it->second);
  }

  for (auto it = scores.begin(); it != scores.end(); it++) {
    // score between [0, inf)
    int score = it->second;
    // normalized score between [0, 255]
    float norm_score = 256.0 * ((float) score) / ((float) max);
    int normalized_score = std::min(255, std::max(0, (int) norm_score));
    it->second = normalized_score;
  }
  return;
}

void compute_scores (const namespacet &ns, 
                     std::map<irep_idt, int> &scores, 
                     const goto_functionst &goto_functions) {
  const auto funs = goto_functions.sorted();

  for (const auto &fun : funs) {
    const symbolt &symbol = ns.lookup(fun->first);
    const irep_idt &name = symbol.name;
    int sc = 0;
    const bool has_body = fun->second.body_available();
    if (has_body) {
      const auto &instructions = fun->second.body.instructions;
      sc = score (symbol, instructions);
    }
    scores.insert ({name, sc});
  }
  normalize_scores(scores);
}

bool is_used (const std::map<irep_idt, bool> &use, const irep_idt &name) {

    const auto used = use.find (name);
    return used != use.end() && used->second;
}

// simple depth first search
void find_used (irep_idt root,
                messaget &msg,
                const namespacet &ns,
                const goto_functionst &goto_functions,
                std::map<irep_idt, bool> &use) {
  if (use.find(root) == use.end()) {
    use.insert({root, true});
    const auto fun = goto_functions.function_map.find(root);
    if (fun != goto_functions.function_map.end()) {
      for (const auto &instruction : fun->second.body.instructions) {
        switch (instruction.type())
        {
          // we may want to do something for GOTOs later
        case GOTO:
          break;
        case FUNCTION_CALL: {
          const irep_idt call = ns.lookup(to_symbol_expr(instruction.call_function())).name;
          find_used (call, msg, ns, goto_functions, use);
          break;
        }
        case OTHER:
        case SET_RETURN_VALUE:
        case DECL:
        case DEAD:
        case ASSIGN:
        case ASSUME:
        case ASSERT:
        case SKIP:
        case END_FUNCTION:
        case LOCATION:
          // unused: 
        case INCOMPLETE_GOTO:
        case NO_INSTRUCTION_TYPE:
        case THROW:
        case CATCH:
        case ATOMIC_BEGIN:
        case ATOMIC_END:
        case START_THREAD:
        case END_THREAD:
          break;
        }

      }
    }
  }
}

void remove_functions_no_body (const namespacet &ns,
                               const goto_functionst &goto_functions,
                               std::map<irep_idt, bool> &use) {
  const auto sorted = goto_functions.sorted();

  for(const auto &fun : sorted) {
    const symbolt &symbol = ns.lookup(fun->first);
    const bool has_body = fun->second.body_available();

    if (!has_body) {
      const auto find = use.find (symbol.name);
      if (find == use.end()) {
        use.insert ({symbol.name, false});
      } else {
        find->second = false;
      }
    }
  }
}

void show_goto_proof_cfg(
  const namespacet &ns,
  ui_message_handlert &ui_message_handler,
  const std::list<std::string> roots,
  const goto_functionst &goto_functions)
{
  //goto_functionst goto_functions;
  //goto_functions.copy_from(goto_functions_);

  messaget msg(ui_message_handler);

  std::map<irep_idt, bool> use;
   msg.status() << roots.size() << "\n";
  if (roots.size() == 0) {
    find_used (goto_functions.entry_point(), msg, ns, goto_functions, use);
  }
  for (const auto &root : roots) {
    const irep_idt &iden = root;
    find_used (iden, msg, ns, goto_functions, use);
    // msg.status() << iden << "\n";
  }

  remove_functions_no_body(ns, goto_functions, use);

  // sort functions alphabetically
  const auto sorted = goto_functions.sorted();

  msg.status() << "digraph G {\n\n";

  std::map<irep_idt, int> scores;
  // initialize scores
  compute_scores (ns, scores, goto_functions);

  for(const auto &fun : sorted)
  {
    const symbolt &symbol = ns.lookup(fun->first);
    const bool has_body = fun->second.body_available();

    // msg.status() << "// " << symbol.name << " " << is_used (use, symbol.name) << "\n";

    if (is_used (use, symbol.name)) {

      if(has_body)
      {
        msg.status() << "// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n";
        msg.status() << messaget::bold << "//" << symbol.display_name()
                     << messaget::reset << " /* " << symbol.name << " */\n";

        const auto &instructions = fun->second.body.instructions;
        int s = scores.find (symbol.name)->second;
        // negate s so that higher original score => more red.
        s = 255 - s;

        std::stringstream stream;
        // Red
        stream << std::hex << 255;
        // Green
        if (s < 16) { stream << 0 << s; } else { stream << s; }
        // Blue
        if (s < 16) { stream << 0 << s; } else { stream << s; }
        std::string color( stream.str() );

        msg.status() << symbol.name 
                     << " [" 
                     << "style=filled,"
                     << "fillcolor=" << "\"#" << color << "\""
                     << "];\n";

        std::set<irep_idt> calls;
        for (const auto &instruction : instructions) {
          switch (instruction.type())
          {
            // we may want to do something for GOTOs later
          case GOTO:
            break;
          case FUNCTION_CALL: {
            // overapproximate by adding duplicate calls
            const irep_idt call = ns.lookup(to_symbol_expr(instruction.call_function())).name;
            if (is_used (use, call)) {
              calls.insert (call);
              //msg.status() << symbol.name << " -> " << format(instruction.call_function()) << ";\n";
            }
            break;
          }
          case OTHER:
          case SET_RETURN_VALUE:
          case DECL:
          case DEAD:
          case ASSIGN:
          case ASSUME:
          case ASSERT:
          case SKIP:
          case END_FUNCTION:
          case LOCATION:
            // unused: 
          case INCOMPLETE_GOTO:
          case NO_INSTRUCTION_TYPE:
          case THROW:
          case CATCH:
          case ATOMIC_BEGIN:
          case ATOMIC_END:
          case START_THREAD:
          case END_THREAD:
            break;
          }

        }
        for (const auto &call : calls) {
          msg.status() << symbol.name << " -> " << call << ";\n";
        }

        // fun->second.body.output(ns, symbol.name, msg.status());
        // msg.status() << messaget::eom;
      }
      else
      {
        msg.status() << symbol.name 
                     << " [" 
                     << "shape=Mdiamond"
                     << "];\n";

      }
    }
  }

  // end digraph
  msg.status() << "}";
  msg.status() << messaget::eom;

}


void show_goto_proof_cfg(
  const goto_modelt &goto_model,
  const std::list<std::string> roots,
  ui_message_handlert &ui_message_handler)
{
  const namespacet ns(goto_model.symbol_table);
  show_goto_proof_cfg(
    ns, ui_message_handler, roots, goto_model.goto_functions);
}
