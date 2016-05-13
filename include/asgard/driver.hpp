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

#include "utils.hpp"  //revoke_root for the drivers
#include "config.hpp" //config for the drivers

namespace asgard {

struct driver_connector {
    //Buffer
    char write_buffer[buffer_size];
    char receive_buffer[buffer_size];

    // The socket file descriptor
    int socket_fd;
};

inline bool open_driver_connection(driver_connector& driver, const char* server_socket_addr, int server_socket_port) {
    // Open the socket
    driver.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (driver.socket_fd < 0) {
        std::cerr << "asgard:driversystem: socket() failed" << std::endl;
        return false;
    }

    struct sockaddr_in server;

    server.sin_addr.s_addr = inet_addr(server_socket_addr);
    server.sin_family = AF_INET;
    server.sin_port = htons(server_socket_port);
 
    //Connect to remote server
    if (connect(driver.socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        std::perror("connect failed. Error");
        return false;
    }    
    std::cout << "Connected\n" << std::endl;

    return true;
}

inline int register_source(driver_connector& driver, const std::string& source_name) {
    // Register the source
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "REG_SOURCE %s", source_name.c_str());
    send(driver.socket_fd, driver.write_buffer, nbytes, 0);

    auto bytes_received                   = recv(driver.socket_fd, driver.receive_buffer, buffer_size, 0);
    driver.receive_buffer[bytes_received] = '\0';

    auto source_id = atoi(driver.receive_buffer);

    std::cout << "asgard:driver: remote source: " << source_id << std::endl;

    return source_id;
}

inline int register_sensor(driver_connector& driver, int source_id, const std::string& type, const std::string& name) {
    // Register the sensor
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "REG_SENSOR %d %s %s", source_id, type.c_str(), name.c_str());
    send(driver.socket_fd, driver.write_buffer, nbytes, 0);

    auto bytes_received                   = recv(driver.socket_fd, driver.receive_buffer, buffer_size, 0);
    driver.receive_buffer[bytes_received] = '\0';

    auto sensor_id = atoi(driver.receive_buffer);

    std::cout << "asgard:driver: remote sensor(" << name << "):" << sensor_id << std::endl;

    return sensor_id;
}

inline int register_actuator(driver_connector& driver, int source_id, const std::string& name) {

    // Register the actuator
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "REG_ACTUATOR %d %s", source_id, name.c_str());
    send(driver.socket_fd, driver.write_buffer, nbytes, 0);

    auto bytes_received                   = recv(driver.socket_fd, driver.receive_buffer, buffer_size, 0);
    driver.receive_buffer[bytes_received] = '\0';

    auto actuator_id = atoi(driver.receive_buffer);

    std::cout << "asgard:driver: remote actuator(" << name << "):" << actuator_id << std::endl;

    return actuator_id;
}

inline void send_data(driver_connector& driver, int source_id, int sensor_id, double value) {
    // Send the data
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "DATA %d %d %.2f", source_id, sensor_id, value);
    send(driver.socket_fd, driver.write_buffer, nbytes, 0);
}

inline void send_event(driver_connector& driver, int source_id, int actuator_id, std::string value) {
    // Send the data
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "EVENT %d %d %s", source_id, actuator_id, value.c_str());
    send(driver.socket_fd, driver.write_buffer, nbytes, 0);
}

inline void unregister_sensor(driver_connector& driver, int source_id, int sensor_id) {
    // Unregister the sensor, if necessary
    if (sensor_id >= 0) {
        auto nbytes = snprintf(driver.write_buffer, buffer_size, "UNREG_SENSOR %d %d", source_id, sensor_id);
        send(driver.socket_fd, driver.write_buffer, nbytes, 0);
    }
}

inline void unregister_actuator(driver_connector& driver, int source_id, int actuator_id) {
    // Unregister the actuator, if necessary
    if (actuator_id >= 0) {
        auto nbytes = snprintf(driver.write_buffer, buffer_size, "UNREG_ACTUATOR %d %d", source_id, actuator_id);
        send(driver.socket_fd, driver.write_buffer, nbytes, 0);
    }
}

inline void unregister_source(driver_connector& driver, int source_id) {
    // Unregister the source, if necessary
    if (source_id >= 0) {
        auto nbytes = snprintf(driver.write_buffer, buffer_size, "UNREG_SOURCE %d", source_id);
        send(driver.socket_fd, driver.write_buffer, nbytes, 0);
    }
}

} //end of namespace asgard
