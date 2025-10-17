// ---------------------------------------------------------------------------
// Focused doctest coverage for LLVector4 operations.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "linden_common.h"

#include "llsd.h"
#include "../m4math.h"
#include "../v4math.h"
#include "../llquaternion.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace
{
constexpr F32 kTol = 1.0e-6f;
constexpr F32 kNormTol = 5.0e-6f;

void expect_vec4(const LLVector4& value,
                 F32 x,
                 F32 y,
                 F32 z,
                 F32 w,
                 std::string_view label,
                 F32 tol = kTol)
{
    INFO(label << " VX");
    LL_CHECK_NEAR(value.mV[VX], x, tol);
    INFO(label << " VY");
    LL_CHECK_NEAR(value.mV[VY], y, tol);
    INFO(label << " VZ");
    LL_CHECK_NEAR(value.mV[VZ], z, tol);
    INFO(label << " VW");
    LL_CHECK_NEAR(value.mV[VW], w, tol);
}
} // namespace

TEST_SUITE("v4math_test")
{
    TEST_CASE("ConstructorsAndClear")
    {
        LLVector4 default_vec;
        expect_vec4(default_vec, 0.f, 0.f, 0.f, 1.f, "default ctor");

        const LLVector4 xyz(10.f, -2.3f, -0.023f);
        expect_vec4(xyz, 10.f, -2.3f, -0.023f, 1.f, "xyz ctor");

        const LLVector4 xyzw(10.f, -2.3f, -0.023f, -2.f);
        expect_vec4(xyzw, 10.f, -2.3f, -0.023f, -2.f, "xyzw ctor");

        const F32 raw[4] = {0.112f, 23.2f, -4.2f, -0.0001f};
        const LLVector4 from_ptr(raw);
        expect_vec4(from_ptr, raw[0], raw[1], raw[2], raw[3], "ptr ctor");

        const LLVector3 base(-2.23f, 1.01f, 42.3f);
        expect_vec4(LLVector4(base), base.mV[VX], base.mV[VY], base.mV[VZ], 1.f, "vec3 ctor");
        expect_vec4(LLVector4(base, -0.234f), base.mV[VX], base.mV[VY], base.mV[VZ], -0.234f, "vec3+w ctor");

        LLVector4 work;
        work.setVec(1.f, 2.f, 3.f);
        work.clearVec();
        expect_vec4(work, 0.f, 0.f, 0.f, 1.f, "clearVec");

        work.zeroVec();
        expect_vec4(work, 0.f, 0.f, 0.f, 0.f, "zeroVec");
    }

    TEST_CASE("SettersAndCopies")
    {
        LLVector4 vec;
        vec.setVec(1.f, -2.f, 3.f);
        expect_vec4(vec, 1.f, -2.f, 3.f, 1.f, "setVec xyz");

        vec.setVec(1.f, -2.f, 3.f, -4.f);
        expect_vec4(vec, 1.f, -2.f, 3.f, -4.f, "setVec xyzw");

        const LLVector3 base(5.f, 6.f, -7.f);
        vec.setVec(base);
        expect_vec4(vec, base.mV[VX], base.mV[VY], base.mV[VZ], 1.f, "setVec(vec3)");

        vec.setVec(base, 9.f);
        expect_vec4(vec, base.mV[VX], base.mV[VY], base.mV[VZ], 9.f, "setVec(vec3,w)");

        const F32 raw[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        vec.setVec(raw);
        expect_vec4(vec, raw[0], raw[1], raw[2], raw[3], "setVec(ptr)");
    }

    TEST_CASE("MagnitudeAndNormalization")
    {
        LLVector4 vec(10.f, -2.3f, -0.023f);
        const F32 length_sq = vec.magVecSquared();
        LL_CHECK_APPROX(length_sq, 10.f * 10.f + 2.3f * 2.3f + 0.023f * 0.023f, kTol);
        LL_CHECK_APPROX(vec.magVec(), std::sqrt(length_sq), kTol);

        const LLVector4 original = vec;
        const F32 norm_length = vec.normVec();
        LL_CHECK_APPROX(norm_length, std::sqrt(length_sq), kNormTol);
        expect_vec4(vec,
                    original.mV[VX] / norm_length,
                    original.mV[VY] / norm_length,
                    original.mV[VZ] / norm_length,
                    1.f,
                    "normVec",
                    kNormTol);
    }

    TEST_CASE("ArithmeticOperators")
    {
        const LLVector4 lhs(1.f, 2.f, -1.1f);
        const LLVector4 rhs(1.2f, 2.5f, 1.f);

        expect_vec4(lhs + rhs, 2.2f, 4.5f, -0.1f, 1.f, "operator+");
        expect_vec4(lhs - rhs, -0.2f, -0.5f, -2.1f, 1.f, "operator-");

        LLVector4 work = lhs;
        work += rhs;
        expect_vec4(work, 2.2f, 4.5f, -0.1f, 1.f, "+=");

        work = lhs;
        work -= rhs;
        expect_vec4(work, -0.2f, -0.5f, -2.1f, 1.f, "-=");

        work = lhs;
        work *= 2.f;
        expect_vec4(work, 2.f, 4.f, -2.2f, 1.f, "*=");

        work = lhs;
        work /= 4.f;
        expect_vec4(work, 0.25f, 0.5f, -0.275f, 1.f, "/=", kTol);

        expect_vec4(lhs / 4.2f,
                    lhs.mV[VX] / 4.2f,
                    lhs.mV[VY] / 4.2f,
                    lhs.mV[VZ] / 4.2f,
                    1.f,
                    "operator/");
    }

    TEST_CASE("DotAndCrossProducts")
    {
        const LLVector4 lhs(2.f, 3.f, 4.f);
        const LLVector4 rhs(-1.f, 0.5f, 6.f);

        LL_CHECK_APPROX(lhs * rhs,
                        lhs.mV[VX] * rhs.mV[VX] + lhs.mV[VY] * rhs.mV[VY] + lhs.mV[VZ] * rhs.mV[VZ],
                        kTol);

        LLVector4 cross = lhs % rhs;
        expect_vec4(cross,
                    lhs.mV[VY] * rhs.mV[VZ] - rhs.mV[VY] * lhs.mV[VZ],
                    lhs.mV[VZ] * rhs.mV[VX] - rhs.mV[VZ] * lhs.mV[VX],
                    lhs.mV[VX] * rhs.mV[VY] - rhs.mV[VX] * lhs.mV[VY],
                    1.f,
                    "cross result");

        LLVector4 inplace = lhs;
        inplace %= rhs;
        CHECK(inplace == cross);
    }

    TEST_CASE("ComparisonOperators")
    {
        LLVector4 first(1.f, 2.f, -1.1f);
        LLVector4 second = first;
        LLVector4 third(1.f, 2.f, -1.05f);

        CHECK(first == second);
        CHECK_FALSE(first != second);
        CHECK(first != third);

        LLVector4 neg = -first;
        CHECK(first == -neg);
    }

    TEST_CASE("ParallelAndAngles")
    {
        const LLVector4 base(1.f, 2.f, -1.1f);
        LLVector4 scaled = base;
        scaled *= 2.f;
        CHECK(are_parallel(base, scaled, 0.001f));

        const LLVector4 other(21.f, 12.f, -123.1f);
        CHECK_FALSE(are_parallel(base, other, 0.001f));

        const F32 angle_same = angle_between(base, scaled);
        LL_CHECK_NEAR(angle_same, 0.f, 1.0e-3f);

        const LLVector4 first(1.f, 2.f, -1.1f);
        const LLVector4 second(21.f, 2.23f, -1.1f);

        LLVector4 n1 = first;
        LLVector4 n2 = second;
        n1.normVec();
        n2.normVec();
        const F32 clamped_dot = std::clamp(n1 * n2, -1.f, 1.f);
        const F32 expected = std::acos(clamped_dot);
        LL_CHECK_NEAR(angle_between(first, second), expected, 1.0e-4f);
    }

    TEST_CASE("DistancesAndLerp")
    {
        const LLVector4 a(-2.3f, 2.f, 1.2f, -0.23f);
        const LLVector4 b(1.3f, 1.f, 1.f, 0.12f);

        const F32 diff_sq =
            (a.mV[VX] - b.mV[VX]) * (a.mV[VX] - b.mV[VX]) +
            (a.mV[VY] - b.mV[VY]) * (a.mV[VY] - b.mV[VY]) +
            (a.mV[VZ] - b.mV[VZ]) * (a.mV[VZ] - b.mV[VZ]);

        LL_CHECK_APPROX(dist_vec(a, b), std::sqrt(diff_sq), kTol);
        LL_CHECK_APPROX(dist_vec_squared(a, b), diff_sq, kTol);

        const F32 t = 0.25f;
        const LLVector4 lerped = lerp(a, b, t);
        expect_vec4(lerped,
                    a.mV[VX] + (b.mV[VX] - a.mV[VX]) * t,
                    a.mV[VY] + (b.mV[VY] - a.mV[VY]) * t,
                    a.mV[VZ] + (b.mV[VZ] - a.mV[VZ]) * t,
                    a.mV[VW] + (b.mV[VW] - a.mV[VW]) * t,
                    "lerp");
    }

    TEST_CASE("Conversions")
    {
        const LLVector4 vec(1.f, 2.f, -1.1f, 0.5f);

        const LLVector3 as_vec3 = vec4to3(vec);
        expect_vec4(vec3to4(as_vec3), vec.mV[VX], vec.mV[VY], vec.mV[VZ], 1.f, "vec3to4");

        const LLSD packed = vec.getValue();
        const LLVector3 unpacked(packed);
        const LLVector4 roundtrip = vec3to4(unpacked);
        expect_vec4(roundtrip, vec.mV[VX], vec.mV[VY], vec.mV[VZ], 1.f, "LLSD roundtrip");
    }
}
