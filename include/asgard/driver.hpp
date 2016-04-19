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

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include "utils.hpp" //revoke_root for the drivers

namespace asgard {

const std::size_t UNIX_PATH_MAX = 108;
const std::size_t buffer_size = 4096;

struct KeyValue {
    std::string key;
    std::string value;
};

struct driver_connector {
    //Buffer
    char write_buffer[buffer_size];
    char receive_buffer[buffer_size];

    // The socket file descriptor
    int socket_fd;

    // The socket addresses
    struct sockaddr_un client_address;
    struct sockaddr_un server_address;
};

void load_config(std::vector<KeyValue>& config){
    std::ifstream file("/etc/asgard/conf");
    std::string str;
    std::size_t i = 0;

    while(std::getline(file, str)){
        std::istringstream line(str);
        config.push_back(KeyValue());
        if(std::getline(line, config[i].key, '=')){
            if(std::getline(line, config[i].value)){
                i++;
            }
        }
    }
}

std::string get_string_value(std::vector<KeyValue>& config, const std::string& key){
    for(std::size_t i=0; i<config.size(); ++i){
        if(config[i].key == key){
            return config[i].value;
        }
    }
    std::cout << "Key not found !" << std::endl;
    return "";
}

int get_int_value(std::vector<KeyValue>& config, const std::string& key){
    for(std::size_t i=0; i<config.size(); ++i){
        if(config[i].key == key){
            return std::stoi(config[i].value);
        }
    }
    std::cout << "Key not found !" << std::endl;
    return -1;
}

inline bool open_driver_connection(driver_connector& driver, const char* client_socket_path, const char* server_socket_path){
    // Open the socket
    driver.socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(driver.socket_fd < 0){
        std::cerr << "asgard:driversystem: socket() failed" << std::endl;
        return false;
    }

    // Init the client address
    memset(&driver.client_address, 0, sizeof(struct sockaddr_un));
    driver.client_address.sun_family = AF_UNIX;
    snprintf(driver.client_address.sun_path, UNIX_PATH_MAX, client_socket_path);

    // Unlink the client socket
    unlink(client_socket_path);

    // Bind to client socket
    if(bind(driver.socket_fd, (const struct sockaddr *) &driver.client_address, sizeof(struct sockaddr_un)) < 0){
        std::cerr << "asgard:driver: bind() failed" << std::endl;
        return false;
    }

    // Init the server address
    memset(&driver.server_address, 0, sizeof(struct sockaddr_un));
    driver.server_address.sun_family = AF_UNIX;
    snprintf(driver.server_address.sun_path, UNIX_PATH_MAX, server_socket_path);

    return true;
}

inline int register_source(driver_connector& driver, const std::string& source_name){
    socklen_t address_length = sizeof(struct sockaddr_un);

    // Register the source
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "REG_SOURCE %s", source_name.c_str());
    sendto(driver.socket_fd, driver.write_buffer, nbytes, 0, (struct sockaddr *) &driver.server_address, sizeof(struct sockaddr_un));

    auto bytes_received = recvfrom(driver.socket_fd, driver.receive_buffer, buffer_size, 0, (struct sockaddr *) &(driver.server_address), &address_length);
    driver.receive_buffer[bytes_received] = '\0';

    auto source_id = atoi(driver.receive_buffer);

    std::cout << "asgard:driver: remote source: " << source_id << std::endl;

    return source_id;
}

inline int register_sensor(driver_connector& driver, int source_id, const std::string& type, const std::string& name){
    socklen_t address_length = sizeof(struct sockaddr_un);

    // Register the sensor
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "REG_SENSOR %d %s %s", source_id, type.c_str(), name.c_str());
    sendto(driver.socket_fd, driver.write_buffer, nbytes, 0, (struct sockaddr *) &driver.server_address, sizeof(struct sockaddr_un));

    auto bytes_received = recvfrom(driver.socket_fd, driver.receive_buffer, buffer_size, 0, (struct sockaddr *) &(driver.server_address), &address_length);
    driver.receive_buffer[bytes_received] = '\0';

    auto sensor_id = atoi(driver.receive_buffer);

    std::cout << "asgard:driver: remote sensor(" << name << "):" << sensor_id << std::endl;

    return sensor_id;
}

inline int register_actuator(driver_connector& driver, int source_id, const std::string& name){
    socklen_t address_length = sizeof(struct sockaddr_un);

    // Register the actuator
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "REG_ACTUATOR %d %s", source_id, name.c_str());
    sendto(driver.socket_fd, driver.write_buffer, nbytes, 0, (struct sockaddr *) &driver.server_address, sizeof(struct sockaddr_un));

    auto bytes_received = recvfrom(driver.socket_fd, driver.receive_buffer, buffer_size, 0, (struct sockaddr *) &(driver.server_address), &address_length);
    driver.receive_buffer[bytes_received] = '\0';

    auto actuator_id = atoi(driver.receive_buffer);

    std::cout << "asgard:driver: remote actuator(" << name << "):" << actuator_id << std::endl;

    return actuator_id;
}

inline void send_data(driver_connector& driver, int source_id, int sensor_id, double value){
    // Send the data
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "DATA %d %d %.2f", source_id, sensor_id, value);
    sendto(driver.socket_fd, driver.write_buffer, nbytes, 0, (struct sockaddr *) &driver.server_address, sizeof(struct sockaddr_un));
}

inline void send_event(driver_connector& driver, int source_id, int actuator_id, std::string value){
    // Send the data
    auto nbytes = snprintf(driver.write_buffer, buffer_size, "EVENT %d %d %s", source_id, actuator_id, value.c_str());
    sendto(driver.socket_fd, driver.write_buffer, nbytes, 0, (struct sockaddr *) &driver.server_address, sizeof(struct sockaddr_un));
}

inline void unregister_sensor(driver_connector& driver, int source_id, int sensor_id){
    // Unregister the sensor, if necessary
    if(sensor_id >= 0){
        auto nbytes = snprintf(driver.write_buffer, buffer_size, "UNREG_SENSOR %d %d", source_id, sensor_id);
        sendto(driver.socket_fd, driver.write_buffer, nbytes, 0, (struct sockaddr *) &driver.server_address, sizeof(struct sockaddr_un));
    }
}

inline void unregister_actuator(driver_connector& driver, int source_id, int actuator_id){
    // Unregister the actuator, if necessary
    if(actuator_id >= 0){
        auto nbytes = snprintf(driver.write_buffer, buffer_size, "UNREG_ACTUATOR %d %d", source_id, actuator_id);
        sendto(driver.socket_fd, driver.write_buffer, nbytes, 0, (struct sockaddr *) &driver.server_address, sizeof(struct sockaddr_un));
    }
}

inline void unregister_source(driver_connector& driver, int source_id){
    // Unregister the source, if necessary
    if(source_id >= 0){
        auto nbytes = snprintf(driver.write_buffer, buffer_size, "UNREG_SOURCE %d", source_id);
        sendto(driver.socket_fd, driver.write_buffer, nbytes, 0, (struct sockaddr *) &driver.server_address, sizeof(struct sockaddr_un));
    }
}

} //end of namespace asgard
