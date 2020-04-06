#include "log.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>

namespace log
{

#ifdef TRACE

template<typename String>
std::ostream& print(const String& kind)
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(now)
        .count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now)
        .count() % 1000000;

    return std::cerr
        << secs << '.'
        << std::setfill('0') << std::setw(6) << micros
        << " [" << kind << "] ";
}

#else

namespace detail
{

class NullBuffer : public std::streambuf
{
public:
    int overflow(int c) { return c; }
};

class NullStream : public std::ostream
{
public:
    NullStream() : std::ostream(&buf) {}

private:
    NullBuffer buf;
};

static NullStream null_stream;

} // namespace log::detail

template<typename String>
std::ostream& print(const String&)
{
    return detail::null_stream;
}

#endif // TRACE

} // namespace log
