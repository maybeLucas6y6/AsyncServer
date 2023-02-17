#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
constexpr auto asio::experimental::as_tuple(asio::use_awaitable) = asio::experimental::as_tuple(asio::use_awaitable);

#include "Server.hpp"
#include "MutexQueue.hpp"
#include "OwnedMessage.hpp"