// ---------------------------------------------------------------------------
// Focused doctest coverage for LLVector2 operations.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "linden_common.h"

#include "../v2math.h"

#include <cmath>
#include <sstream>
#include <string_view>

namespace
{
constexpr F32 kTol = 1.0e-6f;
constexpr F32 kNormTol = 5.0e-6f;

void expect_vec2(const LLVector2& value,
                 F32 expected_x,
                 F32 expected_y,
                 std::string_view label,
                 F32 tol = kTol)
{
    INFO(label << " VX");
    LL_CHECK_NEAR(value.mV[VX], expected_x, tol);
    INFO(label << " VY");
    LL_CHECK_NEAR(value.mV[VY], expected_y, tol);
}
} // namespace

TEST_SUITE("v2math_test")
{
    TEST_CASE("ConstructorsAndReset")
    {
        LLVector2 vec_default;
        expect_vec2(vec_default, 0.f, 0.f, "default ctor");

        const LLVector2 vec_xy(2.f, -3.2f);
        expect_vec2(vec_xy, 2.f, -3.2f, "xy ctor");

        const F32 raw[2] = {3.2f, 4.5f};
        const LLVector2 vec_raw(raw);
        expect_vec2(vec_raw, raw[0], raw[1], "ptr ctor");

        LLVector2 work = vec_raw;
        work.clearVec();
        expect_vec2(work, 0.f, 0.f, "clearVec");

        work.setVec(5.f, -6.5f);
        work.zeroVec();
        expect_vec2(work, 0.f, 0.f, "zeroVec");
    }

    TEST_CASE("SettersAndCopy")
    {
        LLVector2 vec;
        vec.setVec(1.23f, -8.9f);
        expect_vec2(vec, 1.23f, -8.9f, "setVec(x,y)");

        LLVector2 copy;
        copy.setVec(vec);
        CHECK(copy == vec);

        const F32 raw[2] = {0.001f, -0.5f};
        copy.setVec(raw);
        expect_vec2(copy, raw[0], raw[1], "setVec(ptr)");
    }

    TEST_CASE("MagnitudeAndNullChecks")
    {
        const LLVector2 vec(2.2345f, 3.5678f);
        const F32 expected_sq = vec.mV[VX] * vec.mV[VX] + vec.mV[VY] * vec.mV[VY];
        LL_CHECK_APPROX(vec.magVecSquared(), expected_sq, kTol);
        LL_CHECK_APPROX(vec.magVec(), std::sqrt(expected_sq), kTol);

        LLVector2 signed_vec(-2.f, -3.f);
        CHECK(signed_vec.abs());
        expect_vec2(signed_vec, 2.f, 3.f, "abs()");
        CHECK_FALSE(signed_vec.isNull());

        signed_vec.setVec(1.0e-8f, 1.001e-6f);
        CHECK(signed_vec.isNull());
    }

    TEST_CASE("ScaleAndZeroDetection")
    {
        LLVector2 vec;
        vec.setVec(1.f, 2.f);
        LLVector2 scaled = vec;
        scaled.scaleVec(vec); // multiplying component-wise by itself.
        expect_vec2(scaled, 1.f, 4.f, "scaleVec");
        CHECK_FALSE(scaled.isExactlyZero());

        LLVector2 zeros;
        zeros.scaleVec(vec);
        expect_vec2(zeros, 0.f, 0.f, "scaleVec zero");
        CHECK(zeros.isExactlyZero());
    }

    TEST_CASE("VectorArithmeticOperators")
    {
        const LLVector2 a(1.f, 2.f);
        const LLVector2 b(-2.3f, 1.11f);

        expect_vec2(a + b, -1.3f, 3.11f, "operator+");
        expect_vec2(a - b, 3.3f, 0.89f, "operator-");

        LLVector2 work = a;
        work += b;
        expect_vec2(work, -1.3f, 3.11f, "+=");

        work = a;
        work -= b;
        expect_vec2(work, 3.3f, 0.89f, "-=");

        work = a;
        work *= 2.f;
        expect_vec2(work, 2.f, 4.f, "*= scalar");

        work = a;
        work /= 4.f;
        expect_vec2(work, 0.25f, 0.5f, "/= scalar", kTol);
    }

    TEST_CASE("ScalarDivideOperators")
    {
        const LLVector2 vec(0.213f, -2.34f);
        expect_vec2(vec / -0.23f,
                    vec.mV[VX] / -0.23f,
                    vec.mV[VY] / -0.23f,
                    "operator/");
    }

    TEST_CASE("DotAndCrossProducts")
    {
        const LLVector2 lhs(1.f, 2.f);
        const LLVector2 rhs(-2.3f, 1.11f);

        LL_CHECK_APPROX(lhs * rhs, lhs.mV[VX] * rhs.mV[VX] + lhs.mV[VY] * rhs.mV[VY], kTol);

        LLVector2 cross = lhs;
        cross %= rhs;
        expect_vec2(cross,
                    lhs.mV[VX] * rhs.mV[VY] - rhs.mV[VX] * lhs.mV[VY],
                    lhs.mV[VY] * rhs.mV[VX] - rhs.mV[VY] * lhs.mV[VX],
                    "%=");
    }

    TEST_CASE("ComparisonAndIndexing")
    {
        const LLVector2 a(1.f, 2.f);
        const LLVector2 b(1.f, 2.f);
        const LLVector2 c(-0.5f, 3.f);

        CHECK(a == b);
        CHECK_FALSE(a != b);
        CHECK(c < a);
        CHECK_FALSE(a < c);

        CHECK(a[0] == doctest::Approx(1.f));
        CHECK(a[1] == doctest::Approx(2.f));
    }

    TEST_CASE("StreamInsertion")
    {
        const LLVector2 vec(1.f, 2.f);
        std::ostringstream first;
        first << vec;

        std::ostringstream second;
        LLVector2 copy = vec;
        second << copy;

        CHECK(first.str() == second.str());
    }

    TEST_CASE("DistanceFunctions")
    {
        const LLVector2 a(1.f, 2.f);
        const LLVector2 b(-0.32f, 0.2234f);

        const F32 expected_sq = (a.mV[VX] - b.mV[VX]) * (a.mV[VX] - b.mV[VX]) +
                                (a.mV[VY] - b.mV[VY]) * (a.mV[VY] - b.mV[VY]);

        LL_CHECK_APPROX(dist_vec_squared2D(a, b), expected_sq, kTol);
        LL_CHECK_APPROX(dist_vec_squared(a, b), expected_sq, kTol);
        LL_CHECK_APPROX(dist_vec(a, b), std::sqrt(expected_sq), kTol);
    }

    TEST_CASE("LerpAndNormalization")
    {
        const LLVector2 start(1.f, 2.f);
        const LLVector2 end(-0.32f, 0.2234f);
        const F32 t = 0.0121f;

        LLVector2 lerped = lerp(start, end, t);
        expect_vec2(lerped,
                    start.mV[VX] + (end.mV[VX] - start.mV[VX]) * t,
                    start.mV[VY] + (end.mV[VY] - start.mV[VY]) * t,
                    "lerp");

        LLVector2 norm = start;
        const F32 magnitude = norm.normVec();
        LL_CHECK_APPROX(magnitude, std::sqrt(start.mV[VX] * start.mV[VX] + start.mV[VY] * start.mV[VY]), kNormTol);
        expect_vec2(norm,
                    start.mV[VX] / magnitude,
                    start.mV[VY] / magnitude,
                    "normVec normalized",
                    kNormTol);

        norm.setVec(1.0e-8f, 0.f);
        const F32 degenerate = norm.normVec();
        CHECK(degenerate == 0.f);
        expect_vec2(norm, 0.f, 0.f, "normVec degenerate");
    }
}
