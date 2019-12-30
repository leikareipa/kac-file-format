/*
 * Tarpeeksi Hyvae Soft 2019
 * 
 * Helper functions for basic string manipulation.
 *
 */

#ifndef OBJ2KAC_STRING_UTILS_H
#define OBJ2KAC_STRING_UTILS_H

#include <string>
#include <vector>

namespace obj2kac::string_utils
{
    // Returns a copy of the given string with leading and trailing whitespace removed.
    std::string trimmed_string(const std::string &string);

    // Splits the given string by the given delimiter and returns the splittings as a
    // vector.
    std::vector<std::string> string_split(const std::string &string, const char delimiter);
}

#endif
