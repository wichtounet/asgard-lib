//=======================================================================
// Copyright (c) 2015-2016 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <pair>

namespace asgard {

inline bool revoke_root(){
    if (getuid() == 0) {
        if (setgid(1000) != 0){
            std::cout << "asgard:dht11: setgid: Unable to drop group privileges: " << strerror(errno) << std::endl;
            return false;
        }

        if (setuid(1000) != 0){
            std::cout << "asgard:dht11: setgid: Unable to drop user privileges: " << strerror(errno) << std::endl;
            return false;
        }
    }

    if (setuid(0) != -1){
        std::cout << "asgard:dht11: managed to regain root privileges, exiting..." << std::endl;
        return false;
    }

    return true;
}

inline std::pair<int, std::string> exec_command(const std::string& command) {
    std::stringstream output;

    char buffer[1024];

    FILE* stream = popen(command.c_str(), "r");

    while (fgets(buffer, 1024, stream) != NULL) {
        output << buffer;
    }

    auto status = pclose(stream);
    auto exit_code = WEXITSTATUS(status);

    return std::make_pair(exit_code, output.str());
}


} //end of namespace asgard
