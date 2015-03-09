/**
 * \file
 * \brief This file contains handlers for WHOIS recognition on IRC
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

#include "handler.hpp"
#include "network/irc_connection.hpp"

namespace handler {

/**
 * \brief Sets the global id based on a 330 reply from whois (IRC)
 */
class Whois330 : public Handler
{
public:
    Whois330(const Settings& settings, Melanobot* bot)
        : Handler(settings,bot) {}

    bool can_handle(const network::Message& msg) override
    {
        return msg.command == "330" && msg.source && msg.params.size() > 2;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        msg.source->update_user(msg.params[1],{{"global_id",msg.params[2]}});
        return true;
    }

};
REGISTER_HANDLER(Whois330,Whois330);

/**
 * \brief Asks Q for WHOIS or USERS information when a user joins a channel (IRC)
 * \note This will only work if the bot has a Q account, and USERS requires +k
 *       or better on the channel.
 */
class QSendWhois : public Handler
{
public:
    QSendWhois(const Settings& settings, Melanobot* bot)
        : Handler(settings,bot)
    {
        q_bot = settings.get("q_to",q_bot);
    }

    bool can_handle(const network::Message& msg) override
    {
        return msg.command == "JOIN" && msg.params.size() == 1 &&
            msg.source && msg.source->protocol() == "irc";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        network::irc::IrcConnection * source
            = reinterpret_cast<network::irc::IrcConnection*>(msg.source);

        if ( source->name() == msg.from )
        {
            source->command({"PRIVMSG",{q_bot,"users "+msg.channels[0]},priority});
        }
        else
        {
            user::User user = source->get_user(msg.from);
            if ( user.global_id.empty() )
                source->command({"PRIVMSG",{q_bot,"whois "+msg.from},priority});
        }

        return false; // reacts to the message but allows to do futher processing
    }

private:
    std::string q_bot = "Q@CServe.quakenet.org";
};
REGISTER_HANDLER(QSendWhois,QSendWhois);

/**
 * \brief Parses responses from Q WHOIS and USER (IRC)
 */
class QGetWhois : public Handler
{
public:
    QGetWhois(const Settings& settings, Melanobot* bot)
        : Handler(settings,bot)
    {
        q_bot = settings.get("q_from",q_bot);
    }

    bool can_handle(const network::Message& msg) override
    {
        return msg.command == "NOTICE" && msg.from == q_bot &&
            msg.source && msg.source->protocol() == "irc" &&
            msg.params.size() == 2;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::regex regex_qwhois (
            "-Information for user (\\S+) \\(using account ([^)]+)\\):",
            std::regex_constants::syntax_option_type::optimize |
            std::regex_constants::syntax_option_type::ECMAScript );

        static std::regex regex_startusers (
            "Users currently on #[^]+:",
            std::regex_constants::syntax_option_type::optimize |
            std::regex_constants::syntax_option_type::ECMAScript );

        static std::regex regex_users (
            R"([ @+](\S+)\s+(\S+)\s+(?:\+[a-z]+)?\s+\([^@]+@[^)]+\))",
            std::regex_constants::syntax_option_type::optimize |
            std::regex_constants::syntax_option_type::ECMAScript );

        std::smatch match;
        if ( std::regex_match(msg.params[1],match,regex_qwhois) || (
            expects_users && std::regex_match(msg.params[1],match,regex_users) ))
        {
            msg.source->update_user(match[1],{{"global_id",match[2]}});
            return true;
        }
        else if ( std::regex_match(msg.params[1],regex_startusers) )
        {
            expects_users = true;
            return true;
        }
        else if ( expects_users && msg.params[1] == "End of list." )
        {
            expects_users = false;
            return true;
        }

        return false;
    }

private:
    std::string q_bot = "Q";
    bool expects_users = false; ///< Whether it's parsing the output of USERS
};
REGISTER_HANDLER(QGetWhois,QGetWhois);

/**
 * \brief Sends a WHOIS about the message sender
 */
class WhoisCheckMe : public SimpleAction
{
public:
    WhoisCheckMe(const Settings& settings, Melanobot* bot)
        : SimpleAction("checkme",settings,bot)
    {}

    bool can_handle(const network::Message& msg) override
    {
        return SimpleAction::can_handle(msg) &&
            msg.source && msg.source->protocol() == "irc";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        msg.source->command({"WHOIS",{msg.from},priority});
        return true;
    }

private:
    std::string sources_url;
};
REGISTER_HANDLER(WhoisCheckMe,WhoisCheckMe);


} // namespace handler
