/*******************************************************************\

Module: Solvers for VCs Generated by Symbolic Execution of ANSI-C

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/// \file
/// Solvers for VCs Generated by Symbolic Execution of ANSI-C

#include "cbmc_solvers.h"

#include <fstream>
#include <iostream>
#include <memory>

#include <util/make_unique.h>
#include <util/unicode.h>
#include <util/version.h>

#include <solvers/sat/satcheck.h>
#include <solvers/refinement/bv_refinement.h>
#include <solvers/refinement/string_refinement.h>
#include <solvers/smt2/smt2_dec.h>
#include <solvers/sat/dimacs_cnf.h>

#include "cbmc_dimacs.h"
#include "counterexample_beautification.h"

/// Uses the options to pick an SMT 2.0 solver
/// \return An smt2_dect::solvert giving the solver to use.
smt2_dect::solvert cbmc_solverst::get_smt2_solver_type() const
{
  assert(options.get_bool_option("smt2"));

  smt2_dect::solvert s=smt2_dect::solvert::GENERIC;

  if(options.get_bool_option("boolector"))
    s=smt2_dect::solvert::BOOLECTOR;
  else if(options.get_bool_option("mathsat"))
    s=smt2_dect::solvert::MATHSAT;
  else if(options.get_bool_option("cvc3"))
    s=smt2_dect::solvert::CVC3;
  else if(options.get_bool_option("cvc4"))
    s=smt2_dect::solvert::CVC4;
  else if(options.get_bool_option("yices"))
    s=smt2_dect::solvert::YICES;
  else if(options.get_bool_option("z3"))
    s=smt2_dect::solvert::Z3;
  else if(options.get_bool_option("generic"))
    s=smt2_dect::solvert::GENERIC;

  return s;
}

std::unique_ptr<cbmc_solverst::solvert> cbmc_solverst::get_default()
{
  auto solver=util_make_unique<solvert>();

  if(options.get_bool_option("beautify") ||
     !options.get_bool_option("sat-preprocessor")) // no simplifier
  {
    // simplifier won't work with beautification
    solver->set_prop(util_make_unique<satcheck_no_simplifiert>());
  }
  else // with simplifier
  {
    solver->set_prop(util_make_unique<satcheckt>());
  }

  solver->prop().set_message_handler(get_message_handler());

  auto bv_pointers = util_make_unique<bv_pointerst>(ns, solver->prop());

  if(options.get_option("arrays-uf")=="never")
    bv_pointers->unbounded_array = bv_pointerst::unbounded_arrayt::U_NONE;
  else if(options.get_option("arrays-uf")=="always")
    bv_pointers->unbounded_array = bv_pointerst::unbounded_arrayt::U_ALL;

  solver->set_prop_conv(std::move(bv_pointers));

  return solver;
}

std::unique_ptr<cbmc_solverst::solvert> cbmc_solverst::get_dimacs()
{
  no_beautification();
  no_incremental_check();

  auto prop=util_make_unique<dimacs_cnft>();
  prop->set_message_handler(get_message_handler());

  std::string filename=options.get_option("outfile");

  auto cbmc_dimacs=util_make_unique<cbmc_dimacst>(ns, *prop, filename);
  return util_make_unique<solvert>(std::move(cbmc_dimacs), std::move(prop));
}

std::unique_ptr<cbmc_solverst::solvert> cbmc_solverst::get_bv_refinement()
{
  std::unique_ptr<propt> prop=[this]() -> std::unique_ptr<propt>
  {
    // We offer the option to disable the SAT preprocessor
    if(options.get_bool_option("sat-preprocessor"))
    {
      no_beautification();
      return util_make_unique<satcheckt>();
    }
    return util_make_unique<satcheck_no_simplifiert>();
  }();

  prop->set_message_handler(get_message_handler());

  bv_refinementt::infot info;
  info.ns=&ns;
  info.prop=prop.get();
  info.ui=ui;

  // we allow setting some parameters
  if(options.get_bool_option("max-node-refinement"))
    info.max_node_refinement=
      options.get_unsigned_int_option("max-node-refinement");

  info.refine_arrays=options.get_bool_option("refine-arrays");
  info.refine_arithmetic=options.get_bool_option("refine-arithmetic");

  return util_make_unique<solvert>(
    util_make_unique<bv_refinementt>(info),
    std::move(prop));
}

/// the string refinement adds to the bit vector refinement specifications for
/// functions from the Java string library
/// \return a solver for cbmc
std::unique_ptr<cbmc_solverst::solvert> cbmc_solverst::get_string_refinement()
{
  string_refinementt::infot info;
  info.ns=&ns;
  auto prop=util_make_unique<satcheck_no_simplifiert>();
  prop->set_message_handler(get_message_handler());
  info.prop=prop.get();
  info.refinement_bound=DEFAULT_MAX_NB_REFINEMENT;
  info.ui=ui;
  if(options.get_bool_option("max-node-refinement"))
    info.max_node_refinement=
      options.get_unsigned_int_option("max-node-refinement");
  info.refine_arrays=options.get_bool_option("refine-arrays");
  info.refine_arithmetic=options.get_bool_option("refine-arithmetic");

  return util_make_unique<solvert>(
    util_make_unique<string_refinementt>(info), std::move(prop));
}

std::unique_ptr<cbmc_solverst::solvert> cbmc_solverst::get_smt2(
  smt2_dect::solvert solver)
{
  no_beautification();

  const std::string &filename=options.get_option("outfile");

  if(filename=="")
  {
    if(solver==smt2_dect::solvert::GENERIC)
    {
      error() << "please use --outfile" << eom;
      throw 0;
    }

    auto smt2_dec = util_make_unique<smt2_dect>(
      ns,
      "cbmc",
      std::string("Generated by CBMC ") + CBMC_VERSION,
      "QF_AUFBV",
      solver);

    if(options.get_bool_option("fpa"))
      smt2_dec->use_FPA_theory=true;

    return util_make_unique<solvert>(std::move(smt2_dec));
  }
  else if(filename=="-")
  {
    auto smt2_conv = util_make_unique<smt2_convt>(
      ns,
      "cbmc",
      std::string("Generated by CBMC ") + CBMC_VERSION,
      "QF_AUFBV",
      solver,
      std::cout);

    if(options.get_bool_option("fpa"))
      smt2_conv->use_FPA_theory=true;

    smt2_conv->set_message_handler(get_message_handler());

    return util_make_unique<solvert>(std::move(smt2_conv));
  }
  else
  {
    #ifdef _MSC_VER
    auto out=util_make_unique<std::ofstream>(widen(filename));
    #else
    auto out=util_make_unique<std::ofstream>(filename);
    #endif

    if(!*out)
    {
      error() << "failed to open " << filename << eom;
      throw 0;
    }

    auto smt2_conv = util_make_unique<smt2_convt>(
      ns,
      "cbmc",
      std::string("Generated by CBMC ") + CBMC_VERSION,
      "QF_AUFBV",
      solver,
      *out);

    if(options.get_bool_option("fpa"))
      smt2_conv->use_FPA_theory=true;

    smt2_conv->set_message_handler(get_message_handler());

    return util_make_unique<solvert>(std::move(smt2_conv), std::move(out));
  }
}

void cbmc_solverst::no_beautification()
{
  if(options.get_bool_option("beautify"))
  {
    error() << "sorry, this solver does not support beautification" << eom;
    throw 0;
  }
}

void cbmc_solverst::no_incremental_check()
{
  if(options.get_bool_option("all-properties") ||
     options.get_option("cover")!="" ||
     options.get_option("incremental-check")!="")
  {
    error() << "sorry, this solver does not support incremental solving" << eom;
    throw 0;
  }
}
