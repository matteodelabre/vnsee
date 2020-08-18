/**
 * @file options.hpp
 * @brief Minimalist header-only C++17 command line parsing function.
 * @author Mattéo Delabre
 * @see https://forge.delab.re/matteo/options
 * @license
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org>
 */

#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>
#include <map>
#include <vector>

namespace options
{
using Key = std::string;
using Value = std::string;
using Values = std::vector<Value>;
using Dictionary = std::map<Key, Values>;

/**
 * Parse a list of command line arguments into a set of program options.
 *
 * This implementation strives to follow (in decreasing precedence) the
 * guidelines laid out in Chapter 12 of the POSIX.1-2017 specification (see
 * <http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html>);
 * the conventions of Chapter 10 in “The Art of Unix Programming” by Eric S.
 * Raymond; and the author's personal habits.
 *
 * @param start Iterator to the initial argument.
 * @param end Past-the-end iterator to signal the argument list’s end.
 * @return A pair with, first: a dictionary containing for each option present
 * on the command line a key-value pair, with the option’s name as the key, and
 * the list of option-arguments associated to that option, following the order
 * of the command line, as the value. If an option has no associated
 * option-arguments (i.e. is a flag), the value in the dictionary is an empty
 * list; second: a list in which all operands are grouped following the order
 * of the command line.
 */
template<typename Iterator>
auto parse(Iterator start, Iterator end)
{
    Dictionary opts;
    Values operands;

    // Signals whether a “--” argument was encountered, which implies that
    // all further arguments are to be treated as operands
    bool operands_only = false;

    // Array into which the next value should be pushed
    Values* value_collector = &operands;

    for (; start != end; ++start)
    {
        const auto current = *start;

        if (current[0] == '-' && current[1] != '\0' && !operands_only)
        {
            if (current[1] == '-')
            {
                if (current[2] == '\0')
                {
                    operands_only = true;
                    value_collector = &operands;
                }
                else
                {
                    // GNU-style long option. The option's argument can either
                    // be inside the same argument, separated by an “=”
                    // character, or inside the following argument (therefore
                    // simply separated by whitespace)
                    const char* key_end = current + 2;
                    while (*key_end != '\0' && *key_end != '=') ++key_end;

                    auto [item, _] = opts.emplace(
                        std::string(current + 2, key_end - current - 2),
                        Values{});
                    value_collector = &item->second;

                    if (*key_end == '=')
                    {
                        value_collector->emplace_back(key_end + 1);
                        value_collector = &operands;
                    }
                }
            }
            else
            {
                // Unix-style single-character option. Several short options
                // can be grouped together inside the same argument; in this
                // case, only the last option can have an option-argument
                const char* letter = current + 1;

                while (*letter != '\0')
                {
                    auto [item, _] = opts.emplace(Key{*letter}, Values{});
                    value_collector = &item->second;
                    ++letter;
                }
            }

        }
        else
        {
            // Either an option-argument or an operand
            value_collector->emplace_back(current);
            value_collector = &operands;
        }
    }

    return std::make_pair(opts, operands);
}
}

#endif // OPTIONS_HPP
