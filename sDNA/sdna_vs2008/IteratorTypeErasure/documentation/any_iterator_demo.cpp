//  (C) Copyright Thomas Becker 2005. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Revision History
// ================
//
// 7 Jan 2007 (Thomas Becker) Created

#ifdef _MSC_VER

  // Disable stupid warning about what Microsoft thinks is deprecated
  #pragma warning( disable : 4996 )

// This is for Intel 9.1. Strangely, _MSC_VER is defined when using
// Intel 9.1. Think about that later...
#pragma warning( disable : 597 )

#endif

// Includes
// ========

#include "../any_iterator/any_iterator.hpp"
#include<iterator>
#include<vector>
#include<set>
#include<iostream>

using namespace IteratorTypeErasure;

void any_iterator_demo()
{

  std::set<int> a_set;
  std::vector<int> a_vector;

// Overview
// ========

// For an overview of iterator type erasure, see 
//
// http://thbecker.net/free_software_utilities/type_erasure_for_cpp_iterators/start_page.html
//
// If you know what iterator type erasure is all about and what you want to use it for, then
// the following simple examples are all the documentation you need.
//
// IMPORTANT CAVEAT: Using type erasing classes in general and the any_iterator in particular is
// not without pitfalls. Read the warning in the documentation at
// 
// http://thbecker.net/free_software_utilities/type_erasure_for_cpp_iterators/any_iterator.html#caveat
//
// and/or look at the code example labeled "WARNING" at the end of this demo file.

// Basic Usage
// ===========

  // Below is a typedef for an any_iterator instantiation with the same traits as std::set<int>::const_iterator.
  // The way the traits are specified is exactly as in boost::iterator_facade.
  
  typedef any_iterator<
    int, // value type
    boost::bidirectional_traversal_tag, // traversal tag. Note: std iterator categories are supported here
    int const &, // reference type
    ptrdiff_t // difference type is irrelevant here, just don't use void, that'll throw the iterator_adaptor for a loop
  >
  bidirectional_const_iterator_to_int;

  // An object of this type, such as

  bidirectional_const_iterator_to_int a_bidirectional_const_iterator_to_int;

  // will accept any iterator whose traits convert elementwise to the traits of an std::set<int>::const_iterator.
  // Footnote: the statement above is in fact a simplification, but you'll be fine with it most of the time.

  // The following are examples of what you can and cannot assign to this any_iterator:
  
  std::set<int>::const_iterator i1 = a_set.begin(); // yes
  a_bidirectional_const_iterator_to_int = i1;

  std::set<int>::iterator i2 = a_set.begin(); // yes
  a_bidirectional_const_iterator_to_int = i2;

  std::vector<int>::const_iterator i3 = a_vector.begin(); // yes
  a_bidirectional_const_iterator_to_int = i3;

  std::vector<int>::iterator i4 = a_vector.begin(); // yes
  a_bidirectional_const_iterator_to_int = i4;

  std::vector<std::pair<int, int> >::iterator i5; // no, value type does not convert to int.
  std::istreambuf_iterator<char> eos; // no, std::input_iterator_tag does not convert to std::bidirectional_iterator_tag

// Note on Conversions
// ===================

  // In the above examples, the concrete iterators were assigned to any_iterator objects. You may also construct
  // any_iterator objects from concrete iterators:

  bidirectional_const_iterator_to_int another_bidirectional_const_iterator_to_int(a_set.begin());

  // However, the any_iterator constructor from a concrete iterator is explicit, so the following
  // would not compile:

  // bidirectional_const_iterator_to_int another_bidirectional_const_iterator_to_int = a_set.begin();

  // The fact that the any_iterator constructor from a concrete iterator is explicit is unnatural. This
  // is currently not fixable because of an arcane limitation of the SFINAE principle. The problem will
  // go away once C++ concepts are available.

// Metafunction make_any_iterator_type
// ===================================

  // To simplify the declarations of any_iterators, there is a metafunction that takes an iterator type and makes an
  // any_iterator type with the same iterator traits. For example,
  
  typedef make_any_iterator_type<std::set<int>::iterator>::type bidirectional_iterator_to_int;
  
  // is similar to the type bidirectional_const_iterator_to_int above. The difference is that this one is non-const
  // and will therefore not accept any const iterators.
 
  bidirectional_iterator_to_int a_bidirectional_iterator_to_int;
  a_bidirectional_iterator_to_int = i2;
  a_bidirectional_iterator_to_int = i4;

// Input and Output Iterators
// ==========================

  // The metafunction make_any_iterator_type won't work for many input and output iterators that are currently in
  // the standard library. That's because these iterators often define their traits in a way that is very hard to
  // make sense of. You probably won't be using type erasure with input and outpur iterators anyway, but if you
  // must, here are two examples that show you what the real traits of these iterators are:

  typedef any_iterator<
    char,
    boost::single_pass_traversal_tag,
    char const,
    ptrdiff_t
  > any_input_iterator_to_char;
  //
  std::istreambuf_iterator<char> std_in_it(std::cin.rdbuf());
  any_input_iterator_to_char an_input_iterator_to_char(std_in_it);

  typedef any_iterator<
    char,
    boost::incrementable_traversal_tag,
    std::ostreambuf_iterator<char, std::char_traits<char> > &,
    ptrdiff_t
  > any_output_iterator_to_char;
  //
  std::ostreambuf_iterator<char> std_out_it(std::cout.rdbuf());
  any_output_iterator_to_char an_output_iterator_to_char(std_out_it);

// WARNING
// =======

  // Suppose you have two concrete iterators that are interoperable, but of different
  // type, like this:
  //
  std::vector<int> int_vector;
  std::vector<int>::iterator it = int_vector.begin();
  std::vector<int>::const_iterator cit = int_vector.begin();
  it == cit; // evaluates to true

  // If you now wrap these two iterators into any_iterator's of the same type, like this:
  
  typedef any_iterator<int, boost::random_access_traversal_tag,  int const &> random_access_const_iterator_to_int;
  random_access_const_iterator_to_int ait_1(it);
  random_access_const_iterator_to_int ait_2(cit);

  // then their interoperability is lost:
    
  // ait_1 == ait_2; // Bad comparison! Behaves like comparing completely unrelated iterators!

  // This behavior is most certainly highly undesirable and extremely dangerous. In fact, it is so bad
  // that I had at one point decided to declare the idea of the any_iterator infeasible.
  // But then it occurred to me that this pitfall is not entirely specific to the any_iterator.
  // It occurs with any type-erasing class that implements binary operators. Therefore, it is perhaps
  // beneficial to put all this out there and alert people to the problem.

}
