// DOCTEST_SKIP_AUTOGEN: manual suite maintained in doctest
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "linden_common.h"
#include "llsingleton.h"
#include "wrapllerrs.h"
#include "llsd.h"

#include <string>

// Capture execution sequence by appending to log string.
static std::string sLog;

#define DECLARE_CLASS(CLS)                          \
struct CLS: public LLSingleton<CLS>                 \
{                                                   \
    LLSINGLETON(CLS);                               \
    ~CLS();                                         \
public:                                             \
    enum dep_flag                                   \
    {                                               \
        DEP_NONE, /* no dependency */               \
        DEP_CTOR, /* dependency in ctor */          \
        DEP_INIT  /* dependency in initSingleton */ \
    };                                              \
                                                    \
    static dep_flag sDepFlag;                       \
                                                    \
    void initSingleton() override;                  \
    void cleanupSingleton() override;               \
};                                                  \
                                                    \
CLS::dep_flag CLS::sDepFlag = CLS::DEP_NONE

DECLARE_CLASS(A);
DECLARE_CLASS(B);

#define DEFINE_MEMBERS(CLS, OTHER)              \
CLS::CLS()                                      \
{                                               \
    sLog.append(#CLS);                          \
    if (sDepFlag == DEP_CTOR)                   \
    {                                           \
        (void)OTHER::instance();                \
    }                                           \
}                                               \
                                                \
void CLS::initSingleton()                       \
{                                               \
    sLog.append("i" #CLS);                      \
    if (sDepFlag == DEP_INIT)                   \
    {                                           \
        (void)OTHER::instance();                \
    }                                           \
}                                               \
                                                \
void CLS::cleanupSingleton()                    \
{                                               \
    sLog.append("x" #CLS);                      \
}                                               \
                                                \
CLS::~CLS()                                     \
{                                               \
    sLog.append("~" #CLS);                      \
}

DEFINE_MEMBERS(A, B)
DEFINE_MEMBERS(B, A)

class LLSingletonTest final: public LLSingleton<LLSingletonTest>
{
    LLSINGLETON_EMPTY_CTOR(LLSingletonTest);
};

#define PARAMSINGLETON(CLS)                                             \
class CLS: public LLParamSingleton<CLS>                                 \
{                                                                       \
    LLSINGLETON(CLS, const LLSD::String& str): mDesc(str) {}            \
    CLS(LLSD::Integer i): mDesc(i) {}                                   \
                                                                        \
public:                                                                 \
    std::string desc() const { return mDesc.asString(); }               \
                                                                        \
private:                                                                \
    LLSD mDesc;                                                         \
}

PARAMSINGLETON(PSing1);
PARAMSINGLETON(PSing2);

class CircularPCtor final: public LLParamSingleton<CircularPCtor>
{
    LLSINGLETON(CircularPCtor)
    {
        (void)instance();
    }
};

class CircularPInit final: public LLParamSingleton<CircularPInit>
{
    LLSINGLETON_EMPTY_CTOR(CircularPInit);

public:
    void initSingleton() override
    {
        CircularPInit* self = getInstance();
        if (!self)
        {
            throw;
        }
        (void)self;
    }
};

namespace
{
void reset_dependency_flags()
{
    A::sDepFlag = A::DEP_NONE;
    B::sDepFlag = B::DEP_NONE;
}

class SingletonScope
{
public:
    SingletonScope() = default;
    SingletonScope(const SingletonScope&) = delete;
    SingletonScope& operator=(const SingletonScope&) = delete;

    ~SingletonScope()
    {
        LLSingletonBase::deleteAll();
        PSing1::deleteSingleton();
        PSing2::deleteSingleton();
        CircularPCtor::deleteSingleton();
        CircularPInit::deleteSingleton();
        reset_dependency_flags();
        sLog.clear();
    }
};

} // namespace

TEST_SUITE("llsingleton_test")
{
    TEST_CASE("placeholder")
    {
        SingletonScope guard;
    }

    TEST_CASE("LLSingleton basic instance")
    {
        SingletonScope guard;
        auto* instance = LLSingletonTest::getInstance();
        CHECK(instance != nullptr);
    }

    TEST_CASE("LLSingleton delete and recreate")
    {
        SingletonScope guard;
        LLSingletonTest::getInstance();
        CHECK(LLSingletonTest::instanceExists());

        LLSingletonTest::deleteSingleton();
        CHECK_FALSE(LLSingletonTest::instanceExists());

        auto* instance = LLSingletonTest::getInstance();
        CHECK(instance != nullptr);
        CHECK(LLSingletonTest::instanceExists());
    }

    TEST_CASE("A dependency sequence without extra edges")
    {
        SingletonScope guard;
        reset_dependency_flags();
        sLog.clear();

        (void)A::instance();
        CHECK_EQ(sLog, std::string("AiA"));
        LLSingletonBase::deleteAll();
        CHECK_EQ(sLog, std::string("AiAxA~A"));
    }

    TEST_CASE("A constructor depends on B")
    {
        SingletonScope guard;
        reset_dependency_flags();
        A::sDepFlag = A::DEP_CTOR;
        sLog.clear();

        (void)A::instance();
        CHECK_EQ(sLog, "ABiBiA");
        LLSingletonBase::deleteAll();
        CHECK_EQ(sLog, "ABiBiAxA~AxB~B");
    }

    TEST_CASE("A initSingleton depends on B")
    {
        SingletonScope guard;
        reset_dependency_flags();
        A::sDepFlag = A::DEP_INIT;
        sLog.clear();

        (void)A::instance();
        CHECK_EQ(sLog, "AiABiB");
        LLSingletonBase::deleteAll();
        CHECK_EQ(sLog, "AiABiBxA~AxB~B");
    }

    TEST_CASE("A circular dependency")
    {
        SingletonScope guard;
        reset_dependency_flags();
        A::sDepFlag = A::DEP_INIT;
        B::sDepFlag = B::DEP_CTOR;
        sLog.clear();

        (void)A::instance();
        CHECK_EQ(sLog, "AiABiB");
        LLSingletonBase::deleteAll();
        CHECK_EQ(sLog, "AiABiBxA~AxB~B");
    }

    TEST_CASE("B dependency sequence without extra edges")
    {
        SingletonScope guard;
        reset_dependency_flags();
        sLog.clear();

        (void)B::instance();
        CHECK_EQ(sLog, std::string("BiB"));
        LLSingletonBase::deleteAll();
        CHECK_EQ(sLog, std::string("BiBxB~B"));
    }

    TEST_CASE("B constructor depends on A")
    {
        SingletonScope guard;
        reset_dependency_flags();
        B::sDepFlag = B::DEP_CTOR;
        sLog.clear();

        (void)B::instance();
        CHECK_EQ(sLog, "BAiAiB");
        LLSingletonBase::deleteAll();
        CHECK_EQ(sLog, "BAiAiBxB~BxA~A");
    }

    TEST_CASE("B initSingleton depends on A")
    {
        SingletonScope guard;
        reset_dependency_flags();
        B::sDepFlag = B::DEP_INIT;
        sLog.clear();

        (void)B::instance();
        CHECK_EQ(sLog, "BiBAiA");
        LLSingletonBase::deleteAll();
        CHECK_EQ(sLog, "BiBAiAxB~BxA~A");
    }

    TEST_CASE("B circular dependency")
    {
        SingletonScope guard;
        reset_dependency_flags();
        B::sDepFlag = B::DEP_INIT;
        A::sDepFlag = A::DEP_CTOR;
        sLog.clear();

        (void)B::instance();
        CHECK_EQ(sLog, "BiBAiA");
        LLSingletonBase::deleteAll();
        CHECK_EQ(sLog, "BiBAiAxB~BxA~A");
    }

    TEST_CASE("LLParamSingleton basic lifecycle")
    {
        SingletonScope guard;
        WrapLLErrs catcherr;

        CHECK_FALSE(PSing1::instanceExists());
        CHECK_FALSE(PSing1::wasDeleted());

        std::string threw = catcherr.catch_llerrs([]() {
            (void)PSing1::instance();
        });
        CHECK_NE(threw.find("Uninitialized"), std::string::npos);

        threw = catcherr.catch_llerrs([]() {
            (void)PSing1::getInstance();
        });
        CHECK_NE(threw.find("Uninitialized"), std::string::npos);

        PSing1::initParamSingleton("string");
        CHECK_EQ(PSing1::instance().desc(), "string");
        CHECK(PSing1::instanceExists());

        threw = catcherr.catch_llerrs([]() {
            PSing1::initParamSingleton("again");
        });
        CHECK_NE(threw.find("twice"), std::string::npos);

        threw = catcherr.catch_llerrs([]() {
            PSing1::initParamSingleton(17);
        });
        CHECK_NE(threw.find("twice"), std::string::npos);

        PSing1::deleteSingleton();
        CHECK(PSing1::wasDeleted());

        threw = catcherr.catch_llerrs([]() {
            (void)PSing1::instance();
        });
        CHECK_NE(threw.find("deleted"), std::string::npos);
    }

    TEST_CASE("LLParamSingleton alternate constructor")
    {
        SingletonScope guard;
        WrapLLErrs catcherr;

        PSing2::initParamSingleton(17);
        CHECK_EQ(PSing2::instance().desc(), "17");

        std::string threw = catcherr.catch_llerrs([]() {
            PSing2::initParamSingleton(34);
        });
        CHECK_NE(threw.find("twice"), std::string::npos);

        threw = catcherr.catch_llerrs([]() {
            PSing2::initParamSingleton("string");
        });
        CHECK_NE(threw.find("twice"), std::string::npos);
    }

    TEST_CASE("Circular LLParamSingleton constructor")
    {
        SingletonScope guard;
        WrapLLErrs catcherr;

        std::string threw = catcherr.catch_llerrs([]() {
            CircularPCtor::initParamSingleton();
        });
        CHECK_NE(threw.find("constructor"), std::string::npos);
    }

    TEST_CASE("Circular LLParamSingleton initSingleton")
    {
        SingletonScope guard;
        WrapLLErrs catcherr;

        std::string threw = catcherr.catch_llerrs([]() {
            CircularPInit::initParamSingleton();
        });
        CHECK(threw.empty());
    }
}
