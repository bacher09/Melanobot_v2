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
#ifndef XONOTIC_CONNECTION_HPP
#define XONOTIC_CONNECTION_HPP

#include "network/connection.hpp"
#include "concurrency/container.hpp"
#include "user/user_manager.hpp"
#include "xonotic.hpp"
#include "xonotic/darkplaces.hpp"

namespace xonotic {

/**
 * \brief Creates a connection to a Xonotic server
 */
class XonoticConnection : public network::Connection, protected Darkplaces
{
public:
    /**
     * \brief Create from settings
     */
    static std::unique_ptr<XonoticConnection> create(
        const Settings& settings, const std::string& name);

    /**
     * \thread main \lock none
     */
    XonoticConnection ( const network::Server&  server,
                        const std::string&      password,
                        const std::string&      log_dest_ip,
                        Darkplaces::Secure      secure = Darkplaces::Secure::NO,
                        const Settings&         settings = {},
                        const std::string&      name = {} );

    /**
     * \thread main \lock none
     */
    ~XonoticConnection() override
    {
        stop();
    }

    /**
     * \thread external \lock none (reading a constant value)
     * \todo return io.local_endpoint() ?
     */
    network::Server server() const override
    {
        return Darkplaces::server();
    }

    /**
     * \thread external \lock none (reading a constant value)
     */
    std::string description() const override
    {
        return Darkplaces::server().name();
    }

    network::Server log_dest_endpoint() const
    {
        if (log_dest_ip.empty()) {
            return local_endpoint();
        } else {
            return network::Server(log_dest_ip, local_endpoint().port);
        }
    }

    /**
     * \thread any \lock none
     */
    void command(network::Command cmd) override;

    /**
     * \thread external \lock none
     */
    void say(const network::OutputMessage& message) override;

    /**
     * \thread external \lock none
     */
    Status status() const override
    {
        return status_;
    }

    /**
     * \thread external \lock none
     */
    std::string protocol() const override
    {
        return "xonotic";
    }

    /**
     * \thread external \lock none(todo: data)
     */
    void connect() override;

    /**
     * \thread external \lock none(todo: data)
     */
    void disconnect(const string::FormattedString& message = {}) override;

    /**
     * \thread external \lock none(todo: data)
     */
    void reconnect(const string::FormattedString& quit_message = {}) override
    {
        disconnect(quit_message);
        connect();
    }

    /**
     * \thread external \lock none
     */
    string::Formatter* formatter() const override
    {
        return formatter_;
    }

    void update_user(const std::string& local_id,
                     const Properties& properties) override;

    void update_user(const std::string& local_id,
                     const user::User& updated) override;

    user::User get_user(const std::string& local_id) const override;

    std::vector<user::User> get_users(const std::string& channel_mask = "") const override;

    std::string name() const override;
    /**
     * \thread external \lock data
     */
    LockedProperties properties() override;
    /**
     * \thread external \lock data
     */
    string::FormattedProperties pretty_properties() const override;

    /**
     * \brief Adds a command that needs to be sent regularly to the server
     * \param command    Command to be sent
     * \param continuous If true send every each \c status_delay, otherwise only at match start
     */
    void add_polling_command(const network::Command& command, bool continuous=false);

    /**
     * \thead external \lock data
     */
    user::UserCounter count_users(const std::string& channel = {}) const override;

    // Dummy methods:
    bool channel_mask(const std::vector<std::string>&, const std::string& ) const override
    {
        return false;
    }
    bool user_auth(const std::string&, const std::string& auth_group) const override
    {
        return auth_group.empty();
    }
    bool add_to_group(const std::string&, const std::string&) override
    {
        return false;
    }
    bool remove_from_group(const std::string&, const std::string&) override
    {
        return false;
    }
    std::vector<user::User> users_in_group(const std::string&) const override
    {
        return {};
    }
    std::vector<user::User> real_users_in_group(const std::string& group) const override
    {
        return {};
    }

protected:
    void on_connect() override;
    void on_network_error(const std::string& message) override;
    void on_network_input(const std::string&) override;
    void on_receive(const std::string& command, const std::string& message) override;
    void on_receive_log(const std::string& line) override;

private:

    string::Formatter*  formatter_{nullptr};            ///< String formatter

    std::string         cmd_say;                        ///< Command used to say messages
    std::string         cmd_say_as;                     ///< Command used to say messages as another user
    std::string         cmd_say_action;                 ///< Command used to show actions
    std::string         log_dest_ip;                    /// override local ip address
    std::string         remote_server_name;             /// overriden server name
    PropertyTree        properties_;                     ///< Misc properties (eg: map, gametype)

    AtomicStatus        status_{DISCONNECTED};          ///< Connection status

    user::UserManager   user_manager;                   ///< Keeps track of players

    mutable std::mutex  mutex;          ///< Guard data races
    network::Timer      status_polling; ///< Timer used to gether the connection status
    std::vector<network::Command> polling_match; ///< Commands to send on match start
    std::vector<network::Command> polling_status; ///< Commands to send on status_polling

    /**
     * \brief Interprets a message and sends it to the bot
     * \thread xon_input \lock data
     */
    void handle_message(network::Message& msg);

    /**
     * \brief Sends commands needed to keep the connection going
     * \thread any \lock data
     */
    void update_connection();

    /**
     * \brief Cleanup what update_connection() does
     * \thread external (disconnect) \lock data
     */
    void cleanup_connection();

    /**
     * \brief Sends the commands needed to determine the connection status
     */
    void request_status();

    /**
     * \brief Closes the connection, allowing it to be re-opened automatically
     */
    void close_connection();

    /**
     * \brief Clear match info (players. map etc)
     * \note The caller must have a lock on \c mutex
     */
    void clear_match();

    /**
     * \brief Initializes user checking
     */
    void check_user_start();
    /**
     * \brief Checks a player
     */
    void check_user(const std::smatch& match);
    /**
     * \brief Finalizes user checking
     */
    void check_user_end();

};

} // namespace xonotic
#endif // XONOTIC_CONNECTION_HPP
