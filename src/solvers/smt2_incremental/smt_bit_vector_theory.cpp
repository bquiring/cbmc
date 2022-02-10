// Author: Diffblue Ltd.

#include <solvers/smt2_incremental/smt_bit_vector_theory.h>

#include <util/invariant.h>

const char *smt_bit_vector_theoryt::concatt::identifier()
{
  return "concat";
}

smt_sortt smt_bit_vector_theoryt::concatt::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  const auto get_width = [](const smt_termt &term) {
    return term.get_sort().cast<smt_bit_vector_sortt>()->bit_width();
  };
  return smt_bit_vector_sortt{get_width(lhs) + get_width(rhs)};
}

void smt_bit_vector_theoryt::concatt::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  const auto lhs_sort = lhs.get_sort().cast<smt_bit_vector_sortt>();
  INVARIANT(lhs_sort, "Left operand must have bitvector sort.");
  const auto rhs_sort = rhs.get_sort().cast<smt_bit_vector_sortt>();
  INVARIANT(rhs_sort, "Right operand must have bitvector sort.");
}

const smt_function_application_termt::factoryt<smt_bit_vector_theoryt::concatt>
  smt_bit_vector_theoryt::concat{};

const char *smt_bit_vector_theoryt::extractt::identifier()
{
  return "extract";
}

smt_sortt
smt_bit_vector_theoryt::extractt::return_sort(const smt_termt &operand) const
{
  return smt_bit_vector_sortt{i - j + 1};
}

std::vector<smt_indext> smt_bit_vector_theoryt::extractt::indices() const
{
  return {smt_numeral_indext{i}, smt_numeral_indext{j}};
}

void smt_bit_vector_theoryt::extractt::validate(const smt_termt &operand) const
{
  PRECONDITION(i >= j);
  const auto bit_vector_sort = operand.get_sort().cast<smt_bit_vector_sortt>();
  PRECONDITION(bit_vector_sort);
  PRECONDITION(i < bit_vector_sort->bit_width());
}

smt_function_application_termt::factoryt<smt_bit_vector_theoryt::extractt>
smt_bit_vector_theoryt::extract(std::size_t i, std::size_t j)
{
  PRECONDITION(i >= j);
  return smt_function_application_termt::factoryt<extractt>(i, j);
}

static void validate_bit_vector_operator_arguments(
  const smt_termt &left,
  const smt_termt &right)
{
  const auto left_sort = left.get_sort().cast<smt_bit_vector_sortt>();
  INVARIANT(left_sort, "Left operand must have bitvector sort.");
  const auto right_sort = right.get_sort().cast<smt_bit_vector_sortt>();
  INVARIANT(right_sort, "Right operand must have bitvector sort.");
  // The below invariant is based on the smtlib standard.
  // See http://smtlib.cs.uiowa.edu/logics-all.shtml#QF_BV
  INVARIANT(
    left_sort->bit_width() == right_sort->bit_width(),
    "Left and right operands must have the same bit width.");
}

// Bitwise operator definitions

const char *smt_bit_vector_theoryt::nott::identifier()
{
  return "bvnot";
}

smt_sortt smt_bit_vector_theoryt::nott::return_sort(const smt_termt &operand)
{
  return operand.get_sort();
}

void smt_bit_vector_theoryt::nott::validate(const smt_termt &operand)
{
  const auto operand_sort = operand.get_sort().cast<smt_bit_vector_sortt>();
  INVARIANT(operand_sort, "The operand is expected to have a bit-vector sort.");
}

const smt_function_application_termt::factoryt<smt_bit_vector_theoryt::nott>
  smt_bit_vector_theoryt::make_not{};

const char *smt_bit_vector_theoryt::andt::identifier()
{
  return "bvand";
}

smt_sortt smt_bit_vector_theoryt::andt::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return lhs.get_sort();
}

void smt_bit_vector_theoryt::andt::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<smt_bit_vector_theoryt::andt>
  smt_bit_vector_theoryt::make_and{};

const char *smt_bit_vector_theoryt::ort::identifier()
{
  return "bvor";
}

smt_sortt smt_bit_vector_theoryt::ort::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return lhs.get_sort();
}

void smt_bit_vector_theoryt::ort::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<smt_bit_vector_theoryt::ort>
  smt_bit_vector_theoryt::make_or{};

// Relational operator definitions

const char *smt_bit_vector_theoryt::unsigned_less_thant::identifier()
{
  return "bvult";
}

smt_sortt smt_bit_vector_theoryt::unsigned_less_thant::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return smt_bool_sortt{};
}

void smt_bit_vector_theoryt::unsigned_less_thant::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::unsigned_less_thant>
  smt_bit_vector_theoryt::unsigned_less_than{};

const char *smt_bit_vector_theoryt::unsigned_less_than_or_equalt::identifier()
{
  return "bvule";
}

smt_sortt smt_bit_vector_theoryt::unsigned_less_than_or_equalt::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return smt_bool_sortt{};
}

void smt_bit_vector_theoryt::unsigned_less_than_or_equalt::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::unsigned_less_than_or_equalt>
  smt_bit_vector_theoryt::unsigned_less_than_or_equal{};

const char *smt_bit_vector_theoryt::unsigned_greater_thant::identifier()
{
  return "bvugt";
}

smt_sortt smt_bit_vector_theoryt::unsigned_greater_thant::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return smt_bool_sortt{};
}

void smt_bit_vector_theoryt::unsigned_greater_thant::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::unsigned_greater_thant>
  smt_bit_vector_theoryt::unsigned_greater_than{};

