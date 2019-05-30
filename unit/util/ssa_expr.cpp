/*******************************************************************\

Module: Unit test for ssa_expr.h/ssa_expr.cpp

Author: Romain Brenguier, romain.brenguier@diffblue.com

\*******************************************************************/

#include <testing-utils/use_catch.h>

#include <util/arith_tools.h>
#include <util/ssa_expr.h>
#include <util/symbol_table.h>

TEST_CASE("Set level", "[unit][util][ssa_expr]")
{
  GIVEN("An SSA expression constructed from a symbol")
  {
    const signedbv_typet int_type{32};
    const symbol_exprt symbol{"sym", int_type};
    ssa_exprt ssa(symbol);

    WHEN("set_level_0")
    {
      ssa.set_level_0(1);
      REQUIRE(ssa.get_level_0() == "1");
      REQUIRE(ssa.get_level_1() == irep_idt{});
      REQUIRE(ssa.get_level_2() == irep_idt{});
      REQUIRE(ssa.get_original_expr() == symbol);
    }

    WHEN("set_level_1")
    {
      ssa.set_level_1(3);
      REQUIRE(ssa.get_level_0() == irep_idt{});
      REQUIRE(ssa.get_level_1() == "3");
      REQUIRE(ssa.get_level_2() == irep_idt{});
      REQUIRE(ssa.get_original_expr() == symbol);
    }

    WHEN("set_level_2")
    {
      ssa.set_level_2(7);
      REQUIRE(ssa.get_level_0() == irep_idt{});
      REQUIRE(ssa.get_level_1() == irep_idt{});
      REQUIRE(ssa.get_level_2() == "7");
      REQUIRE(ssa.get_original_expr() == symbol);
    }
  }
}

TEST_CASE("Set expression", "[unit][util][ssa_expr]")
{
  GIVEN("An SSA expression constructed from a symbol")
  {
    const signedbv_typet int_type{32};
    const array_typet array_type{int_type, from_integer(10, int_type)};
    const symbol_exprt symbol{"sym", array_type};
    const index_exprt index{symbol, from_integer(9, int_type)};
    ssa_exprt ssa(symbol);
    ssa.set_level_0(1);
    ssa.set_level_1(3);
    ssa.set_level_2(7);

    WHEN("call set_expression with an index_exprt")
    {
      ssa.set_expression(index);
      THEN("Indices are preserved")
      {
        REQUIRE(ssa.get_level_0() == "1");
        REQUIRE(ssa.get_level_1() == "3");
        REQUIRE(ssa.get_level_2() == "7");
      }
      THEN("The underlying expression has been replaced")
      {
        REQUIRE(ssa.get_original_expr() == index);
      }
    }
  }
}
