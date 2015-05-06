/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \section License
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "irc_formatter.hpp"

#include <regex>

#include "string/string_functions.hpp"
#include "string/encoding.hpp"

namespace irc {

std::string FormatterIrc::color(const color::Color12& color) const
{
    int ircn = 1;

    if ( !color.is_valid() )
        return "\xf"; // no color

    switch ( color.to_4bit() )
    {
        case 0b0000: ircn =  1; break; // black
        case 0b0001: ircn =  5; break; // dark red
        case 0b0010: ircn =  3; break; // dark green
        case 0b0011: ircn =  7; break; // dark yellow
        case 0b0100: ircn =  2; break; // dark blue
        case 0b0101: ircn =  6; break; // dark magenta
        case 0b0110: ircn = 10; break; // dark cyan
        case 0b0111: ircn = 15; break; // silver
        case 0b1000: ircn = 14; break; // grey
        case 0b1001: ircn =  4; break; // red
        case 0b1010: ircn =  9; break; // green
        case 0b1011: ircn =  8; break; // yellow
        case 0b1100: ircn = 12; break; // blue
        case 0b1101: ircn = 13; break; // magenta
        case 0b1110: ircn = 11; break; // cyan
        case 0b1111: ircn =  0; break; // white
    }
    return '\3'+string::to_string(ircn,2);
}
std::string FormatterIrc::format_flags(string::FormatFlags flags) const
{
    if ( flags == string::FormatFlags::NO_FORMAT )
        return "\xf"; /// \note clears color as well
    std::string format;
    if ( flags & string::FormatFlags::BOLD )
        format += "\x2";
    if ( flags & string::FormatFlags::UNDERLINE )
        format += "\x1f";
    return format;
}
std::string FormatterIrc::clear() const
{
    return "\xf";
}
string::FormattedString FormatterIrc::decode(const std::string& source) const
{
    string::FormattedString str;

    string::Utf8Encoding parser;

    string::FormatFlags flags;

    std::string ascii;

    auto push_flags = [&flags,&str,&ascii](bool force_ascii)
    {
        if ( ( force_ascii || flags )  && !ascii.empty() )
        {
            str.append<string::AsciiSubstring>(ascii);
            ascii.clear();
        }
        if ( flags )
        {
            str.append<string::Format>(flags);
            flags = string::FormatFlags::NO_FORMAT;
        }
    };

    parser.callback_ascii = [&str,&flags,&parser,&ascii,&push_flags](uint8_t byte)
    {
        // see https://github.com/myano/jenni/wiki/IRC-String-Formatting
        switch ( byte )
        {
            case '\2':
                if ( !(flags & string::FormatFlags::BOLD) )
                    flags |= string::FormatFlags::BOLD;
                break;
            case '\x1f':
                if ( !(flags & string::FormatFlags::UNDERLINE) )
                    flags |= string::FormatFlags::UNDERLINE;
                break;
            case '\xf':
                flags = string::FormatFlags::NO_FORMAT;
                push_flags(true);
                str.append<string::ClearFormatting>();
                break;
            case '\3':
            {
                static std::regex regex_irc_color (
                    "([0-9]{1,2})(?:,[0-9]{1,2})?",
                    std::regex::optimize|std::regex::ECMAScript);
                std::smatch match;
                parser.input.get_regex(regex_irc_color,match);
                push_flags(true);
                str.append<string::Color>(FormatterIrc::color_from_string(match[1]));
                break;
            }
            case '\x1d': case '\x16':
                // Skip unsupported format flags
                break;
            default:
                push_flags(false);
                ascii += byte;
                break;
        }
    };
    parser.callback_utf8 = [&str,&push_flags](uint32_t unicode,const std::string& utf8)
    {
        push_flags(true);
        str.append<string::Unicode>(utf8,unicode);
    };

    parser.parse(source);
    push_flags(true);

    return str;
}
std::string FormatterIrc::name() const
{
    return "irc";
}

color::Color12 FormatterIrc::color_from_string(const std::string& color)
{
    using namespace color;

    static std::regex regex = std::regex( "\3?([0-9]{1,2})",
        std::regex_constants::syntax_option_type::optimize |
        std::regex_constants::syntax_option_type::ECMAScript
    );

    std::smatch match;
    if ( !std::regex_match(color,match,regex) )
        return Color12();

    switch ( string::to_uint(match[1].str()) )
    {
        case  0: return white;
        case  1: return black;
        case  2: return dark_blue;
        case  3: return dark_green;
        case  4: return red;
        case  5: return dark_red;
        case  6: return dark_magenta;
        case  7: return dark_yellow;
        case  8: return yellow;
        case  9: return green;
        case 10: return dark_cyan;
        case 11: return cyan;
        case 12: return blue;
        case 13: return magenta;
        case 14: return gray;
        case 15: return silver;
        default: return Color12();
    }
}

std::string FormatterIrcWhite::color(const color::Color12& color) const
{
    if ( color.is_valid() )
    {
        switch ( color.to_4bit() )
        {
            case 0b1011: // yellow -> dark yellow
                return FormatterIrc::color(color::dark_yellow);
            case 0b1111: // white -> black
                return FormatterIrc::color(color::black);
        }
    }
    return FormatterIrc::color(color);
}

std::string FormatterIrcWhite::name() const
{
    return "irc_white";
}

} // namespace irc