const char *
smt_bit_vector_theoryt::unsigned_greater_than_or_equalt::identifier()
{
  return "bvuge";
}

smt_sortt smt_bit_vector_theoryt::unsigned_greater_than_or_equalt::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return smt_bool_sortt{};
}

void smt_bit_vector_theoryt::unsigned_greater_than_or_equalt::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::unsigned_greater_than_or_equalt>
  smt_bit_vector_theoryt::unsigned_greater_than_or_equal{};

const char *smt_bit_vector_theoryt::signed_less_thant::identifier()
{
  return "bvslt";
}

smt_sortt smt_bit_vector_theoryt::signed_less_thant::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return smt_bool_sortt{};
}

void smt_bit_vector_theoryt::signed_less_thant::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::signed_less_thant>
  smt_bit_vector_theoryt::signed_less_than{};

const char *smt_bit_vector_theoryt::signed_less_than_or_equalt::identifier()
{
  return "bvsle";
}

smt_sortt smt_bit_vector_theoryt::signed_less_than_or_equalt::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return smt_bool_sortt{};
}

void smt_bit_vector_theoryt::signed_less_than_or_equalt::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::signed_less_than_or_equalt>
  smt_bit_vector_theoryt::signed_less_than_or_equal{};

const char *smt_bit_vector_theoryt::signed_greater_thant::identifier()
{
  return "bvsgt";
}

smt_sortt smt_bit_vector_theoryt::signed_greater_thant::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return smt_bool_sortt{};
}

void smt_bit_vector_theoryt::signed_greater_thant::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::signed_greater_thant>
  smt_bit_vector_theoryt::signed_greater_than{};

const char *smt_bit_vector_theoryt::signed_greater_than_or_equalt::identifier()
{
  return "bvsge";
}

smt_sortt smt_bit_vector_theoryt::signed_greater_than_or_equalt::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return smt_bool_sortt{};
}

void smt_bit_vector_theoryt::signed_greater_than_or_equalt::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::signed_greater_than_or_equalt>
  smt_bit_vector_theoryt::signed_greater_than_or_equal{};

const char *smt_bit_vector_theoryt::addt::identifier()
{
  return "bvadd";
}

smt_sortt smt_bit_vector_theoryt::addt::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return lhs.get_sort();
}

void smt_bit_vector_theoryt::addt::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<smt_bit_vector_theoryt::addt>
  smt_bit_vector_theoryt::add{};

const char *smt_bit_vector_theoryt::subtractt::identifier()
{
  return "bvsub";
}

smt_sortt smt_bit_vector_theoryt::subtractt::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return lhs.get_sort();
}

void smt_bit_vector_theoryt::subtractt::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::subtractt>
  smt_bit_vector_theoryt::subtract{};

const char *smt_bit_vector_theoryt::multiplyt::identifier()
{
  return "bvmul";
}

smt_sortt smt_bit_vector_theoryt::multiplyt::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return lhs.get_sort();
}

void smt_bit_vector_theoryt::multiplyt::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::multiplyt>
  smt_bit_vector_theoryt::multiply{};

const char *smt_bit_vector_theoryt::unsigned_dividet::identifier()
{
  return "bvudiv";
}

smt_sortt smt_bit_vector_theoryt::unsigned_dividet::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return lhs.get_sort();
}

void smt_bit_vector_theoryt::unsigned_dividet::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::unsigned_dividet>
  smt_bit_vector_theoryt::unsigned_divide{};

const char *smt_bit_vector_theoryt::signed_dividet::identifier()
{
  return "bvsdiv";
}

smt_sortt smt_bit_vector_theoryt::signed_dividet::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return lhs.get_sort();
}

void smt_bit_vector_theoryt::signed_dividet::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::signed_dividet>
  smt_bit_vector_theoryt::signed_divide{};

const char *smt_bit_vector_theoryt::unsigned_remaindert::identifier()
{
  return "bvurem";
}

smt_sortt smt_bit_vector_theoryt::unsigned_remaindert::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return lhs.get_sort();
}

void smt_bit_vector_theoryt::unsigned_remaindert::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::unsigned_remaindert>
  smt_bit_vector_theoryt::unsigned_remainder{};

const char *smt_bit_vector_theoryt::signed_remaindert::identifier()
{
  return "bvsrem";
}

smt_sortt smt_bit_vector_theoryt::signed_remaindert::return_sort(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  return lhs.get_sort();
}

void smt_bit_vector_theoryt::signed_remaindert::validate(
  const smt_termt &lhs,
  const smt_termt &rhs)
{
  validate_bit_vector_operator_arguments(lhs, rhs);
}

const smt_function_application_termt::factoryt<
  smt_bit_vector_theoryt::signed_remaindert>
  smt_bit_vector_theoryt::signed_remainder{};

const char *smt_bit_vector_theoryt::negatet::identifier()
{
  return "bvneg";
}

smt_sortt smt_bit_vector_theoryt::negatet::return_sort(const smt_termt &operand)
{
  return operand.get_sort();
}

void smt_bit_vector_theoryt::negatet::validate(const smt_termt &operand)
{
  const auto operand_sort = operand.get_sort().cast<smt_bit_vector_sortt>();
  INVARIANT(operand_sort, "The operand is expected to have a bit-vector sort.");
}

const smt_function_application_termt::factoryt<smt_bit_vector_theoryt::negatet>
  smt_bit_vector_theoryt::negate{};
