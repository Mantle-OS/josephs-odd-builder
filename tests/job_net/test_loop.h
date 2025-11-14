#pragma once

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <filesystem>

#include <job_logger.h>

#include <job_io_async_thread.h>

///////////////////
// NET
///////////////////

// UTILS
#include <job_ipaddr.h>
#include <job_url.h>
#include <socket_error.h>
// Sockets
#include <isocket_io.h>
#include <tcp_socket.h>
#include <udp_socket.h>
#include <unix_socket.h>
// TCP client/server
#include <clients/tcp_client.h>
#include <servers/tcp_server.h>
// udp client/server
#include <clients/udp_client.h>
#include <servers/udp_server.h>
// unix client/server
#include <clients/unix_socket_client.h>
#include <servers/unix_socket_server.h>
///////////////////
// END NET
///////////////////

using namespace job::net;
using namespace job::threads;
using namespace std::chrono_literals;

struct TestLoop {
    std::shared_ptr<JobIoAsyncThread> loop;
    TestLoop()
    {
        job::core::JobLogger::instance().setLevel(job::core::LogLevel::Info);
        loop = std::make_shared<JobIoAsyncThread>();
        loop->start();
        REQUIRE(loop->isRunning());
    }

    ~TestLoop()
    {
        if(loop->isRunning())
            loop->stop();
        REQUIRE_FALSE(loop->isRunning());
    }
};

inline static std::string make_temp_sock_path(const std::string &base)
{
    std::string path = "/tmp/" + base + "_" + std::to_string(::getpid()) + ".sock";
    if (std::filesystem::exists(path))
        std::filesystem::remove(path);
    return path;
}
// CHECKPOINT: v1.1
