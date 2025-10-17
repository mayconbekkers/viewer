// ---------------------------------------------------------------------------
// Hand-authored doctest suite covering LLMatrix4 operations.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "linden_common.h"

#include "../m4math.h"
#include "../m3math.h"
#include "../v3math.h"
#include "../v4math.h"

#include <array>
#include <string_view>

namespace
{
constexpr F32 kTolerance = 1.0e-6f;

using Matrix4Array = std::array<std::array<F32, 4>, 4>;
using Matrix3Array = std::array<std::array<F32, 3>, 3>;

Matrix4Array make_identity4()
{
    Matrix4Array result{};
    for (int i = 0; i < 4; ++i)
    {
        result[i][i] = 1.f;
    }
    return result;
}

Matrix4Array multiply_arrays(const Matrix4Array& lhs, const Matrix4Array& rhs)
{
    Matrix4Array result{};
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            F32 value = 0.f;
            for (int k = 0; k < 4; ++k)
            {
                value += lhs[r][k] * rhs[k][c];
            }
            result[r][c] = value;
        }
    }
    return result;
}

LLMatrix4 make_matrix4(const Matrix4Array& values)
{
    LLMatrix4 result;
    result.setZero();
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            result.mMatrix[r][c] = values[r][c];
        }
    }
    return result;
}

void expect_matrix4_equal(const LLMatrix4& actual, const Matrix4Array& expected, std::string_view label)
{
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            INFO(label << " [" << r << "][" << c << "]");
            LL_CHECK_APPROX(actual.mMatrix[r][c], expected[r][c], kTolerance);
        }
    }
}

void expect_matrix3_equal(const LLMatrix3& actual, const Matrix3Array& expected, std::string_view label)
{
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 3; ++c)
        {
            INFO(label << " [" << r << "][" << c << "]");
            LL_CHECK_APPROX(actual.mMatrix[r][c], expected[r][c], kTolerance);
        }
    }
}

void expect_vector3_equal(const LLVector3& actual, const LLVector3& expected, std::string_view label)
{
    for (int i = 0; i < 3; ++i)
    {
        INFO(label << " index " << i);
        LL_CHECK_APPROX(actual.mV[i], expected.mV[i], kTolerance);
    }
}

void expect_vector4_equal(const LLVector4& actual, const LLVector4& expected, std::string_view label)
{
    for (int i = 0; i < 4; ++i)
    {
        INFO(label << " index " << i);
        LL_CHECK_APPROX(actual.mV[i], expected.mV[i], kTolerance);
    }
}
} // namespace

TEST_SUITE("llmatrix4_test")
{
    TEST_CASE("setIdentity produces the canonical 4x4 identity")
    {
        LLMatrix4 mat;
        mat.setZero();
        mat.setIdentity();

        expect_matrix4_equal(mat, make_identity4(), "setIdentity");
    }

    TEST_CASE("setTranslation updates translation row")
    {
        LLMatrix4 mat;
        mat.setIdentity();
        const LLVector3 translation(2.f, -3.f, 4.f);
        mat.setTranslation(translation);

        expect_matrix4_equal(
            mat,
            Matrix4Array{{
                {1.f, 0.f, 0.f, 0.f},
                {0.f, 1.f, 0.f, 0.f},
                {0.f, 0.f, 1.f, 0.f},
                {2.f, -3.f, 4.f, 1.f},
            }},
            "setTranslation");

        expect_vector3_equal(mat.getTranslation(), translation, "getTranslation");
    }

    TEST_CASE("matrix multiplication matches manual computation")
    {
        const Matrix4Array lhs = {{
            {1.f, 2.f, 3.f, 4.f},
            {5.f, 6.f, 7.f, 8.f},
            {2.f, 0.f, 1.f, 0.f},
            {0.f, 0.f, 0.f, 1.f},
        }};
        const Matrix4Array rhs = {{
            {2.f, 0.f, 1.f, 0.f},
            {1.f, 1.f, 0.f, 0.f},
            {0.f, 1.f, 1.f, 0.f},
            {0.f, 0.f, 0.f, 1.f},
        }};
        LLMatrix4 mat_lhs = make_matrix4(lhs);
        const LLMatrix4 mat_rhs = make_matrix4(rhs);

        mat_lhs *= mat_rhs;
        const Matrix4Array expected = multiply_arrays(lhs, rhs);
        expect_matrix4_equal(mat_lhs, expected, "matrix multiply");
    }

    TEST_CASE("vector multiplication applies rotation and translation")
    {
        const Matrix4Array transform = {{
            {0.f, -1.f, 0.f, 0.f},  // 90 degree rotation around Z
            {1.f, 0.f, 0.f, 0.f},
            {0.f, 0.f, 1.f, 0.f},
            {3.f, 4.f, 5.f, 1.f},  // translation
        }};
        const LLMatrix4 mat = make_matrix4(transform);

        const LLVector3 point(2.f, 1.f, 0.f);
        const LLVector3 expected_point(1.f + 3.f,  -2.f + 4.f, 0.f + 5.f);
        expect_vector3_equal(point * mat, expected_point, "vector3 * matrix");

        const LLVector4 homogeneous(2.f, 1.f, 0.f, 1.f);
        const LLVector4 expected_homogeneous(expected_point.mV[VX],
                                             expected_point.mV[VY],
                                             expected_point.mV[VZ],
                                             1.f);
        expect_vector4_equal(homogeneous * mat, expected_homogeneous, "vector4 * matrix");
    }

    TEST_CASE("extracting the upper-left 3x3 block matches source")
    {
        const Matrix4Array values = {{
            {0.5f, 0.0f, 0.0f, 0.f},
            {0.0f, 0.5f, 0.0f, 0.f},
            {0.0f, 0.0f, 2.0f, 0.f},
            {1.0f, 2.0f, 3.0f, 1.f},
        }};
        const LLMatrix4 mat = make_matrix4(values);
        const LLMatrix3 extracted = mat.getMat3();

        constexpr std::array<std::array<F32, 3>, 3> expected = {{
            {0.5f, 0.0f, 0.0f},
            {0.0f, 0.5f, 0.0f},
            {0.0f, 0.0f, 2.0f},
        }};
        expect_matrix3_equal(extracted, expected, "getMat3");
    }
}
