#pragma once
#include "Core/Assert.h"

#include <array>
#include <cmath>
#include <sstream>
#include <string>

namespace Math {
    template <typename T, size_t N> struct Vec {
        Vec() = default;

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>> Vec(const Vec<U, N>& other) {
            for (size_t i = 0; i < N; ++i)
                data[i] = static_cast<T>(other[i]);
        }

        template <typename... Scalars, typename = std::enable_if_t<(sizeof...(Scalars) == N) && (std::conjunction_v<std::is_convertible<Scalars, T>...>)>>
        Vec(Scalars... s) : data{{static_cast<T>(s)...}} {}

        Vec(T value) {
            for (size_t i = 0; i < N; ++i) {
                data[i] = value;
            }
        }

        // Constructs from a string of the form "x,y,z,..."
        Vec(const std::string& str) {
            std::stringstream ss(str);
            char comma;
            for (size_t i = 0; i < N; ++i) {
                ss >> data[i];
                if (i < N - 1)
                    ss >> comma;
            }
        }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>> explicit operator Vec<U, N>() const {
            Vec<U, N> result;
            for (size_t i = 0; i < N; ++i)
                result[i] = static_cast<U>(data[i]);
            return result;
        }

        bool operator==(const Vec<T, N>& other) const {
            bool isNear = true;
            for (size_t i = 0; i < N; i++) {
                if (abs(data[i] - other.data[i]) >= std::numeric_limits<float>::epsilon()) {
                    isNear = false;
                }
            }
            return isNear;
        }

        static Vec<T, N> zero() { return Vec<T, N>(); }

        static Vec<T, N> one() {
            Vec<T, N> result;
            for (size_t i = 0; i < N; ++i) {
                result.data[i] = static_cast<T>(1);
            }
            return result;
        }

        T& operator[](size_t i) { return data[i]; }

        const T& operator[](size_t i) const { return data[i]; }

        Vec<T, N> operator+(const Vec<T, N>& other) const {
            Vec<T, N> result;
            for (size_t i = 0; i < N; ++i) {
                result.data[i] = data[i] + other.data[i];
            }
            return result;
        }

        Vec<T, N> operator-(const Vec<T, N>& other) const {
            Vec<T, N> result;
            for (size_t i = 0; i < N; ++i) {
                result.data[i] = data[i] - other.data[i];
            }
            return result;
        }

        Vec<T, N> operator-() const {
            Vec<T, N> result;
            for (size_t i = 0; i < N; ++i) {
                result.data[i] = -data[i];
            }
            return result;
        }

        Vec<T, N> operator*(const T& scalar) const {
            Vec<T, N> result;
            for (size_t i = 0; i < N; ++i) {
                result.data[i] = data[i] * scalar;
            }
            return result;
        }

        Vec<T, N> operator/(const T& scalar) const {
            AX_CORE_ASSERT(scalar != static_cast<T>(0), "Division by zero");
            Vec<T, N> result;
            for (size_t i = 0; i < N; ++i) {
                result.data[i] = data[i] / scalar;
            }
            return result;
        }

        Vec<T, N>& operator+=(const Vec<T, N>& other) {
            for (size_t i = 0; i < N; ++i) {
                data[i] += other.data[i];
            }
            return *this;
        }

        Vec<T, N>& operator-=(const Vec<T, N>& other) {
            for (size_t i = 0; i < N; ++i) {
                data[i] -= other.data[i];
            }
            return *this;
        }

        Vec<T, N>& operator*=(const T& scalar) {
            for (size_t i = 0; i < N; ++i) {
                data[i] *= scalar;
            }
            return *this;
        }

        Vec<T, N>& operator/=(const T& scalar) {
            AX_CORE_ASSERT(scalar != static_cast<T>(0), "Division by zero");
            for (size_t i = 0; i < N; ++i) {
                data[i] /= scalar;
            }
            return *this;
        }

        friend Vec<T, N> operator*(const T& scalar, const Vec<T, N>& vec) { return vec * scalar; }

        T x() const
            requires(N >= 2)
        {
            return data[0];
        }

        T y() const
            requires(N >= 2)
        {
            return data[1];
        }

        T z() const
            requires(N >= 3)
        {
            return data[2];
        }

        T w() const
            requires(N >= 4)
        {
            return data[3];
        }

        T u() const
            requires(N >= 2)
        {
            return data[0];
        }

        T v() const
            requires(N >= 2)
        {
            return data[1];
        }

        T r() const
            requires(N >= 2)
        {
            return data[0];
        }

        T g() const
            requires(N >= 2)
        {
            return data[1];
        }

        T b() const
            requires(N >= 3)
        {
            return data[2];
        }

        T a() const
            requires(N >= 4)
        {
            return data[3];
        }

        T& x()
            requires(N >= 2)
        {
            return data[0];
        }

        T& y()
            requires(N >= 2)
        {
            return data[1];
        }

        T& z()
            requires(N >= 3)
        {
            return data[2];
        }

        T& w()
            requires(N >= 4)
        {
            return data[3];
        }

        T& u()
            requires(N >= 2)
        {
            return data[0];
        }

        T& v()
            requires(N >= 2)
        {
            return data[1];
        }

        T& r()
            requires(N >= 2)
        {
            return data[0];
        }

        T& g()
            requires(N >= 2)
        {
            return data[1];
        }

        T& b()
            requires(N >= 3)
        {
            return data[2];
        }

        T& a()
            requires(N >= 4)
        {
            return data[3];
        }

      protected:
        std::array<T, N> data{};
    };

    using Vec2 = Vec<float, 2>;
    using Vec3 = Vec<float, 3>;
    using Vec4 = Vec<float, 4>;
    using iVec2 = Vec<int, 2>;
    using iVec3 = Vec<int, 3>;
    using uVec2 = Vec<uint32_t, 2>;

    template <typename T, size_t N> inline T dot(const Vec<T, N>& a, const Vec<T, N>& b) {
        T result = static_cast<T>(0);
        for (size_t i = 0; i < N; ++i) {
            result += a[i] * b[i];
        }
        return result;
    }

    template <typename T, size_t N> inline T lengthSqr(const Vec<T, N>& v) {
        return dot(v, v);
    }

    template <typename T, size_t N> inline T length(const Vec<T, N>& v) {
        return static_cast<T>(std::sqrt(static_cast<long double>(dot(v, v))));
    }

    template <typename T, size_t N> inline Vec<T, N> normalize(const Vec<T, N>& v) {
        T len = length(v);
        AX_CORE_ASSERT(static_cast<float>(len) > std::numeric_limits<float>::epsilon(), "Cannot normalize a zero-length vector");
        return v / len;
    }

    template <typename T, size_t N> inline T distance(const Vec<T, N>& a, const Vec<T, N>& b) {
        return length(a - b);
    }

    template <typename T> inline float cross(const Vec<T, 2>& a, const Vec<T, 2>& b) {
        return a.x() * b.y() - a.y() * b.x();
    }

    template <typename T> inline Vec3 cross(const Vec<T, 3>& a, const Vec<T, 3>& b) {
        return Vec3(a.y() * b.z() - a.z() * b.y(), a.z() * b.x() - a.x() * b.z(), a.x() * b.y() - a.y() * b.x());
    }

    template <typename T, size_t N> inline Vec<T, N> linearInterpolation(Vec<T, N> a, Vec<T, N> b, float t) {
        return a + t * (b - a);
    }
} // namespace Math
