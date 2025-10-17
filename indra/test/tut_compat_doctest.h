#pragma once

/**
 * @file tut_compat_doctest.h
 * @brief Lightweight compatibility layer allowing generated TUT-style tests
 *        to build on top of doctest.
 *
 * This header is intended for auto-generated sources only. It maps the
 * most common TUT primitives to doctest while providing safe fallbacks.
 * Unsupported constructs should be replaced by the generator with explicit
 * DOCTEST_FAIL markers.
 */

#include "doctest.h"
#include "ll_doctest_helpers.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace tut_compat
{
class failure : public std::runtime_error
{
public:
    explicit failure(const char* message)
        : std::runtime_error(message ? message : "") {}
    explicit failure(const std::string& message)
        : std::runtime_error(message) {}
};

inline void ensure(bool condition)
{
    if (!condition)
    {
        throw failure("ensure() failed");
    }
}

inline void ensure(const char* message, bool condition)
{
    if (!condition)
    {
        throw failure(message ? message : "ensure() failed");
    }
}

template <typename Left, typename Right>
inline void ensure_equals(const Left& lhs, const Right& rhs)
{
    if (!(lhs == rhs))
    {
        throw failure("ensure_equals() failed");
    }
}

template <typename Left, typename Right>
inline void ensure_equals(const char* message, const Left& lhs, const Right& rhs)
{
    if (!(lhs == rhs))
    {
        throw failure(message ? message : "ensure_equals() failed");
    }
}

template <typename Expr>
inline void ensure_not(const Expr& value)
{
    if (value)
    {
        throw failure("ensure_not() failed");
    }
}

template <typename Expr>
inline void ensure_not(const char* message, const Expr& value)
{
    if (value)
    {
        throw failure(message ? message : "ensure_not() failed");
    }
}

template <typename Func>
inline void ensure_throws(Func&& fn)
{
    bool threw = false;
    try
    {
        fn();
    }
    catch (...)
    {
        threw = true;
    }
    if (!threw)
    {
        throw failure("ensure_throws() expected an exception");
    }
}

template <typename Func, typename Exception>
inline void ensure_throws(const char* message, Func&& fn, Exception)
{
    try
    {
        fn();
    }
    catch (const Exception&)
    {
        return;
    }
    catch (...)
    {
        throw failure(message ? message : "ensure_throws() threw unexpected exception type");
    }
    throw failure(message ? message : "ensure_throws() expected an exception");
}

inline void set_test_name(const char* name)
{
    INFO("test name: " << (name ? name : "<null>"));
}

inline void skip(const char* reason)
{
    INFO("skip requested: " << (reason ? reason : "<unspecified>"));
    DOCTEST_FAIL("TODO: original test requested skip");
}
} // namespace tut_compat

namespace tut_compat_detail
{
template <typename Range>
inline const Range& make_view(const Range& range)
{
    return range;
}

inline std::string_view make_view(const char* value)
{
    return value ? std::string_view(value) : std::string_view();
}

#ifdef _WIN32
inline std::wstring_view make_view(const wchar_t* value)
{
    return value ? std::wstring_view(value) : std::wstring_view();
}
#endif

template <typename Haystack, typename Needle>
bool contains(const Haystack& hay, const Needle& needle)
{
    const auto hay_view = make_view(hay);
    const auto needle_view = make_view(needle);
    auto begin_hay = std::begin(hay_view);
    auto end_hay = std::end(hay_view);
    auto begin_needle = std::begin(needle_view);
    auto end_needle = std::end(needle_view);
    if (begin_needle == end_needle)
    {
        return true;
    }
    return std::search(begin_hay, end_hay, begin_needle, end_needle) != end_hay;
}
} // namespace tut_compat_detail

template <typename Haystack, typename Needle>
inline void TUT_ENSURE_CONTAINS(const Haystack& hay, const Needle& needle)
{
    CHECK(tut_compat_detail::contains(hay, needle));
}

template <typename Message, typename Haystack, typename Needle>
inline void TUT_ENSURE_CONTAINS(const Message& message, const Haystack& hay, const Needle& needle)
{
    CHECK_MESSAGE(tut_compat_detail::contains(hay, needle), message);
}

#define TUT_ENSURE(...) ::tut_compat::ensure(__VA_ARGS__)
#define TUT_ENSURE_EQ(...) ::tut_compat::ensure_equals(__VA_ARGS__)
#define TUT_ENSURE_NOT(...) ::tut_compat::ensure_not(__VA_ARGS__)
#define TUT_ENSURE_THROWS(expr) ::tut_compat::ensure_throws([&]() { expr; })
#define TUT_ENSURE_APPROX(a, b, eps) LL_CHECK_APPROX((a), (b), (eps))
#define TUT_ENSURE_APPROX_NEAR(a, b, tol) LL_CHECK_NEAR((a), (b), (tol))
#define TUT_ENSURE_APPROX_RANGE(pa, pb, n, tol) LL_CHECK_EQ_RANGE_NEAR((pa), (pb), (n), (tol))
#define TUT_ENSURE_IN_RANGE(x, lo, hi) LL_CHECK_IN_RANGE((x), (lo), (hi))
#define TUT_ENSURE_MEMORY_MATCHES(a, b, len) LL_CHECK_EQ_MEM((a), (b), (len))
#define TUT_CHECK_MSG(cond, msg) ::tut_compat::ensure((msg), (cond))
#define TUT_SUITE(name) TEST_SUITE(name)
#define TUT_CASE(name) TEST_CASE(name)
#define TUT_SET_TEST_NAME(name) ::tut_compat::set_test_name(name)
#define TUT_SKIP(reason) ::tut_compat::skip(reason)
#ifdef _WIN32
#define TUT_ENSURE_WSTR_EQ(a, b) LL_CHECK_EQ_WSTR((a), (b))
#endif
