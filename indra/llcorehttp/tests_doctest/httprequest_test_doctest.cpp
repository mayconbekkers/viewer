// ---------------------------------------------------------------------------
// Deterministic doctest coverage for HttpRequest semantics using fakes.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"

#include "http_fakes.h"

#include "httphandler.h"
#include "httpheaders.h"

#include <memory>
#include <string>
#include <vector>

using namespace LLCore;
using namespace llcorehttp_test;

namespace
{
class SingleStatusHandler final : public HttpHandler
{
public:
    explicit SingleStatusHandler(HttpStatus expected = HttpStatus(HttpStatus::LLCORE, HE_SUCCESS))
        : mExpected(std::move(expected))
    {
    }

    void onCompleted(HttpHandle handle, HttpResponse* response) override
    {
        ++mCalls;
        mLastHandle = handle;
        REQUIRE(response != nullptr);
        LL_CHECK_EQ_STR(response->getStatus().toHex(), mExpected.toHex());
        if (response->getBody())
        {
            std::string body;
            body.resize(response->getBodySize());
            if (!body.empty())
            {
                response->getBody()->read(0, body.data(), body.size());
            }
            mLastBody = body;
        }
        if (response->getHeaders())
        {
            HttpHeaders::ptr_t headers = response->getHeaders();
            if (headers && headers->size() > 0)
            {
                mLastHeaderName = headers->getContainerTESTONLY()[0].first;
                mLastHeaderValue = headers->getContainerTESTONLY()[0].second;
            }
        }
    }

    int mCalls{0};
    HttpHandle mLastHandle{LLCORE_HTTP_HANDLE_INVALID};
    std::string mLastBody;
    std::string mLastHeaderName;
    std::string mLastHeaderValue;

private:
    HttpStatus mExpected;
};

class MultiStatusHandler final : public HttpHandler
{
public:
    void onCompleted(HttpHandle handle, HttpResponse* response) override
    {
        handles.push_back(handle);
        if (response)
        {
            statuses.push_back(response->getStatus());
            if (response->getBody())
            {
                std::string body;
                body.resize(response->getBodySize());
                if (!body.empty())
                {
                    response->getBody()->read(0, body.data(), body.size());
                }
                bodies.push_back(body);
            }
            else
            {
                bodies.emplace_back();
            }
        }
    }

    std::vector<HttpHandle> handles;
    std::vector<HttpStatus> statuses;
    std::vector<std::string> bodies;
};

class FakeHttpRequest
{
public:
    explicit FakeHttpRequest(FakeTransport& transport)
        : mTransport(transport)
    {
    }

    HttpHandle requestNoOp(const HttpHandler::ptr_t& handler)
    {
        const HttpHandle handle = scheduleNoOp(handler);
        pumpAll();
        return handle;
    }

    HttpHandle requestWithResponse(const FakeResponse& response,
                                   const HttpHandler::ptr_t& handler)
    {
        const HttpHandle handle = scheduleWithResponse(response, handler);
        pumpAll();
        return handle;
    }

    HttpHandle scheduleWithResponse(const FakeResponse& response,
                                    const HttpHandler::ptr_t& handler)
    {
        return mTransport.issueWithResponse(handler, response);
    }

    HttpHandle scheduleNoOp(const HttpHandler::ptr_t& handler)
    {
        return mTransport.issueNoOp(handler);
    }

    void pumpAll()
    {
        while (mTransport.pump()) {}
    }

private:
    FakeTransport& mTransport;
};
} // namespace

TEST_SUITE("httprequest_test")
{
    TEST_CASE("construction and noop request")
    {
        FakeTransport transport;
        FakeHttpRequest request(transport);

        auto handler = std::make_shared<SingleStatusHandler>();
        const HttpHandle handle = request.requestNoOp(handler);
        CHECK(handle != LLCORE_HTTP_HANDLE_INVALID);
        CHECK_EQ(handler->mCalls, 1);
        CHECK_EQ(handler->mLastHandle, handle);
    }

    TEST_CASE("request with canned response delivers headers and body")
    {
        FakeTransport transport;
        FakeHttpRequest request(transport);

        FakeResponse response = FakeResponse::SuccessPayload("payload", "text/plain");
        response.status = HttpStatus(HttpStatus::LLCORE, HE_REPLY_ERROR);

        auto handler = std::make_shared<SingleStatusHandler>(response.status);
        const HttpHandle handle = request.requestWithResponse(response, handler);

        CHECK(handle != LLCORE_HTTP_HANDLE_INVALID);
        CHECK_EQ(handler->mCalls, 1);
        CHECK_EQ(handler->mLastHandle, handle);
        LL_CHECK_EQ_STR(handler->mLastHeaderName, "Content-Type");
        LL_CHECK_EQ_STR(handler->mLastHeaderValue, "text/plain");
        LL_CHECK_EQ_STR(handler->mLastBody, "payload");
    }

    TEST_CASE("idempotent retry updates final status")
    {
        FakeTransport transport;
        FakeHttpRequest request(transport);
        FakeClock clock;

        auto handler = std::make_shared<MultiStatusHandler>();

        request.scheduleWithResponse(FakeResponse::ServerError(), handler);
        request.pumpAll();
        CHECK_EQ(handler->statuses.size(), 1U);
        LL_CHECK_EQ_STR(handler->statuses.back().toHex(), HttpStatus(500, HE_REPLY_ERROR).toHex());

        clock.advance(250);

        request.scheduleWithResponse(FakeResponse::SuccessPayload("{}", "application/json"), handler);
        request.pumpAll();

        CHECK_EQ(handler->statuses.size(), 2U);
        LL_CHECK_EQ_STR(handler->statuses.back().toHex(), HttpStatus(200, HE_SUCCESS).toHex());
        CHECK_GE(clock.now(), static_cast<std::uint64_t>(250));
    }

    TEST_CASE("cancel before processing yields cancelled status")
    {
        FakeTransport transport;
        FakeHttpRequest request(transport);

        auto handler = std::make_shared<MultiStatusHandler>();
        HttpHandle handle = request.scheduleWithResponse(FakeResponse::SuccessPayload("ignored"), handler);
        transport.cancel(handle);
        request.pumpAll();

        CHECK_EQ(handler->statuses.size(), 1U);
        CHECK(handler->statuses.back() == HttpStatus(HttpStatus::LLCORE, HE_OP_CANCELED));
    }
}
