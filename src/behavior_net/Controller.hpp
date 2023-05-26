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

#pragma once

#include <behavior_net/Action.hpp>
#include <behavior_net/PetriNet.hpp>

#include <3rd_party/cpp-httplib/httplib.h>

#include <atomic>
#include <iostream>
#include <memory>

namespace capybot
{
namespace bnet
{

// TODO: create ServerI

struct ControllerCallbacks
{
    std::function<void(nlohmann::json const& contentBlocks, std::string_view placeId)> addToken;
    std::function<nlohmann::json()> getNetMarking;
    std::function<void(std::string_view const& id)> triggerManualTransition;
};

class HttpServer
{

public:
    HttpServer(nlohmann::json const& config, ControllerCallbacks const& controllerCbs)
        : m_controllerCbs(controllerCbs)
        , m_addr(config.at("address").get<std::string>())
        , m_port(config.at("port").get<int>())
    {
        std::cout << config << std::endl;
    }

    ~HttpServer() { stop(); }

    void start()
    {
        m_executionThread = std::thread([this] { runServer(); });
    }

    void stop()
    {
        m_server->stop();
        if (m_executionThread.joinable())
        {
            m_executionThread.join();
        }
    }

private:
    void runServer()
    {
        std::cout << "HttpServer::runServer: starting HTTP server..." << std::endl;
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
        std::cout << "HttpServer::runServer: exiting..." << std::endl;
    }

    void setCallbacks(httplib::Server& server);

    httplib::Server* m_server{nullptr};
    ControllerCallbacks m_controllerCbs;

    std::string m_addr;
    int m_port;

    std::thread m_executionThread;
};

class Controller
{
public:
    Controller(NetConfig const& config, std::unique_ptr<PetriNet> petriNet)
        : m_tp(config.get().at("controller").at("thread_poll_workers").get<uint32_t>())
        , m_config(config.get().at("controller"))
        , m_net(std::move(petriNet))
        , m_httpServer(config.get().at("controller").at("http_server"), createCallbacks())
    {
        Place::Factory::createActions(m_tp, config.get().at("controller").at("actions"), m_net->getPlaces());
    }

    ~Controller() { stop(); }

    void addToken(nlohmann::json const& contentBlocks, std::string_view placeId)
    {
        auto token = Token::makeUnique();
        for (nlohmann::json::const_iterator it = contentBlocks.begin(); it != contentBlocks.end(); ++it)
        {
            token->addContentBlock(it.key(), it.value());
        }
        m_net->addToken(token, placeId);

        m_net->prettyPrintState();
    }

    void run()
    {
        std::cout << "Controller::run: running... " << std::endl;
        m_running.store(true);
        m_httpServer.start();
        while (m_running.load())
        {
            m_net->prettyPrintState();
            runEpoch();
        }
        std::cout << "Controller::run: done running." << std::endl;
    }

    void runDetached()
    {
        // TODO: assert joinable
        m_runDetachedThread = std::thread([this] { run(); });
    }

    void stop()
    {
        m_running.store(false);
        m_httpServer.stop();
        if (m_runDetachedThread.joinable())
        {
            m_runDetachedThread.join();
        }
    }

    void runEpoch()
    {
        // log::timePoint("runEpoch start...");
        const uint32_t periodMs = m_config.at("epoch_period_ms").get<uint32_t>();

        // execute all actions
        for (auto&& [_, place] : m_net->getPlaces())
        {
            place->executeActionAsync();
        }

        // wait
        std::this_thread::sleep_for(std::chrono::milliseconds(periodMs)); // TODO: sleep until

        // wait for tasks to complete
        for (auto&& [_, place] : m_net->getPlaces())
        {
            place->checkActionResults();
        }

        // fire all enabled auto transitions
        for (auto&& t : m_net->getTransitions())
        {
            if (t.isManual())
                continue;

            // current logic is to trigger a transition only once per epoch
            // TODO: this should be configurable as this logic does not fulfill all use cases
            if (t.isEnabled())
            {
                t.trigger();
            }
        }

        // log::timePoint("runEpoch ... done");
        // m_net->prettyPrintState();
    }

    PetriNet const& getNet() const { return *m_net; }
    PetriNet& getNet() { return *m_net; }

private:
    ControllerCallbacks createCallbacks()
    {
        return ControllerCallbacks{
            .addToken = [this](nlohmann::json const& contentBlocks,
                               std::string_view placeId) { addToken(contentBlocks, placeId); },
            .getNetMarking = [this]() -> nlohmann::json { return getNet().getMarking(); },
            .triggerManualTransition = [this](std::string_view const& id) { getNet().triggerTransition(id, true); }};
    }

    ThreadPool m_tp;
    nlohmann::json const& m_config;

    std::atomic_bool m_running{false}; // TODO: error handling on logic
    std::thread m_runDetachedThread;

    std::unique_ptr<PetriNet> m_net;

    HttpServer m_httpServer;
};

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
