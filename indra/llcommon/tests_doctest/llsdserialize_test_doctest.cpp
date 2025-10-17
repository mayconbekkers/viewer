// Deterministic doctest subset for LLSD serialization/parse coverage
// DOCTEST_SKIP_AUTOGEN: manual subset maintained by hand
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"

#include "linden_common.h"
#include "llformat.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llmemorystream.h"
#include "llpointer.h"
#include "lluri.h"
#include "lluuid.h"

#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
using FormatterFunction = std::function<void(const LLSD&, std::ostream&)>;
using ParserFunction = std::function<S32(std::istream&, LLSD&, llssize)>;

std::vector<U8> string_to_vector(const std::string& source)
{
    return std::vector<U8>(source.begin(), source.end());
}

void fill_map(LLSD& root, U32 width, U32 depth)
{
    if (depth == 0)
    {
        root["leaf"] = "value";
        return;
    }

    for (U32 index = 0; index < width; ++index)
    {
        std::string key = llformat("child %u", index);
        root[key] = LLSD::emptyMap();
        fill_map(root[key], width, depth - 1);
    }
}

class RoundTripFixture
{
public:
    void setFormatterParser(LLPointer<LLSDFormatter> formatter, LLPointer<LLSDParser> parser)
    {
        mFormatter = [formatter](const LLSD& data, std::ostream& out) mutable
        {
            formatter->format(data, out);
        };
        mParser = [parser](std::istream& in, LLSD& data, llssize max_bytes) mutable -> S32
        {
            parser->reset();
            return parser->parse(in, data, max_bytes);
        };
    }

    void setFormatter(std::function<void(const LLSD&, std::ostream&)> formatter)
    {
        mFormatter = std::move(formatter);
    }

    void setParser(std::function<bool(LLSD&, std::istream&, llssize)> parser)
    {
        mParser = [parser](std::istream& in, LLSD& data, llssize max_bytes) mutable -> S32
        {
            const bool ok = parser(data, in, max_bytes);
            return ok ? 1 : LLSDParser::PARSE_FAILURE;
        };
    }

    void checkRoundTrip(const std::string& label, const LLSD& value)
    {
        REQUIRE_MESSAGE(static_cast<bool>(mFormatter), label << ": formatter not configured");
        REQUIRE_MESSAGE(static_cast<bool>(mParser), label << ": parser not configured");

        std::stringstream serialized;
        mFormatter(value, serialized);
        const std::string payload = serialized.str();

        INFO(label);
        INFO("serialized size: " << payload.size());
        INFO("payload: " << payload);

        LLSD decoded;
        std::stringstream input(payload);
        const S32 parsed_count = mParser(input, decoded, static_cast<llssize>(payload.size()));
        CHECK_MESSAGE(parsed_count != LLSDParser::PARSE_FAILURE, label << ": parse failure");
        CHECK_MESSAGE(decoded == value, label << ": round-trip mismatch");
    }

    void doRoundTripTests(const std::string& label)
    {
        LLSD value;
        checkRoundTrip(label + " undefined", value);

        value = true;
        checkRoundTrip(label + " true bool", value);

        value = false;
        checkRoundTrip(label + " false bool", value);

        value = 1;
        checkRoundTrip(label + " positive int", value);

        value = 0;
        checkRoundTrip(label + " zero int", value);

        value = -1;
        checkRoundTrip(label + " negative int", value);

        value = 1234.5f;
        checkRoundTrip(label + " positive real", value);

        value = -1234.5f;
        checkRoundTrip(label + " negative real", value);

        value = LLSD::emptyArray();
        checkRoundTrip(label + " empty array", value);

        value = LLSD::emptyArray();
        value.append("ali");
        value.append(28);
        checkRoundTrip(label + " array", value);

        value.clear();
        value[0][0] = true;
        value[1][0] = false;
        checkRoundTrip(label + " nested arrays", value);

        value = LLSD::emptyMap();
        value["foo"] = "bar";
        value["baz"] = 100;
        checkRoundTrip(label + " map", value);

        value = LLUUID("c96f9b1e-f589-4100-9774-d98643ce0bed");
        checkRoundTrip(label + " uuid", value);

        value = LLURI("https://secondlife.com/login");
        checkRoundTrip(label + " uri", value);

        value = LLDate("2006-04-24T16:11:33Z");
        checkRoundTrip(label + " date", value);

        value = string_to_vector("hello");
        checkRoundTrip(label + " binary simple", value);

        value = LLSD::emptyMap();
        fill_map(value, 3, 2);
        checkRoundTrip(label + " nested maps", value);
    }

private:
    FormatterFunction mFormatter;
    ParserFunction mParser;
};

