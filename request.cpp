//
// Created by 宋志康 on 2025/2/19.
//


#include "request.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "utils.h"
void Request::pharse_request(std::string request){
    //pharse the request line
    size_t pos = request.find("\r\n");
    if(pos == std::string::npos){
        throw std::invalid_argument("Invalid request: No line found");
    }
    line = request.substr(0,pos);

    //pharse the method
    pos = line.find(" ");
    if(pos == std::string::npos){
        throw std::invalid_argument("Invalid request: No method found");
    }
    method = line.substr(0,pos);

    //pharse the host
    // 1. find the Host header
    pos = request.find("Host: ");
    if (pos == std::string::npos) {
        throw std::invalid_argument("Invalid request: No Host header found");
    }

    // 2. find the end of the Host header line
    std::string after_host = request.substr(pos + 6);
    size_t host_line_end = after_host.find_first_of("\r\n");
    if (host_line_end == std::string::npos) {
        throw std::invalid_argument("Invalid request: Malformed Host header");
    }

    std::string host_line = after_host.substr(0, host_line_end);
    if (host_line.empty()) {
        throw std::invalid_argument("Invalid request: Empty Host value");
    }

    std::cout<<host_line<<std::endl;

    // 3. pharse the host and port
    size_t port_begin = host_line.find_first_of(':');
    if (port_begin != std::string::npos) {
        // has port
        host = host_line.substr(0, port_begin);
        port = host_line.substr(port_begin + 1);

        // verify port
        if (port.empty() || !std::all_of(port.begin(), port.end(), ::isdigit)) {
            throw std::invalid_argument("Invalid request: Invalid port format");
        }
    } else {
        // use default port 80
        host = host_line;
        port = "80";
    }

    // process host format
    host.erase(0, host.find_first_not_of(" \t"));            // delete leading space
    host.erase(host.find_last_not_of(" \t") + 1);            // delete trailing space

    if (host.empty()) {
        throw std::invalid_argument("Invalid request: Empty host after trimming");
    }

    //get content length
    // skip CONNECT method
    if(method == "CONNECT"){
        content_length = 0;
        return;
    }

    content_length = get_content_length_from_head(request);


}

std::string Request::get_host(){
    return host;
}

std::string Request::get_port(){
    return port;
}

std::string Request::get_line(){
    return line;
}

std::string Request::get_method(){
    return method;
}

size_t Request::get_content_length(){
    return content_length;
}
