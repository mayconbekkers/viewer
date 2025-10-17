// ---------------------------------------------------------------------------
// Hand-authored doctest suite covering LLMatrix3 operations.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "linden_common.h"

#include "../m3math.h"
#include "../v3math.h"
#include "../v3dmath.h"
#include "../llquaternion.h"

#include <array>
#include <string_view>

namespace
{
constexpr F32 kTolerance = 1.0e-6f;

LLMatrix3 make_matrix3(const std::array<std::array<F32, 3>, 3>& values)
{
    LLMatrix3 result;
    result.setZero();
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 3; ++c)
        {
            result.mMatrix[r][c] = values[r][c];
        }
    }
    return result;
}

void expect_matrix3_equal(const LLMatrix3& actual,
                          const std::array<std::array<F32, 3>, 3>& expected_values,
                          std::string_view message)
{
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 3; ++c)
        {
            INFO(message << " [" << r << "][" << c << "]");
            LL_CHECK_APPROX(actual.mMatrix[r][c], expected_values[r][c], kTolerance);
        }
    }
}

void expect_vector3_equal(const LLVector3& actual, const LLVector3& expected, std::string_view message)
{
    for (int i = 0; i < 3; ++i)
    {
        INFO(message << " index " << i);
        LL_CHECK_APPROX(actual.mV[i], expected.mV[i], kTolerance);
    }
}
} // namespace

