/*
 * Copyright (C) 2023 Eduardo Rocha
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <3rd_party/nlohmann/json.hpp>
#include <behavior_net/Config.hpp>
#include <behavior_net/server_impl/HttpServer.hpp>
#include <memory>

namespace capybot
{
namespace bnet
{

bool validateHttpServerConfig(nlohmann::json const& netConfig, std::vector<std::string>& errorMessages)
{
    errorMessages.clear();

    if (!netConfig.contains("controller") || !netConfig.at("controller").contains("http_server"))
    {
        return true; // http server not in config
    }
    auto serverConfig = getValueAtPath<nlohmann::json>(netConfig, {"controller", "http_server"}, errorMessages).value();

    // check expected info exists in expected format
    std::ignore = getValueAtKey<std::string>(serverConfig, "address", errorMessages);
    std::ignore = getValueAtKey<int>(serverConfig, "port", errorMessages);

    return errorMessages.empty();
}

REGISTER_NET_CONFIG_VALIDATOR(&validateHttpServerConfig, "HttpServerConfigValidator");

HttpServer::HttpServer(nlohmann::json const& config, ControllerCallbacks const& controllerCbs)
    : m_controllerCbs(controllerCbs)
    , m_addr(config.at("address").get<std::string>())
    , m_port(config.at("port").get<int>())
{
    LOG(INFO) << "Config: " << config << log::endl;
}

void HttpServer::runServer()
{
    LOG(DEBUG) << "HttpServer::runServer: starting HTTP server..." << log::endl;
    httplib::Server server;
    m_server = &server;

    setCallbacks(server);

    // TODO: proper error handling
    server.set_exception_handler([](const auto& req, auto& res, std::exception_ptr ep) {
        auto fmt = "<h1>Error 500</h1><p>%s</p>";
        char buf[BUFSIZ];
        try
        {
            std::rethrow_exception(ep);
        }
        catch (std::exception& e)
        {
            snprintf(buf, sizeof(buf), fmt, e.what());
        }
        catch (...)
        {
            snprintf(buf, sizeof(buf), fmt, "Unknown Exception");
        }
        res.set_content(buf, "text/html");
        res.status = 500;
    });

    server.set_error_handler([](const auto& req, auto& res) {
        auto fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, res.status);
        res.set_content(buf, "text/html");
    });

    server.listen(m_addr, m_port);
    LOG(DEBUG) << "HttpServer::runServer: exiting..." << log::endl;
}

void HttpServer::setCallbacks(httplib::Server& server)
{
    server.Post("/add_token", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json payload = nlohmann::json::parse(req.body);
        m_controllerCbs.addToken(payload.at("content_blocks"), payload.at("place_id").get<std::string>());
        res.set_content("success!", "application/json");
    });
    server.Get("/get_marking", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json marking = m_controllerCbs.getNetMarking();
        res.set_content(marking.dump(), "application/json");
    });
    server.Post("/trigger_manual_transition/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        auto id = req.matches[1];
        m_controllerCbs.triggerManualTransition(id.str());
    });
}

} // namespace bnet
} // namespace capybot
