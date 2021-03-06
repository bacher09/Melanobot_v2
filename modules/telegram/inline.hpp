/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2018 Mattia Basaglia
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
#ifndef MELANOBOT_MODULES_TELEGRAM_INLINE_HPP
#define MELANOBOT_MODULES_TELEGRAM_INLINE_HPP

#include "melanobot/handler.hpp"
#include "telegram-connection.hpp"
#include "web/handler/web-api.hpp"
#include "httpony/formats/json.hpp"

namespace telegram {

class PropertyBuilder : public httpony::json::JsonNode
{
public:
    using JsonNode::JsonNode;

    void maybe_put(const std::string& name, const std::string& value)
    {
        if ( !value.empty() )
            put(name, value);
    }

    void maybe_put(const std::string& name, int value, int min = 0)
    {
        if ( value >= min )
            put(name, value);
    }

};

class InlineQueryResult
{
public:
    virtual ~InlineQueryResult(){}
    virtual PropertyBuilder to_properties() const = 0;
};

class InlineQueryResponse
{
public:
    using pointer = std::unique_ptr<InlineQueryResult>;

    explicit InlineQueryResponse(
            std::string inline_query_id,
            int cache_time = -1,
            bool is_personal = false,
            std::string next_offset = {},
            std::string switch_pm_text = {},
            std::string switch_pm_parameter = {}
    ) : inline_query_id(std::move(inline_query_id)),
        cache_time(cache_time),
        is_personal(is_personal),
        next_offset(std::move(next_offset)),
        switch_pm_text(std::move(switch_pm_text)),
        switch_pm_parameter(std::move(switch_pm_parameter))
    {
    }


    InlineQueryResponse& result(pointer result)
    {
        if ( results.size() >= 50 )
            throw melanobot::MelanobotError("Too many inline results");
        results.push_back(std::move(result));
        return *this;
    }

    template<class ResultT, class... Args>
        InlineQueryResponse& result(Args&&... args)
    {
        return result(std::make_unique<ResultT>(std::forward<Args>(args)...));
    }

    PropertyBuilder to_properties() const
    {
        PropertyBuilder ptree;
        httpony::json::JsonNode treeresults;
        treeresults.to_array();
        int i = 0;
        for ( const auto& result : results )
        {
            treeresults.push_back({"", result->to_properties()});
            treeresults.back().second.put("id", inline_query_id + "-" + std::to_string(i));
            i++;
        }
        ptree.put_child("results", treeresults);
        ptree.put("inline_query_id", inline_query_id);
        ptree.maybe_put("cache_time", cache_time);
        ptree.put("is_personal", is_personal);
        ptree.maybe_put("next_offset", next_offset);
        ptree.maybe_put("switch_pm_text", switch_pm_text);
        ptree.maybe_put("switch_pm_parameter", switch_pm_parameter);

        return ptree;
    }

private:
    std::vector<pointer> results;
    std::string inline_query_id;
    int cache_time;
    bool is_personal;
    std::string next_offset;
    std::string switch_pm_text;
    std::string switch_pm_parameter;
};

/**
 * \brief Just to allow simpler initialization and attribute access
 *        while maintaining a virtual destructor
 */
template<class DataT>
    class SimpleDataInlineQueryResult : public InlineQueryResult
{
public:
    template<class... Args>
    SimpleDataInlineQueryResult(Args&&... args)
        : data({std::forward<Args>(args)...})
    {}

    PropertyBuilder to_properties() const override
    {
        return data.to_properties();
    }

    DataT data;
};

struct PhotoData
{
    std::string photo_url;
    std::string thumb_url;
    int photo_width = 512;
    int photo_height = 512;
    std::string title;
    std::string description;
    std::string parse_mode;
    // reply_markup
    // input_message_content

    PropertyBuilder to_properties() const
    {
        PropertyBuilder ptree;
        ptree.put("photo_url", photo_url);
        ptree.put("type", "photo");
        ptree.put("thumb_url", thumb_url.empty() ? photo_url : thumb_url);
        ptree.maybe_put("photo_width", photo_width, 1);
        ptree.maybe_put("photo_height", photo_height, 1);
        ptree.maybe_put("title", title);
        ptree.maybe_put("description", description);
        ptree.maybe_put("parse_mode", parse_mode);
        return ptree;
    }
};

struct ArticleData
{
    std::string title;
    // input_message_content
    // reply_markup
    std::string url;
    bool hide_url = false;
    std::string description;
    std::string thumb_url;
    int thumb_width = 0;
    int thumb_height = 0;

