/**
 * \file
 * \brief This file defines handlers which allows admin to administrate the bot
 *        (IRC-specific actions)
 * 
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
#ifndef IRC_HANDLER_ADMIN
#define IRC_HANDLER_ADMIN

#include "handler/handler.hpp"

namespace irc {
namespace handler {

/**
 * \brief Changes the bot nick (IRC)
 */
class AdminNick: public ::handler::SimpleAction
{
public:
    AdminNick(const Settings& settings, ::handler::HandlerContainer* parent)
        : SimpleAction("nick",settings,parent)
    {
        synopsis += " nickname";
        help = "Changes the bot nickname";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( !msg.message.empty() )
        {
            msg.destination->command({"NICK",{msg.message}});
            return true;
        }
        return false;
    }
};

/**
 * \brief Makes the bot join channels (IRC)
 */
class AdminJoin: public ::handler::SimpleAction
{
public:
    AdminJoin(const Settings& settings, ::handler::HandlerContainer* parent)
        : SimpleAction("join",settings,parent)
    {
        synopsis += " channel...";
        help = "Makes the bot join one or more channels";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::vector<std::string> channels;
        if ( !msg.message.empty() )
            channels = string::comma_split(msg.message);
        else if ( !msg.channels.empty() )
            channels = msg.channels;
        else
            return false;
        msg.destination->command({"JOIN",channels});
        return true;
    }
};

/**
 * \brief Makes the bot part channels (IRC)
 */
class AdminPart: public ::handler::SimpleAction
{
public:
    AdminPart(const Settings& settings, ::handler::HandlerContainer* parent)
        : SimpleAction("part",settings,parent)
    {
        synopsis += " [channel]";
        help = "Makes the bot part a channel";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::regex regex_part ( "(\\S+)\\s*(.*)",
            std::regex::ECMAScript | std::regex::optimize );
        std::smatch match;

        if ( std::regex_match(msg.message,match,regex_part) )
            msg.destination->command({"PART",{match[1], match[2]}});
        else if ( msg.channels.size() == 1 )
            msg.destination->command({"PART",{msg.channels[0]}});
        else
            return false;
        return true;
    }
};


/**
 * \brief Makes the bot join channels (IRC)
 * \note Use this inside a group
 */
class AcceptInvite: public ::handler::Handler
{
public:
    AcceptInvite(const Settings& settings, ::handler::HandlerContainer* parent)
        : Handler(settings,parent)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( msg.command == "INVITE" && msg.params.size() == 2 )
        {
            msg.destination->command({"JOIN",{msg.params[1]}});
            return true;
        }
        return false;
    }
};

/**
 * \brief Makes the bot execute a raw command (IRC)
 */
class AdminRaw: public ::handler::SimpleAction
{
public:
    AdminRaw(const Settings& settings, ::handler::HandlerContainer* parent)
        : SimpleAction("raw",settings,parent)
    {
        synopsis += " command";
        help = "Sends raw IRC commands";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( !msg.message.empty() )
        {
            std::istringstream ss(msg.message);
            network::Command cmd;
            ss >> cmd.command;
            char c;
            std::string param;
            while ( ss >> c )
            {
                if ( c == ':' )
                {
                    std::getline(ss, param);
                    cmd.parameters.push_back(param);
                    break;
                }
                else
                {
                    ss.putback(c);
                    ss >> param;
                    cmd.parameters.push_back(param);
                }
            }
            msg.destination->command(cmd);
        }
        return true;
    }
};

} // namespace handler
} // namespace irc

#endif // IRC_HANDLER_ADMIN