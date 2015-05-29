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
#ifndef SCRIPT_HANDLERS_HPP
#define SCRIPT_HANDLERS_HPP

#include "handler/handler.hpp"
#include "python.hpp"

namespace python {

/**
 * \brief Runs a python script
 */
class SimplePython : public handler::SimpleAction
{
public:
    SimplePython(const Settings& settings, MessageConsumer* parent)
        : SimpleAction(settings.get("trigger",settings.get("script","")),settings,parent)
    {
        std::string script_rel = settings.get("script","");
        if ( script_rel.empty() )
            throw ConfigurationError("Missing script file");

        script = settings::data_file("scripts/"+script_rel);
        if ( script.empty() )
            throw ConfigurationError("Script file not found: "+script_rel);

        synopsis += settings.get("synopsis","");
        help = settings.get("help", "Runs "+script_rel);
        discard_error = settings.get("discard_error", discard_error);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        PropertyTree properties;
        properties.put_child("source", properties_to_tree(msg.source->message_properties()));
        properties.put_child("user", properties_to_tree(msg.from.properties));
        properties.put("user.name",msg.from.name);
        properties.put("user.channels",string::implode(",",msg.from.channels));
        properties.put("user.global_id",msg.from.global_id);
        properties.put("user.host",msg.from.host);
        properties.put("user.local_id",msg.from.local_id);
        properties.put("message", msg.message);
        properties.put("input", msg.raw);
        properties.put("channels", string::implode(",",msg.channels));

        /// \todo Timeout
        auto output = python::PythonEngine::instance().exec_file(script,properties);
        if ( output.success || !discard_error )
            for ( const auto& line : output.output )
                reply_to(msg,line);

        return true;
    }

private:
    std::string script;         ///< Script file path
    bool discard_error = true;  ///< If \b true, only prints output of scripts that didn't fail
};

} // namespace python
#endif // SCRIPT_HANDLERS_HPP