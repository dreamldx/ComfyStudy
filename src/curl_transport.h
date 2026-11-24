#pragma once

#include <memory>

#include "http_client.h"
#include "transport.h"

namespace http {

// The Transport that talks to real servers with libcurl. One extra thread runs libcurl's multi
// interface for all requests; a coroutine that waits for its answer does not occupy a thread. Safe
// to use from several threads. The options (timeouts) apply to every request.
std::unique_ptr<Transport> MakeCurlTransport(Options options = {});

}  // namespace http
