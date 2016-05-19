//=======================================================================
// Copyright (c) 2015-2016 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>

#include <string>
#include <vector>
#include <cstring>

#include <arpa/inet.h> //inet_addr
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

namespace asgard {

/*
 * This file provides send_message and receive_message that
 * implements the ASGARD protocol.
 */

/*
 * Note: Inspired from stackoverflow question 20149524
 */
inline bool read_n(int sd, char* p, std::size_t n) {
    std::size_t bytes_to_read = n;
    std::size_t bytes_read    = 0;

    while (bytes_to_read > bytes_read) {
        auto result = read(sd, p + bytes_read, bytes_to_read);
        if (result == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK){
                continue;
            } else {
                break;
            }
        } else if (!result) {
            break;
        }

        bytes_to_read -= result;
        bytes_read += result;
    }

    return bytes_read == n && bytes_to_read == 0;
}

/*!
 * Note: Inspired from stackoverflow question 24259640
 */
inline bool write_n(int sd, const char* b, std::size_t s) {
    std::size_t n = s;
    while (0 < n) {
        auto result = write(sd, b, n);
        if (result == -1) {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            } else {
                break;
            }
        }

        n -= result;
        b += result;
    }

    return n == 0;
}

inline bool send_message(int socket_fd, const char* message_buffer, std::size_t message_size){
    char buffer[32];

    auto n_bytes = snprintf(buffer, 32, "ASGARD");
    if(!write_n(socket_fd, buffer, n_bytes)){
        std::cerr << "Failed to send message" << std::endl;
        return false;
    }

    n_bytes = snprintf(buffer, 32, "%3ld", message_size);
    if(!write_n(socket_fd, buffer, n_bytes)){
        std::cerr << "Failed to send message" << std::endl;
        return false;
    }

    if(!write_n(socket_fd, message_buffer, message_size)){
        std::cerr << "Failed to send message" << std::endl;
        return false;
    }

    return true;
}

inline bool receive_message(int socket_fd, char* message_buffer, std::size_t message_size){
    char buffer[32];

    if(!read_n(socket_fd, buffer, 6)){
        std::cerr << "receive_message failed: Didn't receive header" << std::endl;
        return false;
    }

    if(!(buffer[0] == 'A' && buffer[1] == 'S' && buffer[2] == 'G' && buffer[3] == 'A' && buffer[4] == 'R' && buffer[5] == 'D')){
        std::cerr << "receive_message failed: Invalid header" << std::endl;
        return false;
    }

    if(!read_n(socket_fd, buffer, 3)){
        std::cerr << "receive_message failed: Didn't receive size" << std::endl;
        return false;
    }

    buffer[3] = '\0';

    auto n_bytes = atoi(buffer);

    if(n_bytes < 0){
        std::cerr << "receive_message failed: Size negative" << std::endl;
        return false;
    }

    if(static_cast<std::size_t>(n_bytes) + 1 > message_size){
        std::cerr << "receive_message failed: Buffer too small" << std::endl;
        return false;
    }

    if(!read_n(socket_fd, message_buffer, n_bytes)){
        std::cerr << "receive_message failed: Didn't receive message" << std::endl;
        return false;
    }

    message_buffer[n_bytes] = '\0';

    return true;
}

} //end of namespace asgard
