#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"

#include <string>

TEST_SUITE("ll_doctest_helpers")
{
    TEST_CASE("memory comparison matches")
    {
        const unsigned char lhs[] = {1u, 2u, 3u, 4u};
        const unsigned char rhs[] = {1u, 2u, 3u, 4u};
        LL_CHECK_EQ_MEM(lhs, rhs, sizeof(lhs));
    }

    TEST_CASE("value inside range")
    {
        const int value = 42;
        LL_CHECK_IN_RANGE(value, 0, 100);
    }

    TEST_CASE("approximate equality")
    {
        const double sum = 0.1 + 0.2;
        TUT_ENSURE_APPROX(sum, 0.3, 1.0e-9);
    }

    TEST_CASE("near equality within tolerance")
    {
        LL_CHECK_NEAR(10.0, 10.0005, 0.001);
    }

    TEST_CASE("string equality")
    {
        LL_CHECK_EQ_STR(std::string("hello"), std::string("hello"));
    }

#ifdef _WIN32
    TEST_CASE("wide string equality")
    {
        LL_CHECK_EQ_WSTR(std::wstring(L"world"), std::wstring(L"world"));
    }
#endif
}
