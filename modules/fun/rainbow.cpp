/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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

#include "rainbow.hpp"
#include "melanolib/string/ascii.hpp"

namespace fun {

string::FormattedString FormatterRainbow::decode(const std::string& source) const
{
    string::FormattedString str;
    std::vector<color::Color12*> colors;

    melanolib::string::Utf8Parser parser(source);

    while ( !parser.finished() )
    {
        melanolib::string::Utf8Parser::Byte byte = parser.next_ascii();
        if ( melanolib::string::ascii::is_ascii(byte) )
        {
            string::Element color(color::nocolor);
            colors.push_back(&color.reference<color::Color12>());
            str.append(std::move(color));
            str.append(char(byte));
        }
        else
        {
            melanolib::string::Unicode unicode = parser.next();
            if ( unicode.valid() )
            {
                string::Element color(color::nocolor);
                colors.push_back(&color.reference<color::Color12>());
                str.append(std::move(color));
                str.append(std::move(unicode));
            }
        }
    }

    for ( unsigned i = 0; i < colors.size(); i++ )
        *colors[i] = color::Color12::hsv(hue+double(i)/colors.size(), saturation, value);

    return str;
}
std::string FormatterRainbow::name() const
{
    return "rainbow";
}

} // namespace fun
