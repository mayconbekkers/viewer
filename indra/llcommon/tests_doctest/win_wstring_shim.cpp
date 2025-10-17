#include "linden_common.h"
#include "llstring.h"

// llfile.cpp expects this overload when llwchar is not wchar_t.
std::string ll_convert_wide_to_string(const unsigned short* in, std::size_t len, unsigned int code_page)
{
    return ll_convert_wide_to_string(reinterpret_cast<const wchar_t*>(in), len, code_page);
}

