//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CXX03___MEMORY_SWAP_ALLOCATOR_H
#define _LIBCPP___CXX03___MEMORY_SWAP_ALLOCATOR_H

#include <__cxx03/__config>
#include <__cxx03/__memory/allocator_traits.h>
#include <__cxx03/__type_traits/integral_constant.h>
#include <__cxx03/__type_traits/is_swappable.h>
#include <__cxx03/__utility/swap.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

template <typename _Alloc>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX14 void __swap_allocator(_Alloc& __a1, _Alloc& __a2, true_type)
    _NOEXCEPT_(__is_nothrow_swappable_v<_Alloc>) {
  using std::swap;
  swap(__a1, __a2);
}

template <typename _Alloc>
inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX14 void
__swap_allocator(_Alloc&, _Alloc&, false_type) _NOEXCEPT {}

template <typename _Alloc>
inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX14 void __swap_allocator(_Alloc& __a1, _Alloc& __a2)
    _NOEXCEPT_(__is_nothrow_swappable_v<_Alloc>) {
  std::__swap_allocator(
      __a1, __a2, integral_constant<bool, allocator_traits<_Alloc>::propagate_on_container_swap::value>());
}

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___CXX03___MEMORY_SWAP_ALLOCATOR_H
