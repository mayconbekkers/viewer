// DOCTEST_SKIP_AUTOGEN: manual suite maintained in doctest
// ---------------------------------------------------------------------------
// Doctest suite for LLString utilities.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"

#include "linden_common.h"
#include "../llstring.h"

#include <string>
#include <vector>

TEST_SUITE("llstring")
{
    TEST_CASE("trim family handles leading and trailing whitespace")
    {
        std::string head = "  \t  hello world \r\n ";
        LLStringUtil::trimHead(head);
        LL_CHECK_EQ_STR(head, "hello world \r\n ");

        std::string tail = "  trimmed \r\n";
        LLStringUtil::trimTail(tail);
        LL_CHECK_EQ_STR(tail, "  trimmed");

        std::string around = " \t spaced out \n ";
        LLStringUtil::trim(around);
        LL_CHECK_EQ_STR(around, "spaced out");
    }

    TEST_CASE("isValidIndex checks bounds safely")
    {
        const std::string text = "abcd";
        CHECK(LLStringUtil::isValidIndex(text, 0));
        CHECK(LLStringUtil::isValidIndex(text, text.size()));
        CHECK_FALSE(LLStringUtil::isValidIndex(text, text.size() + 1));

        const std::string empty;
        CHECK_FALSE(LLStringUtil::isValidIndex(empty, 0));
    }

    TEST_CASE("getTokens splits sequences with drop delimiters")
    {
        const std::string sentence = "one  two   three";
        const auto tokens = LLStringUtil::getTokens(sentence, " ");
        const std::vector<std::string> expected{ "one", "two", "three" };
        CHECK_EQ(tokens, expected);
    }

    TEST_CASE("getTokens keeps delimiters and honors quoting")
    {
        const std::string arithmetic = "ab+cd / ef*gh";
        const auto arithmetic_tokens = LLStringUtil::getTokens(arithmetic, " ", "+-*/");
        const std::vector<std::string> expected_arithmetic{
            "ab", "+", "cd", "/", "ef", "*", "gh"
        };
        CHECK_EQ(arithmetic_tokens, expected_arithmetic);

        const std::string quoted = "She said, \"Don't go.\"";
        const auto quoted_tokens = LLStringUtil::getTokens(quoted, " ", ",", "\"");
        const std::vector<std::string> expected_quoted{
            "She", "said", ",", "Don't go."
        };
        CHECK_EQ(quoted_tokens, expected_quoted);

        const std::string escaped = "say: 'this isn^'t w^orking'.";
        const auto escaped_tokens = LLStringUtil::getTokens(escaped, " ", "", "'", "^");
        const std::vector<std::string> expected_escaped{
            "say:", "this isn't working."
        };
        CHECK_EQ(escaped_tokens, expected_escaped);
    }

    TEST_CASE("toUpper and toLower convert ASCII strings")
    {
        std::string mixed = "MiXeD";
        LLStringUtil::toUpper(mixed);
        LL_CHECK_EQ_STR(mixed, "MIXED");

        LLStringUtil::toLower(mixed);
        LL_CHECK_EQ_STR(mixed, "mixed");
    }
}
