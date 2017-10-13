/*******************************************************************\

Module: Value Set

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/// \file
/// Value Set

#ifndef CPROVER_POINTER_ANALYSIS_VALUE_SET_H
#define CPROVER_POINTER_ANALYSIS_VALUE_SET_H

#include <set>

#include <util/mp_arith.h>
#include <util/reference_counting.h>

#include "object_numbering.h"
#include "value_sets.h"

class namespacet;

class value_sett
{
  typedef std::function<void(exprt &, const namespacet &)> expr_simplifiert;

  static expr_simplifiert default_simplifier;

public:
  value_sett():
    location_number(0),
    simplifier(default_simplifier)
  {
  }

  explicit value_sett(expr_simplifiert simplifier):
    location_number(0),
    simplifier(std::move(simplifier))
  {
  }

  static bool field_sensitive(
    const irep_idt &id,
    const typet &type,
    const namespacet &);

  unsigned location_number;
  static object_numberingt object_numbering;

  typedef irep_idt idt;

  class objectt
  {
  public:
    objectt():offset_is_set(false)
    {
    }

    explicit objectt(const mp_integer &_offset):
      offset(_offset),
      offset_is_set(true)
    {
    }

    mp_integer offset;
    bool offset_is_set;
    bool offset_is_zero() const
    { return offset_is_set && offset.is_zero(); }
  };

  class object_map_dt
  {
    typedef std::map<unsigned, objectt> data_typet;
    data_typet data;

  public:
    // NOLINTNEXTLINE(readability/identifiers)
    typedef data_typet::iterator iterator;
    // NOLINTNEXTLINE(readability/identifiers)
    typedef data_typet::const_iterator const_iterator;
    // NOLINTNEXTLINE(readability/identifiers)
    typedef data_typet::value_type value_type;
    // NOLINTNEXTLINE(readability/identifiers)
    typedef data_typet::key_type key_type;

    iterator begin() { return data.begin(); }
    const_iterator begin() const { return data.begin(); }
    const_iterator cbegin() const { return data.cbegin(); }

    iterator end() { return data.end(); }
    const_iterator end() const { return data.end(); }
    const_iterator cend() const { return data.cend(); }

    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }

    void erase(key_type i) { data.erase(i); }
    void erase(const_iterator it) { data.erase(it); }

    objectt &operator[](key_type i) { return data[i]; }
    objectt &at(key_type i) { return data.at(i); }
    const objectt &at(key_type i) const { return data.at(i); }

    template <typename It>
    void insert(It b, It e) { data.insert(b, e); }

    template <typename T>
    const_iterator find(T &&t) const { return data.find(std::forward<T>(t)); }

    static const object_map_dt blank;

    object_map_dt()=default;

  protected:
    ~object_map_dt()=default;
  };

  exprt to_expr(const object_map_dt::value_type &it) const;

  typedef reference_counting<object_map_dt> object_mapt;

  void set(object_mapt &dest, const object_map_dt::value_type &it) const
  {
    dest.write()[it.first]=it.second;
  }

  bool insert(object_mapt &dest, const object_map_dt::value_type &it) const
  {
    return insert(dest, it.first, it.second);
  }

  bool insert(object_mapt &dest, const exprt &src) const
  {
    return insert(dest, object_numbering.number(src), objectt());
  }

  bool insert(
    object_mapt &dest,
    const exprt &src,
    const mp_integer &offset) const
  {
    return insert(dest, object_numbering.number(src), objectt(offset));
  }

  bool insert(object_mapt &dest, unsigned n, const objectt &object) const;

  bool insert(object_mapt &dest, const exprt &expr, const objectt &object) const
  {
    return insert(dest, object_numbering.number(expr), object);
  }

  struct entryt
  {
    object_mapt object_map;
    idt identifier;
    std::string suffix;

    entryt()
    {
    }

    entryt(const idt &_identifier, const std::string &_suffix):
      identifier(_identifier),
      suffix(_suffix)
    {
    }
  };

  typedef std::set<exprt> expr_sett;

  typedef std::set<unsigned int> dynamic_object_id_sett;

  #ifdef USE_DSTRING
  typedef std::map<idt, entryt> valuest;
  #else
  typedef std::unordered_map<idt, entryt, string_hash> valuest;
  #endif

  void read_value_set(
    const exprt &expr,
    value_setst::valuest &dest,
    const namespacet &ns) const;

  expr_sett &get(
    const idt &identifier,
    const std::string &suffix);

  void make_any()
  {
    values.clear();
  }

  void clear()
  {
    values.clear();
  }

  entryt &get_entry(
    const entryt &e, const typet &type,
    const namespacet &);

  void output(
    const namespacet &ns,
    std::ostream &out) const;

  valuest values;

  // true = added something new
  bool make_union(object_mapt &dest, const object_mapt &src) const;

  // true = added something new
  bool make_union(const valuest &new_values);

  // true = added something new
  bool make_union(const value_sett &new_values)
  {
    return make_union(new_values.values);
  }

  void guard(
    const exprt &expr,
    const namespacet &ns);

  void apply_code(
    const codet &code,
    const namespacet &ns)
  {
    apply_code_rec(code, ns);
  }

  void assign(
    const exprt &lhs,
    const exprt &rhs,
    const namespacet &ns,
    bool is_simplified,
    bool add_to_sets);

  void do_function_call(
    const irep_idt &function,
    const exprt::operandst &arguments,
    const namespacet &ns);

  // edge back to call site
  void do_end_function(
    const exprt &lhs,
    const namespacet &ns);

  void read_reference_set(
    const exprt &expr,
    value_setst::valuest &dest,
    const namespacet &ns) const;

  bool eval_pointer_offset(
    exprt &expr,
    const namespacet &ns) const;

protected:
  void get_value_set(
    const exprt &expr,
    object_mapt &dest,
    const namespacet &ns,
    bool is_simplified) const;

  void get_reference_set(
    const exprt &expr,
    object_mapt &dest,
    const namespacet &ns) const
  {
    get_reference_set_rec(expr, dest, ns);
  }

  void get_reference_set_rec(
    const exprt &expr,
    object_mapt &dest,
    const namespacet &ns) const;

  void dereference_rec(
    const exprt &src,
    exprt &dest) const;

  void do_free(
    const exprt &op,
    const namespacet &ns);

  exprt make_member(
    const exprt &src,
    const irep_idt &component_name,
    const namespacet &ns);

  // Expression simplification:

private:
  /// Expression simplification function; by default, plain old
  /// util/simplify_expr, but can be customised by subclass.
  expr_simplifiert simplifier;

protected:
  /// Run registered expression simplifier
  void run_simplifier(exprt &e, const namespacet &ns)
  {
    simplifier(e, ns);
  }

  // Subclass customisation points:

protected:
  /// Subclass customisation point for recursion over RHS expression:
  virtual void get_value_set_rec(
    const exprt &expr,
    object_mapt &dest,
    const std::string &suffix,
    const typet &original_type,
    const namespacet &ns) const;

  /// Subclass customisation point for recursion over LHS expression:
  virtual void assign_rec(
    const exprt &lhs,
    const object_mapt &values_rhs,
    const std::string &suffix,
    const namespacet &ns,
    bool add_to_sets);

  /// Subclass customisation point for applying code to this domain:
  virtual void apply_code_rec(
    const codet &code,
    const namespacet &ns);

 private:
  /// Subclass customisation point to filter or otherwise alter the value-set
  /// returned from get_value_set before it is passed into assign. For example,
  /// this is used in one subclass to tag and thus differentiate values that
  /// originated in a particular place, vs. those that have been copied.
  virtual void adjust_assign_rhs_values(
    const exprt &rhs,
    const namespacet &ns,
    object_mapt &rhs_values) const
  {
  }

  /// Subclass customisation point to apply global side-effects to this domain,
  /// after RHS values are read but before they are written. For example, this
  /// is used in a recency-analysis plugin to demote existing most-recent
  /// objects to general case ones.
  virtual void apply_assign_side_effects(
    const exprt &lhs,
    const exprt &rhs,
    const namespacet &ns)
  {
  }
};

#endif // CPROVER_POINTER_ANALYSIS_VALUE_SET_H
