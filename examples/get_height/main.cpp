/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin-client.
 *
 * libbitcoin-client is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <czmq++/czmqpp.hpp>
#include <bitcoin/client.hpp>

using namespace bc;

/**
 * A minimalist example that connects to a server,
 * fetches the current height, and exits.
 */
int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <server>" << std::endl;
        return 1;
    }

    // Set up the server connection:
    czmqpp::context context;
    czmqpp::socket socket(context, ZMQ_DEALER);
    if (socket.connect(argv[1]) < 0)
    {
        std::cerr << "Cannot connect to " << argv[1] << std::endl;
        return 1;
    }

    auto stream = std::make_shared<bc::client::socket_stream>(socket);
    auto codec = std::make_shared<bc::client::obelisk_codec>(
        std::static_pointer_cast<bc::client::message_stream>(stream));

    // Make the request:
    const auto error_handler = [](const code& code)
    {
        std::cout << "error: " << code.message() << std::endl;
    };
    const auto handler = [](size_t height)
    {
        std::cout << "height: " << height << std::endl;
    };
    codec->fetch_last_height(error_handler, handler);

    // Wait for the response:
    while (codec->outstanding_call_count())
    {
        czmqpp::poller poller;
        poller.add(socket);

        // Figure out how much timeout we have left:
        const auto delay = static_cast<int>(codec->wakeup().count());
        if (!delay)
            break;

        // Sleep:
        poller.wait(delay);
        if (poller.terminated())
            break;

        if (!poller.expired())
            stream->signal_response(codec);
    }

    return 0;
}
