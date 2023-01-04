#pragma once

#include <compare>
#include <cstdlib>
#include <type_traits>

#define GNUNETPP_OPERATOR_COMPOARE_RAW_DATA(type) \
inline std::strong_ordering operator<=>(const type& lhs, const type& rhs) \
{ \
    static_assert(std::is_trivial_v<type>); \
    return memcmp(&lhs, &rhs, sizeof(type)) <=> 0; \
} \
inline bool operator==(const type& lhs, const type& rhs) \
{ \
    return (lhs <=> rhs) == 0; \
} \
inline bool operator!=(const type& lhs, const type& rhs) \
{ \
    return !(lhs == rhs); \
} \
inline bool operator<(const type& lhs, const type& rhs) \
{ \
    return (lhs <=> rhs) < 0; \
} \
inline bool operator>(const type& lhs, const type& rhs) \
{ \
    return (lhs <=> rhs) > 0; \
} \
inline bool operator<=(const type& lhs, const type& rhs) \
{ \
    return (lhs <=> rhs) <= 0; \
} \
inline bool operator>=(const type& lhs, const type& rhs) \
{ \
    return (lhs <=> rhs) >= 0; \
}

#define GNUNETPP_RAW_DATA_HASH(type) \
namespace std \
{ \
template <> \
struct hash<type> \
{ \
    size_t operator()(const type& data) const \
    { \
        static_assert(std::is_trivial_v<type>); \
        static_assert(sizeof(type) % sizeof(size_t) == 0); \
        size_t result = 0; \
        const size_t* ptr = reinterpret_cast<const size_t*>(&data); \
        for (size_t i = 0; i < sizeof(type) / sizeof(size_t); ++i) \
            result ^= ptr[i]; \
        return result; \
    } \
}; \
}
