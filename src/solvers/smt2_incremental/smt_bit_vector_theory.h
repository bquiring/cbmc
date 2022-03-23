// Author: Diffblue Ltd.

#ifndef CPROVER_SOLVERS_SMT2_INCREMENTAL_SMT_BIT_VECTOR_THEORY_H
#define CPROVER_SOLVERS_SMT2_INCREMENTAL_SMT_BIT_VECTOR_THEORY_H

#include <solvers/smt2_incremental/smt_terms.h>

class smt_bit_vector_theoryt
{
public:
  struct concatt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<concatt> concat;

  struct extractt final
  {
    std::size_t i;
    std::size_t j;
    static const char *identifier();
    smt_sortt return_sort(const smt_termt &operand) const;
    std::vector<smt_indext> indices() const;
    void validate(const smt_termt &operand) const;
  };
  /// \brief
  ///   Makes a factory for extract function applications.
  /// \param i
  ///   Index of the highest bit to be included in the resulting bit vector.
  /// \param j
  ///   Index of the lowest bit to be included in the resulting bit vector.
  /// \note
  ///   Bit vectors are zero indexed. So the lowest bit index is zero and the
  ///   largest index is the size of the bit vector minus one.
  static smt_function_application_termt::factoryt<extractt>
  extract(std::size_t i, std::size_t j);

  // Bitwise operators
  struct nott final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &operand);
    static void validate(const smt_termt &operand);
  };
  static const smt_function_application_termt::factoryt<nott> make_not;

  struct andt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<andt> make_and;

  struct ort final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<ort> make_or;

  struct nandt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<nandt> nand;

  struct nort final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<nort> nor;

  struct xort final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<xort> make_xor;

  struct xnort final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<xnort> xnor;

  struct comparet final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<comparet> compare;

  // Relational operator class declarations
  struct unsigned_less_thant final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<unsigned_less_thant>
    unsigned_less_than;

  struct unsigned_less_than_or_equalt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<
    unsigned_less_than_or_equalt>
    unsigned_less_than_or_equal;

  struct unsigned_greater_thant final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<unsigned_greater_thant>
    unsigned_greater_than;

  struct unsigned_greater_than_or_equalt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<
    unsigned_greater_than_or_equalt>
    unsigned_greater_than_or_equal;

  struct signed_less_thant final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<signed_less_thant>
    signed_less_than;

  struct signed_less_than_or_equalt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<
    signed_less_than_or_equalt>
    signed_less_than_or_equal;

  struct signed_greater_thant final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<signed_greater_thant>
    signed_greater_than;

  struct signed_greater_than_or_equalt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<
    signed_greater_than_or_equalt>
    signed_greater_than_or_equal;

  struct addt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<addt> add;

  struct subtractt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<subtractt> subtract;

  struct multiplyt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<multiplyt> multiply;

  struct unsigned_dividet final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<unsigned_dividet>
    unsigned_divide;

  struct signed_dividet final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<signed_dividet>
    signed_divide;

  struct unsigned_remaindert final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<unsigned_remaindert>
    unsigned_remainder;

  struct signed_remaindert final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<signed_remaindert>
    signed_remainder;

  struct negatet final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &operand);
    static void validate(const smt_termt &operand);
  };
  /// \brief Arithmetic negation in two's complement.
  static const smt_function_application_termt::factoryt<negatet> negate;

  // Shift operations
  struct shift_leftt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<shift_leftt> shift_left;

  struct logical_shift_rightt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<logical_shift_rightt>
    logical_shift_right;

  struct arithmetic_shift_rightt final
  {
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &lhs, const smt_termt &rhs);
    static void validate(const smt_termt &lhs, const smt_termt &rhs);
  };
  static const smt_function_application_termt::factoryt<arithmetic_shift_rightt>
    arithmetic_shift_right;

  struct repeatt final
  {
    std::size_t i;
    static const char *identifier();
    smt_sortt return_sort(const smt_termt &operand) const;
    std::vector<smt_indext> indices() const;
    void validate(const smt_termt &operand) const;
  };
  static smt_function_application_termt::factoryt<repeatt>
  repeat(std::size_t i);

  struct zero_extendt final
  {
    std::size_t i;
    static const char *identifier();
    smt_sortt return_sort(const smt_termt &operand) const;
    std::vector<smt_indext> indices() const;
    static void validate(const smt_termt &operand);
  };
  static smt_function_application_termt::factoryt<zero_extendt>
  zero_extend(std::size_t i);

  struct sign_extendt final
  {
    std::size_t i;
    static const char *identifier();
    smt_sortt return_sort(const smt_termt &operand) const;
    std::vector<smt_indext> indices() const;
    static void validate(const smt_termt &operand);
  };
  static smt_function_application_termt::factoryt<sign_extendt>
  sign_extend(std::size_t i);

  struct rotate_leftt final
  {
    std::size_t i;
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &operand);
    std::vector<smt_indext> indices() const;
    static void validate(const smt_termt &operand);
  };
  static smt_function_application_termt::factoryt<rotate_leftt>
  rotate_left(std::size_t i);

  struct rotate_rightt final
  {
    std::size_t i;
    static const char *identifier();
    static smt_sortt return_sort(const smt_termt &operand);
    std::vector<smt_indext> indices() const;
    static void validate(const smt_termt &operand);
  };
  static smt_function_application_termt::factoryt<rotate_rightt>
  rotate_right(std::size_t i);
};

#endif // CPROVER_SOLVERS_SMT2_INCREMENTAL_SMT_BIT_VECTOR_THEORY_H
