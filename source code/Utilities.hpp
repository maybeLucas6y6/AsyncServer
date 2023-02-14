#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <memory>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);