    PropertyBuilder to_properties() const
    {
        PropertyBuilder ptree;
        ptree.put("type", "article");
        ptree.put("title", title);
        ptree.maybe_put("url", url);
        ptree.put("hide_url", hide_url);
        ptree.maybe_put("description", description);
        ptree.maybe_put("thumb_url", thumb_url);
        ptree.maybe_put("thumb_width", thumb_width);
        ptree.maybe_put("thumb_height", thumb_height);
        return ptree;
    }
};


struct GifData
{
    std::string gif_url;
    std::string thumb_url;
    int gif_width = 0;
    int gif_height = 0;
    int gif_duration = 0;
    std::string title;
    std::string caption;
    std::string parse_mode;
    // reply_markup
    // input_message_content

    PropertyBuilder to_properties() const
    {
        PropertyBuilder ptree;
        ptree.put("gif_url", gif_url);
        ptree.put("type", "gif");
        ptree.maybe_put("gif_width", gif_width, 1);
        ptree.maybe_put("gif_height", gif_height, 1);
        ptree.maybe_put("gif_duration", gif_duration, 1);
        ptree.put("thumb_url", thumb_url.empty() ? gif_url : thumb_url);
        ptree.maybe_put("title", title);
        ptree.maybe_put("caption", caption);
        ptree.maybe_put("parse_mode", parse_mode);
        return ptree;
    }
};

using InlineQueryResultPhoto = SimpleDataInlineQueryResult<PhotoData>;
using InlineQueryResultGif = SimpleDataInlineQueryResult<GifData>;
using InlineQueryResultArticle = SimpleDataInlineQueryResult<ArticleData>;


class DynamicInlineQueryResult : public InlineQueryResult
{
public:
    DynamicInlineQueryResult(PropertyBuilder properties)
        : properties(std::move(properties))
    {}

    PropertyBuilder to_properties() const override
    {
        return properties;
    }

    PropertyBuilder properties;
};


template<class Base=melanobot::Handler>
    class InlineHandler : public Base
{
public:
    template<class...Args>
        InlineHandler(const Settings& settings, Args&&... args)
        : Base(std::forward<Args>(args)...)
    {
        cache_time = settings.get("cache_time", cache_time);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::UNKNOWN &&
            msg.command == "inline_query" &&
            msg.source == msg.destination &&
            msg.source->protocol() == "telegram" &&
            msg.params.size() >= 2
        ;
    }

    bool on_handle(network::Message& msg) override
    {
        auto response = on_handle_query(msg, msg.message, msg.params[1]);
        send_response(msg, response);
        return true;
    }

protected:
    void send_response(const network::Message& msg, const InlineQueryResponse& response) const
    {
        TelegramConnection* connection = dynamic_cast<TelegramConnection*>(msg.source);
        if ( !connection )
            throw melanobot::MelanobotError("Invalid telegram connection");
        connection->post("answerInlineQuery", response.to_properties(), {});
    }

    InlineQueryResponse create_response(const network::Message& msg) const
    {
        return InlineQueryResponse(msg.params[0], cache_time);
    }

    virtual InlineQueryResponse on_handle_query(
        const network::Message& msg,
        const std::string& query,
        const std::string& offset
    ) const
    {
        return create_response(msg);
    }

    int cache_time = -1;
};

class MultiUriBase : public InlineHandler<>
{
protected:
    class UriDescription
    {
    public:
        UriDescription(std::string base, std::string param)
            : base(std::move(base)),
              param(std::move(param)),
              has_query(this->base.find('?') != std::string::npos)
        {}

        std::string full_uri(const std::string& query) const
        {
            return base + (has_query ? '&' : '?') + param + "=" + httpony::urlencode(query);
        }

    private:
        std::string base;
        std::string param;
        bool has_query = false;
    };

public:
    MultiUriBase(const std::string& param_prefix, const Settings& settings, MessageConsumer* parent)
        : InlineHandler(settings, settings, parent)
    {
        auto url = settings.get(param_prefix + "_url", "");
        if ( !url.empty() )
        {
            auto photo_param = settings.get(param_prefix + "_param", "");
            if ( photo_param.empty() )
                throw melanobot::ConfigurationError(
                    "If you specify " + param_prefix +
                    "_url you must specify " + param_prefix + "_param"
                );
            uris.push_back({url, photo_param});
        }

        for ( const auto& uri : settings.get_child(param_prefix + "s", {}) )
        {
            uris.push_back({uri.first, uri.second.data()});
        }

    }

private:
    InlineQueryResponse on_handle_query(
        const network::Message& msg,
        const std::string& query,
        const std::string& offset
    ) const override
    {
        auto resp = create_response(msg);
        for ( const auto& desc : uris )
        {
            add_result(resp, desc, query);
        }
        return resp;
    }

