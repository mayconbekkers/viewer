#pragma once

/**
 * @file ll_doctest_helpers.h
 * @brief Reusable assertion helpers for doctest-based unit tests.
 *
 * Usage guidelines:
 * - Prefer `LL_CHECK_MSG` when you want to attach a readable failure message to a boolean expression.
 *   Example:
 *     LL_CHECK_MSG(result.success(), "login should succeed with valid credentials");
 *
 * - Use `LL_CHECK_APPROX` for floating-point comparisons that require a tolerance.
 *   Example:
 *     LL_CHECK_APPROX(actual_value, expected_value, 1.0e-5);
 *
 * - Employ `LL_CHECK_EQ_RANGE` when validating contiguous buffers or array contents.
 *   It will emit the first mismatching index to ease debugging.
 *   Example:
 *     LL_CHECK_EQ_RANGE(buffer_a.data(), buffer_b.data(), buffer_a.size());
 */

#include "doctest.h"

#include <cmath>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace ll::test::detail
{
template <typename TLeft, typename TRight>
inline void check_range_equal(
    const TLeft* lhs,
    const TRight* rhs,
    std::size_t length,
    const char* lhs_expr,
    const char* rhs_expr)
{
    bool match = true;
    std::size_t mismatch_index = 0;

    for (std::size_t index = 0; index < length; ++index)
    {
        if (!(lhs[index] == rhs[index]))
        {
            match = false;
            mismatch_index = index;
            break;
        }
    }

    if (!match)
    {
        std::ostringstream description;
        description << lhs_expr << "[" << mismatch_index << "] ("
                    << lhs[mismatch_index] << ") differs from "
                    << rhs_expr << "[" << mismatch_index << "] ("
                    << rhs[mismatch_index] << ")";
        INFO("range length: " << length);
        INFO("mismatch index: " << mismatch_index);
        CHECK_MESSAGE(false, description.str());
        return;
    }

    CHECK(match);
}
} // namespace ll::test::detail

inline void ensure_memory_matches(
    const char* message,
    const void* actual,
    unsigned int actual_len,
    const void* expected,
    unsigned int expected_len)
{
    const char* prefix = (message && message[0]) ? message : "";
    if (actual_len != expected_len)
    {
        std::ostringstream description;
        if (prefix[0] != '\0')
        {
            description << prefix << ": ";
        }
        description << "expected " << expected_len << " bytes but received " << actual_len;
        CHECK_MESSAGE(false, description.str());
        return;
    }
    const bool match = (std::memcmp(actual, expected, actual_len) == 0);
    if (!match)
    {
        std::ostringstream description;
        if (prefix[0] != '\0')
        {
            description << prefix << ": ";
        }
        description << "buffers differ";
        CHECK_MESSAGE(false, description.str());
        return;
    }
    CHECK(match);
}

inline void ensure_memory_matches(
    const void* actual,
    unsigned int actual_len,
    const void* expected,
    unsigned int expected_len)
{
    ensure_memory_matches("", actual, actual_len, expected, expected_len);
}

namespace doctest
{
template <typename T>
struct StringMaker<std::vector<T>>
{
    static String convert(const std::vector<T>& values)
    {
        std::ostringstream out;
        out << "[";
        bool first = true;
        for (const auto& entry : values)
        {
            if (!first)
            {
                out << ", ";
            }
            first = false;
            out << StringMaker<T>::convert(entry).c_str();
        }
        out << "]";
        return String(out.str().c_str());
    }
};
} // namespace doctest

#define LL_CHECK_NEAR(actual, expected, abs_tol) \
    CHECK(std::fabs((actual) - (expected)) <= (abs_tol))

template <typename Actual, typename Expected, typename Epsilon>
inline void ll_check_approx_impl(const Actual& actual, const Expected& expected, const Epsilon& epsilon)
{
    CHECK(actual == doctest::Approx(expected).epsilon(epsilon));
}

#define LL_CHECK_APPROX(actual, expected, epsilon) \
    ll_check_approx_impl((actual), (expected), (epsilon))

template <typename ItA, typename ItB>
inline void LL_CHECK_EQ_RANGE_NEAR(ItA a, ItB b, std::size_t n, double abs_tol)
{
    for (std::size_t i = 0; i < n; ++i, ++a, ++b)
    {
        CHECK(std::fabs(double(*a) - double(*b)) <= abs_tol);
    }
}

inline void LL_CHECK_EQ_STR(const std::string& a, const std::string& b)
{
    CHECK(a == b);
}

inline void LL_CHECK_EQ_MEM(const void* a, const void* b, std::size_t len)
{
    const auto* pa = static_cast<const unsigned char*>(a);
    const auto* pb = static_cast<const unsigned char*>(b);
    for (std::size_t i = 0; i < len; ++i)
    {
        CHECK(pa[i] == pb[i]);
    }
}

inline std::string normalize_separators(std::string value)
{
    for (char& ch : value)
    {
        if (ch == '\\')
        {
            ch = '/';
        }
    }
    return value;
}

#ifdef _WIN32
inline void LL_CHECK_EQ_WSTR(const std::wstring& a, const std::wstring& b)
{
    CHECK(a == b);
}
#endif

#define LL_CHECK_EQ_RANGE(ptrA, ptrB, len)                                                                  \
    do                                                                                                      \
    {                                                                                                       \
        const auto* _ll_ptrA = (ptrA);                                                                      \
        const auto* _ll_ptrB = (ptrB);                                                                      \
        const std::size_t _ll_length = static_cast<std::size_t>(len);                                       \
        ::ll::test::detail::check_range_equal(_ll_ptrA, _ll_ptrB, _ll_length, #ptrA, #ptrB);                \
    } while (false)

#define LL_CHECK_IN_RANGE(x, lo, hi)                                                                        \
    do                                                                                                      \
    {                                                                                                       \
        CHECK((x) >= (lo));                                                                                 \
        CHECK((x) <= (hi));                                                                                 \
    } while (false)

#define LL_CHECK_MSG(condition, message) CHECK_MESSAGE((condition), (message))
