#ifndef RMIOC_FLAGS_HPP
#define RMIOC_FLAGS_HPP

#include <boost/preprocessor.hpp>

#define RMIOC_FLAGS_DEFINE_ENTRY(r, data, i, name) \
    static constexpr auto name = 1U << i; \
    auto BOOST_PP_CAT(has_, name) () const -> bool \
    { \
        return this->bits[i]; \
    } \
    void BOOST_PP_CAT(set_, name) (bool flag) \
    { \
        this->bits[i] = flag; \
    }

#define RMIOC_FLAGS_DEFINE_IMPL(name, entries) \
    class name \
    { \
    public: \
        name() = default; \
        name(unsigned long val) : bits(val) {} \
        BOOST_PP_SEQ_FOR_EACH_I(RMIOC_FLAGS_DEFINE_ENTRY,, entries) \
    private: \
        std::bitset<BOOST_PP_SEQ_SIZE(entries)> bits; \
    }

/**
 * Generate the definition of a type-safe flags class.
 *
 * @param name Name of the class.
 * @param fields... Elements of the flags field.
 * @example `RMIOC_FLAGS_DEFINE(flags, a, b, c);` generates:
 *
 * class flags
 * {
 * public:
 *     flags() = default;
 *     flags(unsigned long val) : bits(val) {}
 *
 *     static constexpr auto a = 1U << 0;
 *     auto has_a () const -> bool { return this->bits[0]; }
 *     void set_a(bool flag) { this->bits[0] = flag; }
 *
 *     static constexpr auto b = 1U << 1;
 *     auto has_b () const -> bool { return this->bits[1]; }
 *     void set_b (bool flag) { this->bits[1] = flag; }
 *
 *     static constexpr auto c = 1U << 2;
 *     auto has_c () const -> bool { return this->bits[2]; }
 *     void set_c (bool flag) { this->bits[2] = flag; }
 *
 * private:
 *     std::bitset<3> bits;
 * };
 */
#define RMIOC_FLAGS_DEFINE(name, ...) \
    RMIOC_FLAGS_DEFINE_IMPL(name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#endif // RMIOC_FLAGS_HPP
