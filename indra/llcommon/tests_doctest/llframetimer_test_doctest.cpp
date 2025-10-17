// DOCTEST_SKIP_AUTOGEN: manual suite maintained in doctest
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "linden_common.h"
#include "llframetimer.h"

namespace
{
class FrameTimeSeed
{
public:
    FrameTimeSeed() { LLFrameTimer::updateFrameTime(); }
};

constexpr F64 kToleranceSeconds = 0.001;
} // namespace

TEST_SUITE("llframetimer_test")
{
    TEST_CASE("set expiry matches get expiry at current time")
    {
        FrameTimeSeed seed;
        const F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
        LLFrameTimer timer;
        timer.setExpiryAt(seconds_since_epoch);
        LL_CHECK_NEAR(timer.expiresAt(), seconds_since_epoch, kToleranceSeconds);
    }

    TEST_CASE("set expiry matches get expiry at future times")
    {
        FrameTimeSeed seed;
        F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
        LLFrameTimer timer;

        seconds_since_epoch += 10.0;
        timer.setExpiryAt(seconds_since_epoch);
        LL_CHECK_NEAR(timer.expiresAt(), seconds_since_epoch, kToleranceSeconds);

        seconds_since_epoch += 10.0;
        timer.setExpiryAt(seconds_since_epoch);
        LL_CHECK_NEAR(timer.expiresAt(), seconds_since_epoch, kToleranceSeconds);
    }

    TEST_CASE("timer expires within expected iterations")
    {
        FrameTimeSeed seed;
        const clock_t t1 = clock();
        ms_sleep(200);
        const clock_t t2 = clock();
        const clock_t elapsed = t2 - t1 + 1;
        INFO("ms_sleep() duration (ms): " << static_cast<long>(elapsed));

        F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds() + 2.0;
        LLFrameTimer timer;
        timer.setExpiryAt(seconds_since_epoch);

        int iterations_until_expiration = 0;
        while (!timer.hasExpired())
        {
            ms_sleep(200);
            LLFrameTimer::updateFrameTime();
            ++iterations_until_expiration;
        }
        CHECK(iterations_until_expiration <= 10);
    }

    TEST_CASE("empty regression placeholder")
    {
        CHECK(true);
    }
}
