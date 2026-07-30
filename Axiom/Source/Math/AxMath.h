#pragma once
#include <cmath>
#include <format>
#include <limits>
#include <random>
#include <sstream>
#include <string>

namespace Math {
    // ------- Axiom Math Constants -------
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TAU = 6.28318530717958647692f;        // 2 * PI
    constexpr float HALF_PI = 1.57079632679489661923f;    // PI / 2
    constexpr float QUARTER_PI = 0.78539816339744830962f; // PI / 4
    constexpr float DEG_TO_RAD = 0.01745329251994329577f; // PI / 180
    constexpr float RAD_TO_DEG = 57.2957795130823208768f; // 180 / PI
    constexpr float E = 2.71828182845904523536f;
    constexpr float SQRT2 = 1.41421356237309504880f; // sqrt(2)
    constexpr float SQRT3 = 1.73205080756887729352f; // sqrt(3)

    constexpr float INF = std::numeric_limits<float>::infinity();
    constexpr float EPSILON = std::numeric_limits<float>::epsilon();

    // ------- Axiom Math functions -------
    float sin(float radians);
    float cos(float radians);
    float tan(float radians);
    float asin(float value);
    float acos(float value);
    float atan(float value);
    float sqrt(float value);
    float pow(float base, float exponent);
    float abs(float value);
    float toRadians(float degrees);
    float clamp(float value, float min, float max);
    float linearInterpolation(float a, float b, float t);

    bool isPowerOfTwo(uint64_t value);
    // Returns a random float in the range [0.0, 1.0)
    float random();
    float random(float seed);
    // Returns a random float in the range [min, max]
    float random(float min, float max);
    float random(float min, float max, float seed);
    // Returns a random integer in the range [min, max]
    int randomInt(int min, int max);
    int randomInt(int min, int max, float seed);

    // ------- Axiom Math Utilities -------
    uint32_t megabytes(uint32_t mb);
    uint32_t gigabytes(uint32_t gb);
} // namespace Math

// TODO: implement SIMD versions of these types and improve the overall implementation
#include "Mat.h"
#include "Rect.h"
#include "Vec.h"

// ------- Axiom Math Type Formatters -------
template <typename T, size_t N, typename CharT> struct std::formatter<Math::Vec<T, N>, CharT> {
    std::formatter<T, CharT> elem;

    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
        auto it = ctx.begin();
        auto end = ctx.end();
        if (it == end || *it == CharT('}'))
            return it;
        return elem.parse(ctx);
    }

    template <typename FormatContext> auto format(const Math::Vec<T, N>& v, FormatContext& ctx) const {
        auto out = ctx.out();
        *out++ = CharT('V');
        *out++ = CharT('e');
        *out++ = CharT('c');
        if constexpr (std::is_same_v<CharT, char>) {
            out = std::format_to(out, "{}", N);
        } else {
            out = std::format_to(out, L"{}", N);
        }
        *out++ = CharT('(');
        for (std::size_t i = 0; i < N; ++i) {
            out = elem.format(v[i], ctx);
            if (i + 1 < N) {
                *out++ = CharT(',');
                *out++ = CharT(' ');
            }
        }
        *out++ = CharT(')');
        return out;
    }
};

template <typename T, size_t N, typename CharT> struct std::formatter<Math::Mat<T, N>, CharT> {
    std::formatter<T, CharT> elem_;

    std::formatter<std::size_t, CharT> idx_;

    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
        auto it = ctx.begin();
        auto end = ctx.end();
        if (it == end || *it == CharT('}'))
            return it;
        return elem_.parse(ctx);
    }

    template <typename FormatContext> auto format(const Math::Mat<T, N>& m, FormatContext& ctx) const {
        auto out = ctx.out();

        *out++ = CharT('M');
        *out++ = CharT('a');
        *out++ = CharT('t');
        out = idx_.format(N, ctx);
        *out++ = CharT('[');
        for (std::size_t i = 0; i < N; ++i) {
            *out++ = CharT('(');
            for (std::size_t j = 0; j < N; ++j) {
                out = elem_.format(m[j][i], ctx);
                if (j + 1 < N) {
                    *out++ = CharT(',');
                    *out++ = CharT(' ');
                }
            }
            *out++ = CharT(')');
            if (i + 1 < N) {
                *out++ = CharT(',');
                *out++ = CharT(' ');
            }
        }
        *out++ = CharT(']');
        return out;
    }
};
