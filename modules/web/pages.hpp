/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
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
#ifndef MELANOBOT_MODULE_WEB_PAGES_HPP
#define MELANOBOT_MODULE_WEB_PAGES_HPP

#include <boost/filesystem.hpp>
#include "server.hpp"
#include "melanobot/melanobot.hpp"

namespace web {


/**
 * \brief Web page handler rendering files in a directory on disk
 */
class RenderStatic : public WebPage
{
public:
    explicit RenderStatic(const Settings& settings)
    {
        directory = settings.get("directory", "");
        if ( directory.empty() || !boost::filesystem::is_directory(directory) )
            throw melanobot::ConfigurationError("Invalid path: " + directory.string());

        uri = read_uri(settings, "static");

        default_mime_type = settings.get("default_mime_type", default_mime_type.string());

        for ( const auto& mimes : settings.get_child("Mime", {}) )
            extension_to_mime[mimes.first] = mimes.second.data();

        /// \todo blacklist patterns
    }

    bool matches(const Request& request, const PathSuffix& path) const override
    {
        return path.match_prefix(uri) && boost::filesystem::is_regular(full_path(path));
    }

    Response respond(Request& request, const PathSuffix& path, const HttpServer& sv) const override
    {
        auto file_path = full_path(path);
        std::ifstream input(file_path.string());
        if ( !input.is_open() )
            throw HttpError(StatusCode::NotFound);
        Response response(mime(file_path), StatusCode::OK, request.protocol);
        std::array<char, 1024> chunk;
        while ( input.good() )
        {
            input.read(chunk.data(), chunk.size());
            response.body.write(chunk.data(), input.gcount());
        }

        return response;
    }

protected:
    const MimeType& mime(const boost::filesystem::path& path) const
    {
        auto it = extension_to_mime.find(path.extension().string());
        if ( it != extension_to_mime.end() )
            return it->second;
        return default_mime_type;
    }

    boost::filesystem::path full_path(const PathSuffix& path) const
    {
        auto file_path = directory;
        for ( const auto& dir : path.left_stripped(uri.size()) )
            file_path /= dir;
        return file_path;
    }

private:
    boost::filesystem::path directory;
    httpony::Path uri;
    std::unordered_map<std::string, MimeType> extension_to_mime;
    MimeType default_mime_type = "application/octet-stream";
};

/**
 * \brief Renders a fixed file
 */
class RenderFile : public WebPage
{
public:
    explicit RenderFile(const Settings& settings)
    {
        file_path = settings.get("path", file_path);
        uri = read_uri(settings);

        mime_type = settings.get("mime_type", mime_type.string());
    }

    bool matches(const Request& request, const PathSuffix& path) const override
    {
        return path.match_exactly(uri);
    }

    Response respond(Request& request, const PathSuffix& path, const HttpServer& sv) const override
    {
        std::ifstream input(file_path);
        if ( !input.is_open() )
            throw HttpError(StatusCode::InternalServerError);
        Response response(mime_type, StatusCode::OK, request.protocol);
        std::array<char, 1024> chunk;
        while ( input.good() )
        {
            input.read(chunk.data(), chunk.size());
            response.body.write(chunk.data(), input.gcount());
        }

        return response;
    }

private:
    std::string file_path;
    httpony::Path uri;
    MimeType mime_type = "application/octet-stream";
};

/**
 * \brief Groups pages under a common prefix
 */
class PageDirectory : public HttpRequestHandler, public WebPage
{
public:
    explicit PageDirectory(const Settings& settings)
    {
        uri = read_uri(settings);
        load_pages(settings);
    }

    bool matches(const Request& request, const PathSuffix& path) const override
    {
        return path.match_prefix(uri);
    }

    Response respond(Request& request, const PathSuffix& path, const HttpServer& sv) const override
    {
        return HttpRequestHandler::respond(
            request, StatusCode::OK, path.left_stripped(uri.size()), sv
        );
    }

private:
    httpony::Path uri;
};

/**
 * \brief Renders an error page using HTML
 */
class HtmlErrorPage : public ErrorPage
{
public:
    explicit HtmlErrorPage(const Settings& settings)
    {
        css_file = settings.get("css", css_file);
        extra_info = settings.get("css", css_file);
    }

    Response respond(const Status& status, Request& request, const HttpServer& sv) const
    {
        using namespace httpony::quick_xml;
        using namespace httpony::quick_xml::html;
        Response response("text/html", status, request.protocol);

        HtmlDocument document("Error " + melanolib::string::to_string(status.code));
        if ( !css_file.empty() )
        {
            document.head().append(Element("link", Attributes{
                {"rel", "stylesheet"}, {"type", "text/css"}, {"href", css_file}
            }));
        }

        document.body().append(Element("h1", Text(status.message)));

        std::string reply = "The URL " + request.uri.path.url_encoded(true) + " ";
        if ( status == StatusCode::NotFound )
            reply += "was not found.";
        else if ( status.type() == httpony::StatusType::ClientError )
            reply += "has not been accessed correctly.";
        else if ( status.type() == httpony::StatusType::ServerError )
            reply += "caused a server error.";
        else
            reply += "caused an unknown error";
        document.body().append(Element("p", Text(reply)));

        if ( !extra_info.empty() )
        {
            document.body().append(Element("p", Text(
                sv.format_info(extra_info, request, response)
            )));
        }

        response.body << document << '\n';
        return response;
    }

private:
    std::string css_file;
    std::string extra_info;
};



/**
 * \brief Web page showing an overview of the bot status
 * \todo Require SSL client certs (as part of some other page wrapper)
 */
class StatusPage : public WebPage
{
public:
    explicit StatusPage(const Settings& settings)
    {
        uri = read_uri(settings, "");
    }

