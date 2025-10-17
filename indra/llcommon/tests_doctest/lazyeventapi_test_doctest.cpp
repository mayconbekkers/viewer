// DOCTEST_SKIP_AUTOGEN: manual suite maintained in doctest
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "linden_common.h"
#include "lazyeventapi.h"
#include "llevents.h"
#include "llsdutil.h"

#include <vector>

namespace
{
LLSD gData;

class PumpScope
{
public:
    PumpScope() { gData.clear(); }
    PumpScope(const PumpScope&) = delete;
    PumpScope& operator=(const PumpScope&) = delete;
    ~PumpScope()
    {
        LLEventPumps::deleteSingleton();
        gData.clear();
    }
};

class MyListener final: public LLEventAPI
{
public:
    explicit MyListener(const LL::LazyEventAPIParams& params):
        LLEventAPI(params) {}

    void set_data(const LLSD& event)
    {
        gData = event["data"];
    }
};

class MyRegistrar final: public LL::LazyEventAPI<MyListener>
{
    using super = LL::LazyEventAPI<MyListener>;
    using super::listener;

public:
    MyRegistrar():
        super("Test", "This is a test LLEventAPI")
    {
        add("set", "This is a set operation", &listener::set_data);
    }
};
} // namespace

TEST_SUITE("lazyeventapi")
{
    TEST_CASE("LazyEventAPI dispatches set operation")
    {
        PumpScope scope;
        MyRegistrar registrar;
        LLEventPumps::instance().obtain("Test").post(llsd::map("op", "set", "data", "hey"));
        CHECK_EQ(gData.asString(), "hey");
    }

    TEST_CASE("No LazyEventAPI instance ignores posts")
    {
        PumpScope scope;
        LLEventPumps::instance().obtain("Test").post(llsd::map("op", "set", "data", "moot"));
        CHECK_FALSE(gData.isDefined());
    }

    TEST_CASE("LazyEventAPI exposes metadata")
    {
        PumpScope scope;
        MyRegistrar registrar;

        const MyRegistrar* found = nullptr;
        for (const auto& tracker : LL::LazyEventAPIBase::instance_snapshot())
        {
            found = dynamic_cast<const MyRegistrar*>(&tracker);
            if (found)
            {
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK_EQ(found->getName(), "Test");
        CHECK(found->getDesc().find("test LLEventAPI") != std::string::npos);
        CHECK_EQ(found->getDispatchKey(), "op");

        std::vector<LL::LazyEventAPIBase::NameDesc> ops{ found->begin(), found->end() };
        REQUIRE_EQ(ops.size(), 1u);
        CHECK_EQ(ops[0].first, "set");
        CHECK(ops[0].second.find("set operation") != std::string::npos);

        LLSD metadata = found->getMetadata(ops[0].first);
        CHECK_EQ(metadata["name"].asString(), ops[0].first);
        CHECK_EQ(metadata["desc"].asString(), ops[0].second);
    }
}
#// DOCTEST_SKIP_AUTOGEN: manual suite maintained in doctest
#include "doctest.h"
