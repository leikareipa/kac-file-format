/*
 * Tarpeeksi Hyvae Soft 2019
 * 
 * Helper functions for basic string manipulation.
 *
 */

#include <sstream>
#include "string_utils.h"

namespace obj2kac::string_utils
{
    std::string trimmed_string(const std::string &string)
    {
        const std::string whitespaceCharacters = " \t\n\v\r\f";

        const auto start = string.find_first_not_of(whitespaceCharacters);
        const auto end = string.find_last_not_of(whitespaceCharacters);

        if ((start == std::string::npos) ||
            (end == std::string::npos))
        {
            return "";
        }

        return string.substr(start, (end - start + 1));
    }

    std::vector<std::string> string_split(const std::string &string, const char delimiter)
    {
        if (string.find(delimiter) == std::string::npos)
        {
            return {string};
        }

        std::vector<std::string> split;
        std::istringstream stringStream(string);
        std::string tmpBuffer;

        while (std::getline(stringStream, tmpBuffer, delimiter))
        {
            split.push_back(trimmed_string(tmpBuffer));
        }

        return split;
    }
}
