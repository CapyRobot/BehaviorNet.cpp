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

#include "behavior_net/Action.hpp"
#include "behavior_net/PetriNet.hpp"

#include "3rd_party/cpp-httplib/httplib.h"

#include <iostream>
#include <memory>

namespace capybot
{
namespace bnet
{

// TODO:
//     - server should not depend on Controller, create a separate interface
//     - create ServerI

class Controller;

class HttpServer
{

public:
    HttpServer(nlohmann::json const& config, Controller* controllerPtr)
        : m_controller(controllerPtr)
        , m_addr(config.at("address").get<std::string>())
        , m_port(config.at("port").get<int>())
    {
        std::cout << config << std::endl;
    }

    void start()
    {
        m_executionThread = std::thread([this] { runServer(); });
    }
    void stop()
    {
        m_server->stop();
        m_executionThread.join();
    }

private:
    void runServer()
    {
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
    }

    void setCallbacks(httplib::Server& server);

    httplib::Server* m_server{nullptr};
    Controller* m_controller;

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
        , m_httpServer(config.get().at("controller").at("http_server"), this)
    {
        Place::Factory::createActions(m_tp, config.get().at("controller").at("actions"), m_net->getPlaces());
    }

    ~Controller() { stop(); }

    void addToken(nlohmann::json const& contentBlocks, std::string_view placeId)
    {
        auto token = Token::makeUnique();
        for (auto&& block : contentBlocks)
        {
            token->addContentBlock(block.at("key"), block.at("content"));
        }
        m_net->addToken(token, placeId);

        m_net->prettyPrintState();
    }

    void run()
    {
        m_running.store(true);
        m_httpServer.start();
        while (m_running.load())
        {
            runEpoch();
        }
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
    ThreadPool m_tp;
    nlohmann::json const& m_config;

    std::atomic<bool> m_running{false}; // TODO: error handling on logic
    std::thread m_runDetachedThread;

    std::unique_ptr<PetriNet> m_net;

    HttpServer m_httpServer;
};

void HttpServer::setCallbacks(httplib::Server& server)
{
    server.Post("/add_token", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json payload = nlohmann::json::parse(req.body);
        m_controller->addToken(payload.at("content_blocks"), payload.at("place_id").get<std::string>());
        res.set_content("success!", "application/json");
    });
    server.Get("/get_marking", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json marking = m_controller->getNet().getMarking();
        res.set_content(marking.dump(), "application/json");
    });
    server.Post("/trigger_manual_transition/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        auto id = req.matches[1];
        m_controller->getNet().triggerTransition(id.str(), true);
    });
}

} // namespace bnet
} // namespace capybot
