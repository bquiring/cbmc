/*******************************************************************\

 Module: Tests for full_array_abstract_objectt maximum length

 Author: Jez Higgins

\*******************************************************************/

#include "array_builder.h"
#include <analyses/variable-sensitivity/variable_sensitivity_object_factory.h>
#include <analyses/variable-sensitivity/variable_sensitivity_test_helpers.h>
#include <testing-utils/use_catch.h>
#include <util/arith_tools.h>
#include <util/bitvector_types.h>
#include <util/mathematical_types.h>

using abstract_object_ptrt = std::shared_ptr<const abstract_objectt>;

static abstract_object_ptrt write_array(
  const abstract_object_ptrt &array,
  int index,
  int new_value,
  abstract_environmentt &env,
  namespacet &ns)
{
  const typet type = signedbv_typet(32);

  return array->write(
    env,
    ns,
    std::stack<exprt>(),
    index_exprt(nil_exprt(), from_integer(index, type)),
    env.eval(from_integer(new_value, type), ns),
    false);
}

SCENARIO(
  "arrays have maximum length",
  "[core][analyses][variable-sensitivity][full_array_abstract_object][max_"
  "array]")
{
  auto object_factory = variable_sensitivity_object_factoryt::configured_with(
    vsd_configt::value_set());
  abstract_environmentt environment(object_factory);
  environment.make_top();
  symbol_tablet symbol_table;
  namespacet ns(symbol_table);

  WHEN("array = {1, 2, 3}, writes under maximum size")
  {
    WHEN("array[3] = 4")
    {
      auto array = build_array({1, 2, 3}, environment, ns);

      auto updated = write_array(array, 3, 4, environment, ns);

      THEN("array equals {1, 2, 3, 4}")
      {
        EXPECT_INDEX(updated, 0, 1, environment, ns);
        EXPECT_INDEX(updated, 1, 2, environment, ns);
        EXPECT_INDEX(updated, 2, 3, environment, ns);
        EXPECT_INDEX(updated, 3, 4, environment, ns);
      }
    }
    WHEN("a[0] = 99")
    {
      auto array = build_array({1, 2, 3}, environment, ns);

      auto updated = write_array(array, 0, 99, environment, ns);

      THEN("array equals {99, 2, 3}")
      {
        EXPECT_INDEX(updated, 0, 99, environment, ns);
        EXPECT_INDEX(updated, 1, 2, environment, ns);
        EXPECT_INDEX(updated, 2, 3, environment, ns);
      }
    }
    WHEN("a[5] = 99")
    {
      auto array = build_array({1, 2, 3}, environment, ns);

      auto updated = write_array(array, 5, 99, environment, ns);

      THEN("array equals {1, 2, 3, <empty>, <empty>, 99}")
      {
        EXPECT_INDEX(updated, 0, 1, environment, ns);
        EXPECT_INDEX(updated, 1, 2, environment, ns);
        EXPECT_INDEX(updated, 2, 3, environment, ns);
        EXPECT_INDEX_TOP(updated, 3, environment, ns);
        EXPECT_INDEX_TOP(updated, 4, environment, ns);
        EXPECT_INDEX(updated, 5, 99, environment, ns);
      }
    }
  }
  WHEN("array = {1, 2, 3}, writes beyond maximum size mapped to max_size")
  {
    WHEN("array[99] = 4")
    {
      auto array = build_array({1, 2, 3}, environment, ns);

      auto updated = write_array(array, 99, 4, environment, ns);

      THEN("array equals {1, 2, 3, ..., [10] = 4}")
      {
        EXPECT_INDEX(updated, 0, 1, environment, ns);
        EXPECT_INDEX(updated, 1, 2, environment, ns);
        EXPECT_INDEX(updated, 2, 3, environment, ns);
        EXPECT_INDEX(updated, 10, 4, environment, ns);
      }
    }
  }
  WHEN("array = {1, 2, 3}, reads beyond maximum size mapped to max_size")
  {
    WHEN("a[max] = 4")
    {
      auto array = build_array({1, 2, 3}, environment, ns);

      auto updated = write_array(array, 99, 4, environment, ns);

      for(int i = 10; i <= 30; i += 5)
        THEN("array[" + std::to_string(i) + "] = 4}")
        {
          EXPECT_INDEX(updated, i, 4, environment, ns);
        }
    }
  }
  WHEN("array = {1, 2, 3}, writes beyond maximum size are merged")
  {
    WHEN("array[98] = 3, array[99] = 4")
    {
      auto array = build_array({1, 2, 3}, environment, ns);

      auto updated = write_array(array, 99, 4, environment, ns);
      updated = write_array(updated, 98, 3, environment, ns);

      THEN("array equals {1, 2, 3, ..., [10] = TOP}")
      {
        EXPECT_INDEX(updated, 0, 1, environment, ns);
        EXPECT_INDEX(updated, 1, 2, environment, ns);
        EXPECT_INDEX(updated, 2, 3, environment, ns);
        EXPECT_INDEX(updated, 10, {3, 4}, environment, ns);
      }
    }
    WHEN("array[99] = 3, array[99] = 4, array[100] = 5")
    {
      auto array = build_array({1, 2, 3}, environment, ns);

      auto updated = write_array(array, 100, 5, environment, ns);
      updated = write_array(updated, 99, 4, environment, ns);
      updated = write_array(updated, 98, 3, environment, ns);

      THEN("array equals {1, 2, 3, ..., [10] = TOP}")
      {
        EXPECT_INDEX(updated, 0, 1, environment, ns);
        EXPECT_INDEX(updated, 1, 2, environment, ns);
        EXPECT_INDEX(updated, 2, 3, environment, ns);
        EXPECT_INDEX(updated, 10, {3, 4, 5}, environment, ns);
      }
    }
  }
}
