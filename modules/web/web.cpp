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
#include "handler/web-api-concrete.hpp"
#include "client/http.hpp"
#include "server/pages.hpp"
#include "server/push_pages.hpp"
#include "module/melanomodule.hpp"
#include "string/replacements.hpp"

/**
 * \brief Initializes the web module
 */
MELANOMODULE_ENTRY_POINT module::Melanomodule melanomodule_web_metadata()
{
    return {"web", "Web services"};
}

MELANOMODULE_ENTRY_POINT void melanomodule_web_initialize(const Settings&)
{
    module::register_log_type("web", color::dark_blue);
    module::register_log_type("wsv", color::dark_blue);
    module::register_service<web::HttpClient>("http");
    module::register_instantiable_service<web::HttpServer>("HttpServer");

    module::register_handler<web::SearchVideoYoutube>("SearchVideoYoutube");
    module::register_handler<web::UrbanDictionary>("UrbanDictionary");
    module::register_handler<web::SearchWebSearx>("SearchWebSearx");
    module::register_handler<web::VideoInfo>("VideoInfo");
    module::register_handler<web::MediaWiki>("MediaWiki");
    module::register_handler<web::MediaWikiTitles>("MediaWikiTitles");
    module::register_handler<web::MediaWikiCategoryTitle>("MediaWikiCategoryTitle");
    module::register_handler<web::WhereIsGoogle>("WhereIsGoogle");
    module::register_handler<web::RandomReddit>("RandomReddit");
    module::register_handler<web::OpenWeather>("OpenWeather");

    web::PageRegistry::instance().register_page<web::RenderStatic>("RenderStatic");
    web::PageRegistry::instance().register_page<web::PageDirectory>("Directory");
    web::PageRegistry::instance().register_page<web::RenderFile>("RenderFile");
    web::PageRegistry::instance().register_page<web::HtmlErrorPage>("HtmlErrorPage");
    web::PageRegistry::instance().register_page<web::StatusPage>("StatusPage");
    web::PageRegistry::instance().register_page<web::Redirect>("Redirect");
    web::PageRegistry::instance().register_page<web::PushPage>("PushPage");

    string::FilterRegistry::instance().register_filter("urlencode",
        [](const std::vector<string::FormattedString>& args) -> string::FormattedString
        {
            if ( args.empty() )
                return {};
            string::FormatterUtf8 utf8;
            return utf8.decode(httpony::urlencode(args[0].encode(utf8)));
        }
    );
    string::FilterRegistry::instance().register_filter("urldecode",
        [](const std::vector<string::FormattedString>& args) -> string::FormattedString
        {
            if ( args.empty() )
                return {};
            string::FormatterUtf8 utf8;
            return utf8.decode(httpony::urldecode(args[0].encode(utf8)));
        }
    );
}
