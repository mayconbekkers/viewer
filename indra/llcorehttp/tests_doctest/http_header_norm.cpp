/**
 * @file http_header_norm.cpp
 * @brief Implementação das utilidades de normalização de cabeçalhos HTTP.
 */

#include "http_header_norm.h"

#include "linden_common.h"
#include "llstring.h"

#include <cctype>
#include <unordered_map>
#include <unordered_set>

namespace llcorehttp_test
{

namespace
{
bool is_space(char ch)
{
    return ch == ' ' || ch == '\t';
}
} // namespace

std::string normalize_header_name(std::string_view name)
{
    std::string result(name);
    LLStringUtil::trim(result);
    LLStringUtil::toLower(result);
    return result;
}

std::string unfold_legacy_lines(const std::string& value)
{
    std::string result;
    result.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '\r' && (i + 1) < value.size() && value[i + 1] == '\n')
        {
            i += 2;
            while (i < value.size() && (value[i] == ' ' || value[i] == '\t'))
            {
                ++i;
            }
            if (!result.empty() && result.back() != ' ')
            {
                result.push_back(' ');
            }
            --i; // compensar incremento do for
        }
        else
        {
            result.push_back(value[i]);
        }
    }
    return result;
}

std::string normalize_header_value(const std::string& value)
{
    std::string unfolded = unfold_legacy_lines(value);
    LLStringUtil::trim(unfolded);

    std::string collapsed;
    collapsed.reserve(unfolded.size());
    bool last_was_space = false;

    for (unsigned char ch : unfolded)
    {
        if (std::isspace(ch))
        {
            if (!last_was_space && !collapsed.empty())
            {
                collapsed.push_back(' ');
            }
            last_was_space = true;
        }
        else
        {
            collapsed.push_back(static_cast<char>(ch));
            last_was_space = false;
        }
    }

    return collapsed;
}

HeaderList canonicalize_headers(const HeaderList& raw)
{
    HeaderList canonical;
    canonical.reserve(raw.size());
    for (const auto& entry : raw)
    {
        canonical.emplace_back(normalize_header_name(entry.first),
                               normalize_header_value(entry.second));
    }
    return canonical;
}

HeaderBuckets merge_duplicates(const HeaderList& canonical)
{
    HeaderBuckets buckets;
    std::unordered_map<std::string, std::size_t> index;

    for (const auto& entry : canonical)
    {
        const auto iter = index.find(entry.first);
        if (iter == index.end())
        {
            index.emplace(entry.first, buckets.size());
            buckets.emplace_back(entry.first, std::vector<std::string>{entry.second});
        }
        else
        {
            buckets[iter->second].second.push_back(entry.second);
        }
    }
    return buckets;
}

HeaderList collapse_merged(const HeaderBuckets& buckets)
{
    static const std::unordered_set<std::string> kCommaJoin = {
        "accept",
        "accept-encoding",
        "accept-language",
        "cache-control",
        "pragma",
        "warning"
    };

    HeaderList flattened;
    for (const auto& bucket : buckets)
    {
        if (bucket.second.empty())
        {
            continue;
        }

        if (kCommaJoin.count(bucket.first))
        {
            std::string combined;
            for (std::size_t i = 0; i < bucket.second.size(); ++i)
            {
                if (i)
                {
                    combined.append(", ");
                }
                combined.append(bucket.second[i]);
            }
            flattened.emplace_back(bucket.first, combined);
        }
        else
        {
            for (const auto& value : bucket.second)
            {
                flattened.emplace_back(bucket.first, value);
            }
        }
    }
    return flattened;
}

} // namespace llcorehttp_test