template <typename ParserT>
class ParseFixture
{
public:
    ParseFixture()
        : mParser(new ParserT)
    {
    }

    void ensureParse(const std::string& msg,
                     const std::string& input_text,
                     const LLSD& expected_value,
                     S32 expected_count,
                     S32 depth_limit = -1)
    {
        std::stringstream input(input_text);
        LLSD parsed;
        mParser->reset();
        const S32 count = mParser->parse(
            input,
            parsed,
            static_cast<llssize>(input_text.size()),
            depth_limit);

        INFO(msg);
        INFO("input: " << input_text);
        if (expected_count == LLSDParser::PARSE_FAILURE)
        {
            CHECK(count == LLSDParser::PARSE_FAILURE);
            CHECK(parsed == expected_value);
        }
        else
        {
            CHECK(parsed == expected_value);
            CHECK(count == expected_count);
        }
    }

private:
    LLPointer<ParserT> mParser;
};

class CompatibilityFixture
{
public:
    void ensureBinaryAndNotation(const std::string& label, const LLSD& input)
    {
        INFO(label);

        std::stringstream binary_stream;
        const S32 binary_count = LLSDSerialize::toBinary(input, binary_stream);
        binary_stream.seekg(0);

        LLSD from_binary;
        const S32 parsed_binary = LLSDSerialize::fromBinary(
            from_binary,
            binary_stream,
            LLSDSerialize::SIZE_UNLIMITED);
        CHECK(parsed_binary == binary_count);
        CHECK(from_binary == input);

        std::stringstream notation_stream;
        const S32 notation_count = LLSDSerialize::toNotation(from_binary, notation_stream);
        notation_stream.seekg(0);

        LLSD from_notation;
        const S32 parsed_notation = LLSDSerialize::fromNotation(
            from_notation,
            notation_stream,
            LLSDSerialize::SIZE_UNLIMITED);
        CHECK(parsed_notation == notation_count);
        CHECK(from_notation == input);
    }

    void ensureBinaryAndXML(const std::string& label, const LLSD& input)
    {
        INFO(label);

        std::stringstream binary_stream;
        const S32 binary_count = LLSDSerialize::toBinary(input, binary_stream);
        binary_stream.seekg(0);

        LLSD from_binary;
        const S32 parsed_binary = LLSDSerialize::fromBinary(
            from_binary,
            binary_stream,
            LLSDSerialize::SIZE_UNLIMITED);
        CHECK(parsed_binary == binary_count);

        std::stringstream xml_stream;
        const S32 xml_count = LLSDSerialize::toXML(from_binary, xml_stream);
        xml_stream.seekg(0);

        LLSD from_xml;
        const S32 parsed_xml = LLSDSerialize::fromXML(from_xml, xml_stream);
        CHECK(parsed_xml == xml_count);
        CHECK(from_xml == input);
    }
};
} // namespace