    virtual void add_result(
        InlineQueryResponse& resp,
        const UriDescription& desc,
        const std::string& query
    ) const = 0;

    std::vector<UriDescription> uris;

};

/**
 * \brief Generates picture urls based on the queries
 */
class InlinePhotoUrl : public MultiUriBase
{
public:
    InlinePhotoUrl(const Settings& settings, MessageConsumer* parent)
        : MultiUriBase("photo", settings, parent)
    {}

private:
    void add_result(
        InlineQueryResponse& resp,
        const UriDescription& desc,
        const std::string& query
    ) const override
    {
        resp.result<InlineQueryResultPhoto>(desc.full_uri(query));
    }
};

class InlineGifUrl : public MultiUriBase
{
public:
    InlineGifUrl(const Settings& settings, MessageConsumer* parent)
        : MultiUriBase("gif", settings, parent)
    {}

private:
    void add_result(
        InlineQueryResponse& resp,
        const UriDescription& desc,
        const std::string& query
    ) const override
    {
        resp.result<InlineQueryResultGif>(desc.full_uri(query));
    }
};


class InlineExternalJson : public InlineHandler<web::SimpleJson>
{
public:
    InlineExternalJson(const Settings& settings, MessageConsumer* parent)
        : InlineHandler(settings, "", settings, parent)
    {
        for ( const auto& data : settings.get_child("data_template", {}) )
        {
            data_template.insert({
                data.first,
                string::FormatterConfig().decode(data.second.data())
            });
        }

        uri_base = settings.get("uri_base", uri_base);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::string url = uri_base + msg.message;
        request_json(msg, web::Request("GET", {url}));
        return true;
    }

    void json_failure(const network::Message& msg) override
    {
        send_response(msg, InlineQueryResponse(msg.params[0], 0));
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        const Settings* result_parent = nullptr;
        if ( result_path.empty() )
        {
            result_parent = &parsed;
        }
        else
        {
            if ( auto result = parsed.get_child_optional(result_path) )
                result_parent = &*result;
        }

        auto resp = create_response(msg);
        if ( result_parent )
        {
            for ( const auto& result : *result_parent )
                resp.result<DynamicInlineQueryResult>(format_result(result.second));
        }
        send_response(msg, resp);
    }

private:
    PropertyBuilder format_result(const Settings& result)
    {
        PropertyBuilder out;
        for ( const auto& data : data_template )
        {
            out.put(
                data.first,
                data.second.replaced(result).encode(string::FormatterUtf8())
            );
        }
        return out;
    }

private:
    std::string uri_base;
    string::FormattedProperties data_template;
    std::string result_path;
};

class InlineFixedResponse : public InlineHandler<>
{
public:
    InlineFixedResponse(const Settings& settings, MessageConsumer* parent)
        : InlineHandler(settings, settings, parent)
    {
        for ( const auto& resp : settings )
        {
            if ( resp.first == "response" )
            {
                responses.push_back({});
                string::FormattedProperties& props = responses.back();
                for ( const auto& data : resp.second )
                {
                    props.insert({
                        data.first,
                        string::FormatterConfig().decode(data.second.data())
                    });
                }
            }
        }
    }
protected:
    virtual InlineQueryResponse on_handle_query(
        const network::Message& msg,
        const std::string& query,
        const std::string& offset
    ) const
    {
        auto response = create_response(msg);
        auto properties = msg.source->pretty_properties(msg.from);
        properties["query"] = query;
        for ( const auto& resp_template : responses )
            response.result<DynamicInlineQueryResult>(format_result(resp_template, properties));
        return response;
    }

private:
    PropertyBuilder format_result(
        const string::FormattedProperties& out_template,
        const string::FormattedProperties& replacements
    ) const
    {
        PropertyBuilder out;
        for ( const auto& data : out_template )
        {
            out.put(
                data.first,
                data.second.replaced(replacements).encode(string::FormatterUtf8())
            );
        }
        return out;
    }

    std::vector<string::FormattedProperties> responses;
};

} // namespace telegram

#endif // MELANOBOT_MODULES_TELEGRAM_INLINE_HPP

