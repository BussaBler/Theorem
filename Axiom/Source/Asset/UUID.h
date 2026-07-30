#pragma once
#include <format>
#include <iomanip>
#include <random>
#include <sstream>

namespace Axiom {
    class UUID {
      public:
        UUID() : value(0) {}
        UUID(uint64_t value) : value(value) {}
        ~UUID() = default;

        inline static UUID generate() { return UUID(distribution(generator)); }

        inline bool isValid() const { return value != 0; }

        operator uint64_t() const { return value; }

      private:
        static std::random_device randomDevice;
        static std::mt19937_64 generator;
        static std::uniform_int_distribution<uint64_t> distribution;

        uint64_t value;
    };
} // namespace Axiom

namespace std {
    template <> struct hash<Axiom::UUID> {
        size_t operator()(const Axiom::UUID& uuid) const { return hash<uint64_t>()(static_cast<uint64_t>(uuid)); }
    };

    template <> struct formatter<Axiom::UUID> {
        constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

        auto format(const Axiom::UUID& uuid, format_context& ctx) {
            std::stringstream ss;
            ss << std::hex << std::setw(16) << std::setfill('0') << static_cast<uint64_t>(uuid);
            return std::format_to(ctx.out(), "{}", ss.str());
        }

        auto format(const Axiom::UUID& uuid, std::format_context& ctx) const {
            std::stringstream ss;
            ss << std::hex << std::setw(16) << std::setfill('0') << static_cast<uint64_t>(uuid);
            return std::format_to(ctx.out(), "{}", ss.str());
        }
    };
} // namespace std