TEST_SUITE("llmatrix3_test")
{
    TEST_CASE("setIdentity produces the identity matrix")
    {
        LLMatrix3 mat;
        mat.setZero();
        mat.setIdentity();

        constexpr std::array<std::array<F32, 3>, 3> expected = {{
            {1.f, 0.f, 0.f},
            {0.f, 1.f, 0.f},
            {0.f, 0.f, 1.f},
        }};
        expect_matrix3_equal(mat, expected, "setIdentity");
    }

    TEST_CASE("setZero clears all components")
    {
        LLMatrix3 mat = make_matrix3({{
            {1.f, 2.f, 3.f},
            {4.f, 5.f, 6.f},
            {7.f, 8.f, 9.f},
        }});
        mat.setZero();

        constexpr std::array<std::array<F32, 3>, 3> expected = {{
            {0.f, 0.f, 0.f},
            {0.f, 0.f, 0.f},
            {0.f, 0.f, 0.f},
        }};
        expect_matrix3_equal(mat, expected, "setZero");
    }

    TEST_CASE("setRows assigns rows and getters return them")
    {
        const LLVector3 row0(2.f, 1.f, 4.f);
        const LLVector3 row1(3.f, 5.f, 7.f);
        const LLVector3 row2(6.f, 9.f, 7.f);

        LLMatrix3 mat;
        mat.setRows(row0, row1, row2);

        constexpr std::array<std::array<F32, 3>, 3> expected = {{
            {2.f, 1.f, 4.f},
            {3.f, 5.f, 7.f},
            {6.f, 9.f, 7.f},
        }};
        expect_matrix3_equal(mat, expected, "setRows");
        expect_vector3_equal(mat.getFwdRow(), row0, "getFwdRow");
        expect_vector3_equal(mat.getLeftRow(), row1, "getLeftRow");
        expect_vector3_equal(mat.getUpRow(), row2, "getUpRow");
    }

    TEST_CASE("matrix multiplication produces expected values")
    {
        const LLMatrix3 lhs = make_matrix3({{
            {1.f, 3.f, 5.f},
            {3.f, 6.f, 1.f},
            {4.f, 6.f, 9.f},
        }});
        const LLMatrix3 rhs = make_matrix3({{
            {1.f, 1.f, 5.f},
            {3.f, 6.f, 8.f},
            {8.f, 6.f, 2.f},
        }});
        const LLMatrix3 result = lhs * rhs;

        constexpr std::array<std::array<F32, 3>, 3> expected = {{
            {50.f, 49.f, 39.f},
            {29.f, 45.f, 65.f},
            {94.f, 94.f, 86.f},
        }};
        expect_matrix3_equal(result, expected, "matrix multiply");
    }

    TEST_CASE("vector multiplication mixes rows and columns")
    {
        const LLMatrix3 mat = make_matrix3({{
            {1.f, 3.f, 5.f},
            {3.f, 6.f, 1.f},
            {4.f, 6.f, 9.f},
        }});
        const LLVector3 vec(1.f, 3.f, 5.f);
        const LLVector3 expected(30.f, 51.f, 53.f);

        const LLVector3 result = vec * mat;
        expect_vector3_equal(result, expected, "vector * matrix");
    }

    TEST_CASE("vector3d multiplication uses same semantics")
    {
        const LLMatrix3 mat = make_matrix3({{
            {1.f, 3.f, 5.f},
            {3.f, 2.f, 1.f},
            {4.f, 6.f, 0.f},
        }});
        const LLVector3d vec(0.0, 3.0, 4.0);
        const LLVector3d expected(25.0, 30.0, 3.0);

        const LLVector3d result = vec * mat;
        for (int i = 0; i < 3; ++i)
        {
            INFO("vector3d * matrix index " << i);
            const F32 actual_component = static_cast<F32>(result.mdV[i]);
            const F32 expected_component = static_cast<F32>(expected.mdV[i]);
            LL_CHECK_APPROX(actual_component, expected_component, kTolerance);
        }
    }

    TEST_CASE("equality and inequality operators respond correctly")
    {
        LLMatrix3 first = make_matrix3({{
            {1.f, 3.f, 5.f},
            {3.f, 6.f, 1.f},
            {4.f, 6.f, 9.f},
        }});
        LLMatrix3 second = make_matrix3({{
            {1.f, 3.f, 5.f},
            {3.f, 6.f, 1.f},
            {4.f, 6.f, 9.f},
        }});
        CHECK(first == second);

        second.setRows(LLVector3(3.f, 6.f, 1.f),
                       LLVector3(3.f, 6.f, 1.f),
                       LLVector3(4.f, 6.f, 9.f));
        CHECK(first != second);
    }

    TEST_CASE("quaternion conversion matches legacy expectations")
    {
        LLMatrix3 mat;
        mat.setRows(LLVector3(2.f, 1.f, 6.f),
                    LLVector3(1.f, 1.f, 3.f),
                    LLVector3(1.f, 7.f, 5.f));

        const LLQuaternion quat = mat.quaternion();
        LL_CHECK_APPROX(quat.mQ[0], -0.66666669f, kTolerance);
        LL_CHECK_APPROX(quat.mQ[1], -0.83333337f, kTolerance);
        LL_CHECK_APPROX(quat.mQ[2], 0.0f, kTolerance);
        LL_CHECK_APPROX(quat.mQ[3], 1.5f, kTolerance);
    }

    TEST_CASE("transpose swaps rows and columns")
    {
        LLMatrix3 mat;
        mat.setRows(LLVector3(1.f, 2.f, 3.f),
                    LLVector3(3.f, 2.f, 1.f),
                    LLVector3(2.f, 2.f, 2.f));
        mat.transpose();

        constexpr std::array<std::array<F32, 3>, 3> expected = {{
            {1.f, 3.f, 2.f},
            {2.f, 2.f, 2.f},
            {3.f, 1.f, 2.f},
        }};
        expect_matrix3_equal(mat, expected, "transpose");
    }

    TEST_CASE("determinant matches reference result")
    {
        LLMatrix3 mat;
        mat.setRows(LLVector3(1.f, 2.f, 3.f),
                    LLVector3(3.f, 2.f, 1.f),
                    LLVector3(2.f, 2.f, 2.f));
        LL_CHECK_APPROX(mat.determinant(), 0.0f, kTolerance);
    }

    TEST_CASE("adjoint transpose builds the expected cofactor matrix")
    {
        LLMatrix3 mat;
        mat.setRows(LLVector3(3.f, 2.f, 1.f),
                    LLVector3(6.f, 2.f, 1.f),
                    LLVector3(3.f, 6.f, 8.f));
        mat.adjointTranspose();

        constexpr std::array<std::array<F32, 3>, 3> expected = {{
            {10.f, -10.f, 0.f},
            {-45.f, 21.f, 3.f},
            {30.f, -12.f, -6.f},
        }};
        expect_matrix3_equal(mat, expected, "adjointTranspose");
    }
}
