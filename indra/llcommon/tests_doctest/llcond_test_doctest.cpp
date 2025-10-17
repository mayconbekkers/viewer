// DOCTEST_SKIP_AUTOGEN: manual suite maintained in doctest
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "linden_common.h"
#include "llcond.h"
#include "llcoros.h"

namespace
{
class CoroScope
{
public:
    CoroScope() = default;
    CoroScope(const CoroScope&) = delete;
    CoroScope& operator=(const CoroScope&) = delete;
    ~CoroScope()
    {
        LLCoros::deleteSingleton();
    }
};
} // namespace

TEST_SUITE("llcond")
{
    TEST_CASE("Immediate gratification")
    {
        CoroScope scope_guard;
        LLScalarCond<int> cond{0};

        cond.set_one(1);
        CHECK(cond.wait_for_equal(F32Milliseconds(1), 1));
        CHECK_FALSE(cond.wait_for_unequal(F32Milliseconds(1), 1));
    }

    TEST_CASE("Simple two-coroutine test")
    {
        CoroScope scope_guard;
        LLScalarCond<int> cond{0};

        auto launch_name = LLCoros::instance().launch(
            "llcond_test_simple_two_coroutine",
            [&cond]() {
                CHECK_EQ(cond.get(), 0);
                cond.set_all(1);
                cond.wait_equal(2);
                CHECK_EQ(cond.get(), 2);
                cond.set_all(3);
            });
        (void)launch_name;

        CHECK_EQ(cond.get(), 1);
        cond.set_all(2);
        cond.wait_equal(3);
    }
}
