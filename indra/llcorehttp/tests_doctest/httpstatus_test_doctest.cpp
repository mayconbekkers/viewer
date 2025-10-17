// ---------------------------------------------------------------------------
// Doctest port of legacy HttpStatus tests (header-based TUT suite).
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"

#include "httpcommon.h"

#include <curl/curl.h>
#include <curl/multi.h>

using namespace LLCore;

namespace
{
constexpr F32 kTol = 1.0e-6f;
} // namespace

TEST_SUITE("httpstatus_test")
{
    TEST_CASE("construction and bool conversion")
    {
        HttpStatus status(HttpStatus::EXT_CURL_EASY, 0);
        CHECK(status);
        CHECK_FALSE(!status);

        status = HttpStatus(HttpStatus::EXT_CURL_MULTI, 0);
        CHECK(status);
        CHECK_FALSE(!status);

        status = HttpStatus(HttpStatus::LLCORE, HE_SUCCESS);
        CHECK(status);
        CHECK_FALSE(!status);

        status = HttpStatus(HttpStatus::EXT_CURL_MULTI, -1);
        CHECK_FALSE(status);
        CHECK(!status);

        status = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_BAD_DOWNLOAD_RESUME);
        CHECK_FALSE(status);
        CHECK(!status);
    }

    TEST_CASE("valid status string conversion")
    {
        HttpStatus status(HttpStatus::EXT_CURL_EASY, 0);
        std::string msg = status.toString();
        LL_CHECK_MSG(msg.empty(), "Success should return empty string");

        status = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_BAD_FUNCTION_ARGUMENT);
        msg = status.toString();
        LL_CHECK_MSG(!msg.empty(), "curl easy error should have message");

        status = HttpStatus(HttpStatus::EXT_CURL_MULTI, CURLM_OUT_OF_MEMORY);
        msg = status.toString();
        LL_CHECK_MSG(!msg.empty(), "curl multi error should have message");

        status = HttpStatus(HttpStatus::LLCORE, HE_SHUTTING_DOWN);
        msg = status.toString();
        LL_CHECK_MSG(!msg.empty(), "llcore error should have message");
    }

    TEST_CASE("invalid status string conversion")
    {
        HttpStatus status(HttpStatus::EXT_CURL_EASY, 32726);
        std::string msg = status.toString();
        LL_CHECK_MSG(!msg.empty(), "unknown curl easy code still stringifies");

        status = HttpStatus(HttpStatus::EXT_CURL_MULTI, -470);
        msg = status.toString();
        LL_CHECK_MSG(!msg.empty(), "unknown curl multi code still stringifies");

        status = HttpStatus(HttpStatus::LLCORE, 923);
        msg = status.toString();
        LL_CHECK_MSG(!msg.empty(), "unknown llcore code still stringifies");
    }

    TEST_CASE("equality and inequality semantics")
    {
        const HttpStatus success_core(HttpStatus::LLCORE, HE_SUCCESS);
        const HttpStatus success_easy(HttpStatus::EXT_CURL_EASY, HE_SUCCESS);
        CHECK(success_core != success_easy);

        const HttpStatus error_core(HttpStatus::LLCORE, HE_REPLY_ERROR);
        const HttpStatus shutting_down(HttpStatus::LLCORE, HE_SHUTTING_DOWN);
        CHECK(error_core != success_easy);
        CHECK(shutting_down != success_easy);
    }

    TEST_CASE("http status encoding and overrides")
    {
        HttpStatus status(200, HE_SUCCESS);
        std::string msg = status.toString();
        LL_CHECK_MSG(msg.empty(), "successful 200 should be empty");
        CHECK(status);

        status = HttpStatus(200, HE_REPLY_ERROR);
        msg = status.toString();
        LL_CHECK_MSG(!msg.empty(), "application error should produce message");
        CHECK_FALSE(status);
        CHECK(status.toULong() > 1UL);

        const HttpStatus success_http(200, HE_SUCCESS);
        const HttpStatus error_http(200, HE_REPLY_ERROR);
        CHECK(success_http != error_http);

        status = HttpStatus(406, HE_SUCCESS);
        msg = status.toString();
        LL_CHECK_MSG(msg.empty(), "application success trumps HTTP error code");
        CHECK(status);

        const HttpStatus http_200_ok(200, HE_SUCCESS);
        const HttpStatus http_201_ok(201, HE_SUCCESS);
        CHECK(http_200_ok != http_201_ok);

        const HttpStatus http_200_err(200, HE_REPLY_ERROR);
        const HttpStatus http_201_err(201, HE_REPLY_ERROR);
        CHECK(http_200_err != http_201_err);
    }

    TEST_CASE("http status human readable strings")
    {
        HttpStatus status(100, HE_REPLY_ERROR);
        std::string msg = status.toString();
        LL_CHECK_EQ_STR(msg, "Continue");

        status = HttpStatus(200, HE_SUCCESS);
        msg = status.toString();
        LL_CHECK_MSG(msg.empty(), "success remains empty");

        status = HttpStatus(199, HE_REPLY_ERROR);
        LL_CHECK_EQ_STR(status.toString(), "Unknown error");

        status = HttpStatus(505, HE_REPLY_ERROR);
        LL_CHECK_EQ_STR(status.toString(), "HTTP Version not supported");

        status = HttpStatus(506, HE_REPLY_ERROR);
        LL_CHECK_EQ_STR(status.toString(), "Unknown error");

        status = HttpStatus(999, HE_REPLY_ERROR);
        LL_CHECK_EQ_STR(status.toString(), "Unknown error");
    }

    TEST_CASE("hex representation")
    {
        HttpStatus status(404);
        LL_CHECK_EQ_STR(status.toHex(), "01940001");
    }

    TEST_CASE("terse string rendering")
    {
        HttpStatus status(404);
        LL_CHECK_EQ_STR(status.toTerseString(), "Http_404");

        status = HttpStatus(200);
        LL_CHECK_EQ_STR(status.toTerseString(), "Http_200");

        status = HttpStatus(200, HE_REPLY_ERROR);
        LL_CHECK_EQ_STR(status.toTerseString(), "Http_200");

        status = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_CONNECT);
        LL_CHECK_EQ_STR(status.toTerseString(), "Easy_7");

        status = HttpStatus(HttpStatus::EXT_CURL_MULTI, CURLM_OUT_OF_MEMORY);
        LL_CHECK_EQ_STR(status.toTerseString(), "Multi_3");

        status = HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_SET);
        LL_CHECK_EQ_STR(status.toTerseString(), "Core_7");

        status = HttpStatus(22000, 1);
        LL_CHECK_EQ_STR(status.toTerseString(), "Unknown_1");

        status = HttpStatus(22000, -1);
        LL_CHECK_EQ_STR(status.toTerseString(), "Unknown_65535");
    }
}
