// DOCTEST_SKIP_AUTOGEN: manual suite maintained in doctest
// ---------------------------------------------------------------------------
// Doctest suite for the LL error logging helpers.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"

#include "linden_common.h"
#include "../llerror.h"
#include "../llerrorcontrol.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
class TestRecorder final: public LLError::Recorder
{
public:
    TestRecorder() { showTime(false); }

    void recordMessage(LLError::ELevel, const std::string& message) override
    {
        mMessages.push_back(message);
    }

    void clear() { mMessages.clear(); }
    std::size_t size() const { return mMessages.size(); }
    const std::vector<std::string>& messages() const { return mMessages; }

private:
    std::vector<std::string> mMessages;
};

class ErrorFixture
{
public:
    struct FatalException final: std::runtime_error
    {
        explicit FatalException(const std::string& message):
            std::runtime_error(message) {}
    };

    ErrorFixture()
    {
        lastFatalMessage().clear();
        mRecorder = std::make_shared<TestRecorder>();
        mSavedSettings = LLError::saveAndResetSettings();
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        LLError::setFatalFunction(&ErrorFixture::fatalCall);
        LLError::addRecorder(mRecorder);
    }

    ~ErrorFixture()
    {
        LLError::removeRecorder(mRecorder);
        LLError::restoreSettings(mSavedSettings);
    }

    TestRecorder& recorder()
    {
        return *std::static_pointer_cast<TestRecorder>(mRecorder);
    }

    static std::string& lastFatalMessage()
    {
        static std::string message;
        return message;
    }

    static void fatalCall(const std::string& message)
    {
        lastFatalMessage() = message;
        throw FatalException(message);
    }

private:
    LLError::RecorderPtr mRecorder;
    LLError::SettingsStoragePtr mSavedSettings;
};
} // namespace

TEST_SUITE("llerror")
{
    TEST_CASE("info logs captured when level permits")
    {
        ErrorFixture fixture;
        LL_INFOS("llerror_doctest") << "info message payload" << LL_ENDL;
        CHECK_EQ(fixture.recorder().size(), 1u);
        CHECK(fixture.recorder().messages()[0].find("info message payload") != std::string::npos);
    }

    TEST_CASE("default level filters out info when set to errors")
    {
        ErrorFixture fixture;
        LLError::setDefaultLevel(LLError::LEVEL_ERROR);
        LL_INFOS("llerror_doctest") << "should be filtered" << LL_ENDL;
        CHECK_EQ(fixture.recorder().size(), 0u);
    }

    TEST_CASE("error logs trigger fatal function")
    {
        ErrorFixture fixture;
        bool threw = false;
        std::string fatal_output;
        try
        {
            LL_ERRS("llerror_doctest") << "fatal message" << LL_ENDL;
        }
        catch (const ErrorFixture::FatalException& ex)
        {
            threw = true;
            fatal_output = ErrorFixture::lastFatalMessage();
            INFO("fatal output: " << fatal_output);
            CHECK_EQ(std::string(ex.what()), fatal_output);
            CHECK_FALSE(fatal_output.empty());
            CHECK(fatal_output.find("fatal message") != std::string::npos);
        }
        CHECK(threw);
        if (threw)
        {
            const auto normalized = normalize_separators(fatal_output);
            CHECK(normalized.find("fatal message") != std::string::npos);
        }
    }
}