TUT_SUITE("llsdserialize_test")
{
    TUT_CASE("llsdserialize_test::sd_xml_object_test_1")
    {
        LLSD sd;
        LLPointer<LLSDXMLFormatter> formatter = new LLSDXMLFormatter;

        auto xml_test = [&](const char* name, const std::string& expected)
        {
            std::ostringstream out;
            formatter->format(sd, out);
            INFO(name);
            LL_CHECK_EQ_STR(expected, out.str());
        };

        xml_test("undef", "<llsd><undef /></llsd>\n");

        sd = 3463;
        xml_test("integer", "<llsd><integer>3463</integer></llsd>\n");

        sd = "";
        xml_test("empty string", "<llsd><string /></llsd>\n");

        sd = "foobar";
        xml_test("string", "<llsd><string>foobar</string></llsd>\n");

        sd = LLUUID::null;
        xml_test("null uuid", "<llsd><uuid /></llsd>\n");

        sd = LLUUID("c96f9b1e-f589-4100-9774-d98643ce0bed");
        xml_test("uuid", "<llsd><uuid>c96f9b1e-f589-4100-9774-d98643ce0bed</uuid></llsd>\n");

        sd = LLURI("https://secondlife.com/login");
        xml_test("uri", "<llsd><uri>https://secondlife.com/login</uri></llsd>\n");

        sd = LLDate("2006-04-24T16:11:33Z");
        xml_test("date", "<llsd><date>2006-04-24T16:11:33Z</date></llsd>\n");

        sd = string_to_vector("hello");
        xml_test("binary", "<llsd><binary encoding=\"base64\">aGVsbG8=</binary></llsd>\n");
    }

    TUT_CASE("llsdserialize_test::sd_xml_object_test_2")
    {
        LLSD sd;
        LLPointer<LLSDXMLFormatter> formatter = new LLSDXMLFormatter;

        formatter->boolalpha(true);
        sd = true;
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><boolean>true</boolean></llsd>\n", out.str());
        }
        sd = false;
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><boolean>false</boolean></llsd>\n", out.str());
        }

        formatter->boolalpha(false);
        sd = true;
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><boolean>1</boolean></llsd>\n", out.str());
        }
        sd = false;
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><boolean>0</boolean></llsd>\n", out.str());
        }
    }

    TUT_CASE("llsdserialize_test::sd_xml_object_test_3")
    {
        LLSD sd;
        LLPointer<LLSDXMLFormatter> formatter = new LLSDXMLFormatter;

        formatter->realFormat("%.2f");
        sd = 1.0;
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><real>1.00</real></llsd>\n", out.str());
        }

        sd = -34379.0438;
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><real>-34379.04</real></llsd>\n", out.str());
        }

        formatter->realFormat("%.4f");
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><real>-34379.0438</real></llsd>\n", out.str());
        }

        formatter->realFormat("%.0f");
        sd = 0.0;
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><real>0</real></llsd>\n", out.str());
        }

        sd = 3287.4387;
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><real>3287</real></llsd>\n", out.str());
        }
    }

    TUT_CASE("llsdserialize_test::sd_xml_object_test_4")
    {
        LLSD sd;
        LLPointer<LLSDXMLFormatter> formatter = new LLSDXMLFormatter;

        sd = LLSD::emptyArray();
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><array /></llsd>\n", out.str());
        }

        sd = LLSD::emptyArray();
        sd.append(LLSD());
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><array><undef /></array></llsd>\n", out.str());
        }

        sd = LLSD::emptyArray();
        sd.append(LLSD());
        sd.append(1);
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><array><undef /><integer>1</integer></array></llsd>\n", out.str());
        }
    }

    TUT_CASE("llsdserialize_test::sd_xml_object_test_5")
    {
        LLSD sd = LLSD::emptyMap();
        LLPointer<LLSDXMLFormatter> formatter = new LLSDXMLFormatter;

        sd["foo"] = "bar";
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><map><key>foo</key><string>bar</string></map></llsd>\n", out.str());
        }

        sd["baz"] = LLSD();
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><map><key>baz</key><undef /><key>foo</key><string>bar</string></map></llsd>\n", out.str());
        }
    }

    TUT_CASE("llsdserialize_test::sd_xml_object_test_6")
    {
        LLSD sd;
        LLPointer<LLSDXMLFormatter> formatter = new LLSDXMLFormatter;

        sd = string_to_vector("hello");
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR("<llsd><binary encoding=\"base64\">aGVsbG8=</binary></llsd>\n", out.str());
        }

        sd = string_to_vector("6|6|asdfhappybox|60e44ec5-305c-43c2-9a19-b4b89b1ae2a6|60e44ec5-305c-43c2-9a19-b4b89b1ae2a6|60e44ec5-305c-43c2-9a19-b4b89b1ae2a6|00000000-0000-0000-0000-000000000000|7fffffff|7fffffff|0|0|82000|450fe394-2904-c9ad-214c-a07eb7feec29|(No Description)|0|10|0");
        const std::string expected =
            "<llsd><binary encoding=\"base64\">Nnw2fGFzZGZoYXBweWJveHw2MGU0NGVjNS0zMDVjLTQzYzItOWExOS1iNGI4OWIxYWUyYTZ8NjBlNDRlYzUtMzA1Yy00M2MyLTlhMTktYjRiODliMWFlMmE2fDYwZTQ0ZWM1LTMwNWMtNDNjMi05YTE5LWI0Yjg5YjFhZTJhNnwwMDAwMDAwMC0wMDAwLTAwMDAtMDAwMC0wMDAwMDAwMDAwMDB8N2ZmZmZmZmZ8N2ZmZmZmZmZ8MHwwfDgyMDAwfDQ1MGZlMzk0LTI5MDQtYzlhZC0yMTRjLWEwN2ViN2ZlZWMyOXwoTm8gRGVzY3JpcHRpb24pfDB8MTB8MA==</binary></llsd>\n";
        {
            std::ostringstream out;
            formatter->format(sd, out);
            LL_CHECK_EQ_STR(expected, out.str());
        }
    }

    TUT_CASE("llsdserialize_test::TestLLSDSerializeObject_test_1")
    {
        RoundTripFixture fixture;
        fixture.setFormatterParser(
            new LLSDNotationFormatter(false, "", LLSDFormatter::OPTIONS_PRETTY_BINARY),
            new LLSDNotationParser());
        fixture.doRoundTripTests("pretty binary notation serialization");
    }

    TUT_CASE("llsdserialize_test::TestLLSDSerializeObject_test_2")
    {
        RoundTripFixture fixture;
        fixture.setFormatterParser(
            new LLSDNotationFormatter(false, "", LLSDFormatter::OPTIONS_NONE),
            new LLSDNotationParser());
        fixture.doRoundTripTests("raw binary notation serialization");
    }

    TUT_CASE("llsdserialize_test::TestLLSDSerializeObject_test_3")
    {
        RoundTripFixture fixture;
        fixture.setFormatterParser(new LLSDXMLFormatter(), new LLSDXMLParser());
        fixture.doRoundTripTests("xml serialization");
    }

    TUT_CASE("llsdserialize_test::TestLLSDSerializeObject_test_4")
    {
        RoundTripFixture fixture;
        fixture.setFormatterParser(new LLSDBinaryFormatter(), new LLSDBinaryParser());
        fixture.doRoundTripTests("binary serialization");
    }

    TUT_CASE("llsdserialize_test::TestLLSDSerializeObject_test_5")
    {
        RoundTripFixture fixture;
        fixture.setFormatter(
            [](const LLSD& sd, std::ostream& out)
            {
                LLSDSerialize::serialize(sd, out, LLSDSerialize::LLSD_BINARY);
            });
        fixture.setParser(LLSDSerialize::deserialize);
        fixture.doRoundTripTests("serialize(LLSD_BINARY)");
    }

    TUT_CASE("llsdserialize_test::TestLLSDSerializeObject_test_6")
    {
        RoundTripFixture fixture;
        fixture.setFormatter(
            [](const LLSD& sd, std::ostream& out)
            {
                LLSDSerialize::serialize(sd, out, LLSDSerialize::LLSD_XML);
            });
        fixture.setParser(LLSDSerialize::deserialize);
        fixture.doRoundTripTests("serialize(LLSD_XML)");
    }

    TUT_CASE("llsdserialize_test::TestLLSDSerializeObject_test_7")
    {
        RoundTripFixture fixture;
        fixture.setFormatter(
            [](const LLSD& sd, std::ostream& out)
            {
                LLSDSerialize::serialize(sd, out, LLSDSerialize::LLSD_NOTATION);
            });
        fixture.setParser(LLSDSerialize::deserialize);
        fixture.doRoundTripTests("serialize(LLSD_NOTATION)");
    }

    TUT_CASE("llsdserialize_test::TestLLSDSerializeObject_test_8")
    {
        RoundTripFixture fixture;
        fixture.setFormatterParser(
            new LLSDNotationFormatter(false, "", LLSDFormatter::OPTIONS_NONE),
            new LLSDNotationParser());
        fixture.setParser(LLSDSerialize::deserialize);
        fixture.doRoundTripTests("LLSDNotationFormatter -> deserialize");
    }

    TUT_CASE("llsdserialize_test::TestLLSDSerializeObject_test_9")
    {
        RoundTripFixture fixture;
        fixture.setFormatterParser(
            new LLSDXMLFormatter(false, "", LLSDFormatter::OPTIONS_NONE),
            new LLSDXMLParser());
        fixture.setParser(LLSDSerialize::deserialize);
        fixture.doRoundTripTests("LLSDXMLFormatter -> deserialize");
    }

    TUT_CASE("llsdserialize_test::TestLLSDXMLParsingObject_test_1")
    {
        ParseFixture<LLSDXMLParser> fixture;
        fixture.ensureParse(
            "malformed xml",
            "<llsd><string>ha ha</string>",
            LLSD(),
            LLSDParser::PARSE_FAILURE);
        fixture.ensureParse(
            "not llsd",
            "<html><body><p>ha ha</p></body></html>",
            LLSD(),
            LLSDParser::PARSE_FAILURE);
        fixture.ensureParse(
            "value without llsd",
            "<string>ha ha</string>",
            LLSD(),
            LLSDParser::PARSE_FAILURE);
        fixture.ensureParse(
            "key without llsd",
            "<key>ha ha</key>",
            LLSD(),
            LLSDParser::PARSE_FAILURE);
    }

    TUT_CASE("llsdserialize_test::TestLLSDXMLParsingObject_test_2")
    {
        ParseFixture<LLSDXMLParser> fixture;
        LLSD expected;
        expected["amy"] = 23;
        expected["bob"] = LLSD();
        expected["cam"] = 1.23;

        fixture.ensureParse(
            "unknown data type",
            "<llsd><map>"
            "<key>amy</key><integer>23</integer>"
            "<key>bob</key><bigint>99999999999999999</bigint>"
            "<key>cam</key><real>1.23</real>"
            "</map></llsd>",
            expected,
            static_cast<S32>(expected.size()) + 1);
    }

    TUT_CASE("llsdserialize_test::TestLLSDXMLParsingObject_test_3")
    {
        ParseFixture<LLSDXMLParser> fixture;

        LLSD expected;
        expected["amy"] = 23;
        expected["cam"] = 1.23;
        fixture.ensureParse(
            "map with html",
            "<llsd><map>"
            "<key>amy</key><integer>23</integer>"
            "<html><body>ha ha</body></html>"
            "<key>cam</key><real>1.23</real>"
            "</map></llsd>",
            expected,
            static_cast<S32>(expected.size()) + 1);

        expected.clear();
        expected["amy"] = 23;
        expected["cam"] = 1.23;
        fixture.ensureParse(
            "map with value for key",
            "<llsd><map>"
            "<key>amy</key><integer>23</integer>"
            "<string>ha ha</string>"
            "<key>cam</key><real>1.23</real>"
            "</map></llsd>",
            expected,
            static_cast<S32>(expected.size()) + 1);

        expected.clear();
        expected["amy"] = 23;
        expected["bob"] = LLSD::emptyMap();
        expected["cam"] = 1.23;
        fixture.ensureParse(
            "map with map of html",
            "<llsd><map>"
            "<key>amy</key><integer>23</integer>"
            "<key>bob</key>"
            "<map>"
            "<html><body>ha ha</body></html>"
            "</map>"
            "<key>cam</key><real>1.23</real>"
            "</map></llsd>",
            expected,
            static_cast<S32>(expected.size()) + 1);
    }

    TUT_CASE("llsdserialize_test::TestLLSDXMLParsingObject_test_4")
    {
        ParseFixture<LLSDXMLParser> fixture;

        std::string xml = "<llsd><binary encoding=\"base64\">aGVsbG8=</binary></llsd>\n";
        fixture.ensureParse(
            "binary hello",
            xml,
            string_to_vector("hello"),
            1);

        const std::string blob =
            "<llsd><binary encoding=\"base64\">Nnw2fGFzZGZoYXBweWJveHw2MGU0NGVjNS0zMDVjLTQzYzItOWExOS1iNGI4OWIxYWUyYTZ8NjBlNDRlYzUtMzA1Yy00M2MyLTlhMTktYjRiODliMWFlMmE2fDYwZTQ0ZWM1LTMwNWMtNDNjMi05YTE5LWI0Yjg5YjFhZTJhNnwwMDAwMDAwMC0wMDAwLTAwMDAtMDAwMC0wMDAwMDAwMDAwMDB8N2ZmZmZmZmZ8N2ZmZmZmZmZ8MHwwfDgyMDAwfDQ1MGZlMzk0LTI5MDQtYzlhZC0yMTRjLWEwN2ViN2ZlZWMyOXwoTm8gRGVzY3JpcHRpb24pfDB8MTB8MA==</binary></llsd>\n";
        fixture.ensureParse(
            "binary blob",
            blob,
            string_to_vector("6|6|asdfhappybox|60e44ec5-305c-43c2-9a19-b4b89b1ae2a6|60e44ec5-305c-43c2-9a19-b4b89b1ae2a6|60e44ec5-305c-43c2-9a19-b4b89b1ae2a6|00000000-0000-0000-0000-000000000000|7fffffff|7fffffff|0|0|82000|450fe394-2904-c9ad-214c-a07eb7feec29|(No Description)|0|10|0"),
            1);
    }

    TUT_CASE("llsdserialize_test::TestLLSDXMLParsingObject_test_5")
    {
        ParseFixture<LLSDXMLParser> fixture;

        LLSD level_5 = LLSD::emptyMap(); level_5["level_5"] = 42.f;
        LLSD level_4 = LLSD::emptyMap(); level_4["level_4"] = level_5;
        LLSD level_3 = LLSD::emptyMap(); level_3["level_3"] = level_4;
        LLSD level_2 = LLSD::emptyMap(); level_2["level_2"] = level_3;
        LLSD level_1 = LLSD::emptyMap(); level_1["level_1"] = level_2;
        LLSD level_0 = LLSD::emptyMap(); level_0["level_0"] = level_1;
        LLSD expected = LLSD::emptyMap(); expected["deep"] = level_0;

        fixture.ensureParse(
            "deep llsd xml map",
            "<llsd><map>"
            "<key>deep</key><map>"
            "<key>level_0</key><map>"
            "<key>level_1</key><map>"
            "<key>level_2</key><map>"
            "<key>level_3</key><map>"
            "<key>level_4</key><map>"
            "<key>level_5</key><real>42.0</real>"
            "</map>"
            "</map>"
            "</map>"
            "</map>"
            "</map>"
            "</map>"
            "</map></llsd>",
            expected,
            8);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_1")
    {
        ParseFixture<LLSDNotationParser> fixture;
        fixture.ensureParse("malformed notation map", "{'ha ha'", LLSD(), LLSDParser::PARSE_FAILURE);
        fixture.ensureParse("malformed notation array", "['ha ha'", LLSD(), LLSDParser::PARSE_FAILURE);
        fixture.ensureParse("malformed notation string", "'ha ha", LLSD(), LLSDParser::PARSE_FAILURE);
        fixture.ensureParse("bad notation noise", "g48ejlnfr", LLSD(), LLSDParser::PARSE_FAILURE);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_2")
    {
        ParseFixture<LLSDNotationParser> fixture;
        fixture.ensureParse("valid undef", "!", LLSD(), 1);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_3")
    {
        ParseFixture<LLSDNotationParser> fixture;
        LLSD val = false;
        fixture.ensureParse("valid boolean false 0", "false", val, 1);
        fixture.ensureParse("valid boolean false 1", "f", val, 1);
        fixture.ensureParse("valid boolean false 2", "0", val, 1);
        fixture.ensureParse("valid boolean false 3", "F", val, 1);
        fixture.ensureParse("valid boolean false 4", "FALSE", val, 1);

        val = true;
        fixture.ensureParse("valid boolean true 0", "true", val, 1);
        fixture.ensureParse("valid boolean true 1", "t", val, 1);
        fixture.ensureParse("valid boolean true 2", "1", val, 1);
        fixture.ensureParse("valid boolean true 3", "T", val, 1);
        fixture.ensureParse("valid boolean true 4", "TRUE", val, 1);

        val.clear();
        fixture.ensureParse("invalid true", "TR", val, LLSDParser::PARSE_FAILURE);
        fixture.ensureParse("invalid false", "FAL", val, LLSDParser::PARSE_FAILURE);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_4")
    {
        ParseFixture<LLSDNotationParser> fixture;
        LLSD val = 123;
        fixture.ensureParse("valid integer", "i123", val, 1);
        val.clear();
        fixture.ensureParse("invalid integer", "421", val, LLSDParser::PARSE_FAILURE);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_5")
    {
        ParseFixture<LLSDNotationParser> fixture;
        LLSD val = 456.7;
        fixture.ensureParse("valid real", "r456.7", val, 1);
        val.clear();
        fixture.ensureParse("invalid real", "456.7", val, LLSDParser::PARSE_FAILURE);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_6")
    {
        ParseFixture<LLSDNotationParser> fixture;
        LLUUID id("01234567-89ab-cdef-0123-456789abcdef");
        LLSD val = id;
        fixture.ensureParse("valid uuid", "u01234567-89ab-cdef-0123-456789abcdef", val, 1);
        fixture.ensureParse("unparseable uuid", "u123", LLSD(), LLSDParser::PARSE_FAILURE);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_7")
    {
        ParseFixture<LLSDNotationParser> fixture;

        fixture.ensureParse("valid string 1", "\"foolish\"", LLSD("foolish"), 1);
        fixture.ensureParse("valid string 2", "\"g'day\"", LLSD("g'day"), 1);
        fixture.ensureParse("valid string 3", "'have a \"nice\" day'", LLSD("have a \"nice\" day"), 1);
        fixture.ensureParse("valid string 4", "s(8)\"whatever\"", LLSD("whatever"), 1);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_8")
    {
        ParseFixture<LLSDNotationParser> fixture;
        fixture.ensureParse("valid uri", "l\"http://www.google.com\"", LLSD(LLURI("http://www.google.com")), 1);
        fixture.ensureParse("valid date", "d\"2007-12-28T09:22:53.10Z\"", LLSD(LLDate("2007-12-28T09:22:53.10Z")), 1);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_9")
    {
        ParseFixture<LLSDNotationParser> fixture;
        LLSD val = string_to_vector("abc321");
        fixture.ensureParse("valid binary b64", "b64\"YWJjMzIx\"", val, 1);
        fixture.ensureParse("valid binary b16", "b16\"616263333231\"", val, 1);
        fixture.ensureParse("valid binary raw", "b(6)\"abc321\"", val, 1);

        fixture.ensureParse("size longer than bytes left", "b(5)\"abc321\"", string_to_vector("abc32"), 1);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_10")
    {
        ParseFixture<LLSDNotationParser> fixture;

        LLSD map_val = LLSD::emptyMap();
        map_val["amy"] = 23;
        map_val["bob"] = LLSD();
        map_val["cam"] = 1.23;
        fixture.ensureParse(
            "simple map",
            "{'amy':i23,'bob':!,'cam':r1.23}",
            map_val,
            4);

        LLSD array_val = LLSD::emptyArray();
        array_val.append(23);
        array_val.append(LLSD());
        array_val.append(1.23);
        fixture.ensureParse(
            "simple array",
            "[i23,!,r1.23]",
            array_val,
            4);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_11")
    {
        ParseFixture<LLSDNotationParser> fixture;

        LLSD level_1 = LLSD::emptyMap(); level_1["level_2"] = 99;
        LLSD level_0 = LLSD::emptyMap(); level_0["level_1"] = level_1;
        LLSD root = LLSD::emptyMap();
        root["deep"] = LLSD::emptyMap();
        root["deep"]["level_0"] = level_0;

        fixture.ensureParse(
            "nested notation 3 deep",
            "{'deep' : {'level_0':{'level_1':{'level_2': i99} } } }",
            root,
            5,
            5);
    }

    TUT_CASE("llsdserialize_test::TestLLSDNotationParsingObject_test_12")
    {
        ParseFixture<LLSDNotationParser> fixture;
        fixture.ensureParse(
            "nested notation exceeding depth",
            "{'deep' : {'level_0':{'level_1':{'level_2':{'level_3':{'level_4':{'level_5':{'level_6':{'level_7':{'level_8':{'level_9':i99}"
            "} } } } } } } } } }",
            LLSD(),
            LLSDParser::PARSE_FAILURE,
            9);
    }

    TUT_CASE("llsdserialize_test::TestLLSDCompatibleObject_test_1")
    {
        CompatibilityFixture fixture;
        LLSD value;
        fixture.ensureBinaryAndNotation("undef", value);
        fixture.ensureBinaryAndXML("undef", value);

        value = true;
        fixture.ensureBinaryAndNotation("boolean true", value);
        fixture.ensureBinaryAndXML("boolean true", value);

        value = false;
        fixture.ensureBinaryAndNotation("boolean false", value);
        fixture.ensureBinaryAndXML("boolean false", value);

        value = 1;
        fixture.ensureBinaryAndNotation("integer positive", value);
        fixture.ensureBinaryAndXML("integer positive", value);

        value = -234567;
        fixture.ensureBinaryAndNotation("integer negative", value);
        fixture.ensureBinaryAndXML("integer negative", value);

        value = 0.0;
        fixture.ensureBinaryAndNotation("real zero", value);
        fixture.ensureBinaryAndXML("real zero", value);
    }

    TUT_CASE("llsdserialize_test::TestLLSDCompatibleObject_test_2")
    {
        CompatibilityFixture fixture;
        LLSD value = "foobar";
        fixture.ensureBinaryAndNotation("string", value);
        fixture.ensureBinaryAndXML("string", value);
    }

    TUT_CASE("llsdserialize_test::TestLLSDCompatibleObject_test_3")
    {
        CompatibilityFixture fixture;
        LLSD value = LLUUID("01234567-89ab-cdef-0123-456789abcdef");
        fixture.ensureBinaryAndNotation("uuid", value);
        fixture.ensureBinaryAndXML("uuid", value);
    }

    TUT_CASE("llsdserialize_test::TestLLSDCompatibleObject_test_4")
    {
        CompatibilityFixture fixture;
        LLSD value = LLDate(12345.0);
        fixture.ensureBinaryAndNotation("date", value);
        fixture.ensureBinaryAndXML("date", value);
    }

    TUT_CASE("llsdserialize_test::TestLLSDCompatibleObject_test_5")
    {
        CompatibilityFixture fixture;
        LLSD value = LLURI("http://www.secondlife.com/");
        fixture.ensureBinaryAndNotation("uri", value);
        fixture.ensureBinaryAndXML("uri", value);
    }

    TUT_CASE("llsdserialize_test::TestLLSDCompatibleObject_test_6")
    {
        CompatibilityFixture fixture;
        std::vector<U8> buffer;
        buffer.reserve(128);
        for (int index = 0; index < 128; ++index)
        {
            buffer.push_back(static_cast<U8>((index * 37) % 256));
        }
        LLSD value = buffer;
        fixture.ensureBinaryAndNotation("binary", value);
        fixture.ensureBinaryAndXML("binary", value);
    }

    TUT_CASE("llsdserialize_test::TestLLSDCompatibleObject_test_7")
    {
        CompatibilityFixture fixture;
        LLSD value = LLSD::emptyArray();
        value.append(1);
        value.append("hello");
        value.append(LLUUID("01234567-89ab-cdef-0123-456789abcdef"));
        fixture.ensureBinaryAndNotation("array", value);
        fixture.ensureBinaryAndXML("array", value);
    }

    TUT_CASE("llsdserialize_test::TestLLSDCompatibleObject_test_8")
    {
        CompatibilityFixture fixture;
        LLSD value = LLSD::emptyMap();
        value["foo"] = "bar";
        value["baz"] = 100;
        fixture.ensureBinaryAndNotation("map", value);
        fixture.ensureBinaryAndXML("map", value);
    }

    // The original suite also included Python interop and filesystem-based tests.
    // They are intentionally omitted here because they require external tools and I/O.
}
