#pragma once

#include <string>
#include <string_view>

namespace utility {
    //
    // String utilities.
    //

    // Conversion functions for UTF8<->UTF16.
    std::string narrow(std::wstring_view str);
    std::wstring widen(std::string_view str);

    static constexpr unsigned int Fnv1aBasis = 0x811C9DC5;
    static constexpr unsigned int Fnv1aPrime = 0x01000193;

    constexpr unsigned int hashFnv1a(const char *s, unsigned int h = Fnv1aBasis)
    {
        return !*s ? h : hashFnv1a(s + 1, (h ^ *s) * Fnv1aPrime);
    }

    constexpr unsigned int hashFnv1b(const char *s, unsigned int h = Fnv1aBasis)
    {
        return !*s ? h : hashFnv1b(s + 1, static_cast<unsigned int>((h ^ *s) * static_cast<unsigned long long>(Fnv1aPrime)));
    }
}

constexpr auto operator "" _fnv(const char* s, size_t) -> unsigned {
    return static_cast<unsigned>(utility::hashFnv1a(s));
}