    bool matches(const Request& request, const PathSuffix& path) const override
    {
        return path.match_prefix(uri);
    }

    Response respond(Request& request, const PathSuffix& path, const HttpServer& sv) const override
    {
        auto local_path = path.left_stripped(uri.size());
        httpony::quick_xml::html::HtmlDocument html("Bot status");

        if ( local_path.empty() )
            home(html.body());
        else if ( local_path.match_exactly("connection") )
            connection_list(html.body());
        else if ( local_path.match_exactly("service") )
            service_list(html.body());
        else if ( local_path.match_prefix("connection") && local_path.size() == 2 )
            connection(local_path[1], html.body());
        else if ( local_path.match_prefix("service") && local_path.size() == 2 )
            service(local_path[1], html.body());
        else
            throw HttpError(StatusCode::NotFound);
        Response response("text/html", StatusCode::OK, request.protocol);
        html.print(response.body, true);
        return response;
    }

private:
    using BlockElement = httpony::quick_xml::BlockElement;

    static melanobot::Melanobot& bot()
    {
        return melanobot::Melanobot::instance();
    }

    void home(BlockElement& parent) const
    {
        connection_list(parent);
        service_list(parent);
    }

    void connection_list(BlockElement& parent) const
    {
        using namespace httpony::quick_xml;
        using namespace httpony::quick_xml::html;

        parent.append(Element{"h1", Text{"Connections"}});

        List connections;
        for ( const auto& conn : bot().connection_names() )
            connections.add_item(Link("./connection/"+conn, conn));
        parent.append(connections);
    }

    void service_list(BlockElement& parent) const
    {
        using namespace httpony::quick_xml;
        using namespace httpony::quick_xml::html;

        parent.append(Element{"h1", Text{"Services"}});

        List services;
        for ( const auto& svc : bot().service_list() )
            services.add_item(Link("./service/" + std::to_string(uintptr_t(svc.get())), svc->name()));
        parent.append(services);
    }

    void connection(const std::string& name, BlockElement& parent) const
    {
        using namespace httpony::quick_xml;
        using namespace httpony::quick_xml::html;

        network::Connection* conn = bot().connection(name);
        if ( !conn )
            throw HttpError(StatusCode::NotFound);

        parent.append(Element{"h1", Text{name}});

        Table table;
        table.add_header_row(Text{"Property"}, Text{"Value"});
        table.add_data_row(Text("Protocol"), Text(conn->protocol()));
        table.add_data_row(Text("Status"), status(conn->status()));
        table.add_data_row(Text("Name"), Text(conn->name()));
        table.add_data_row(Text("Config Name"), Text(conn->config_name()));
        table.add_data_row(Text("Formatter"), Text(conn->formatter()->name()));
        table.add_data_row(Text("Server"), Text(conn->server().name()));

//         for ( const auto& prop : conn->properties())
//             table.add_data_row(Text(prop.first), Text(prop.second));

        /// \todo FormatterHtml
        string::FormatterUtf8 formatter;
        for ( const auto& prop : conn->pretty_properties())
            table.add_data_row(Text(prop.first), Text(prop.second.encode(formatter)));

        parent.append(table);
    }

    void service(const std::string& id, BlockElement& parent) const
    {
        using namespace httpony::quick_xml;
        using namespace httpony::quick_xml::html;

        AsyncService* service = nullptr;
        for ( const auto& svc : bot().service_list() )
            if ( std::to_string(uintptr_t(svc.get())) == id )
            {
                service = svc.get();
                break;
            }
        if ( !service )
            throw HttpError(StatusCode::NotFound);

        parent.append(Element{"h1", Text{service->name()}});

        Table table;
        table.add_header_row(Text{"Property"}, Text{"Value"});
        table.add_data_row(Text("Status"), status(
            service->running() ?
                network::Connection::CONNECTED :
                network::Connection::DISCONNECTED
        ));
        table.add_data_row(Text("Name"), Text(service->name()));

        parent.append(table);
    }

    httpony::quick_xml::BlockElement status(network::Connection::Status status) const
    {
        using namespace httpony::quick_xml;

        std::string status_name = "Disconnected";
        if ( status > network::Connection::CHECKING )
            status_name = "Connected";
        else if ( status >= network::Connection::CONNECTING )
            status_name = "Connecting";

        return BlockElement{
            "span",
            Attribute("class", "status_" + status_name),
            Text(status_name)
        };
    }

private:
    httpony::Path uri;
};

} // namespace web
#endif // MELANOBOT_MODULE_WEB_PAGES_HPP