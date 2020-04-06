#ifndef LOG_HPP
#define LOG_HPP

#include <ostream>

namespace log
{

/**
 * Print a log header and return the log stream for printing further data.
 *
 * @param kind Kind of header to print.
 * @return Log stream for printing the log message.
 */
template<typename String>
std::ostream& print(const String& kind);

}

#include "log.tpp" // IWYU pragma: export

#endif // LOG_HPP
