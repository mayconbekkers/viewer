/**
 * @file http_header_norm.h
 * @brief Declarar utilidades determinísticas para normalização de cabeçalhos HTTP nos testes.
 */
#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace llcorehttp_test
{

using HeaderList = std::vector<std::pair<std::string, std::string>>;
using HeaderBuckets = std::vector<std::pair<std::string, std::vector<std::string>>>;

std::string normalize_header_name(std::string_view name);
std::string unfold_legacy_lines(const std::string& value);
std::string normalize_header_value(const std::string& value);

HeaderList canonicalize_headers(const HeaderList& raw);
HeaderBuckets merge_duplicates(const HeaderList& canonical);
HeaderList collapse_merged(const HeaderBuckets& buckets);

} // namespace llcorehttp_test

