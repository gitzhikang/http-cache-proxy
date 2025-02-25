//
// Created by 宋志康 on 2025/2/19.
//

#include "response.h"

#include <iostream>

void Response::pharse_line(std::string &response_str){
    //pharse the request line
    size_t pos = response_str.find("\r\n");
    if(pos == std::string::npos){
        throw std::invalid_argument("Invalid request: No line found");
    }
    line = response_str.substr(0,pos);
}

void Response::pharse_response(std::string &response_str){

      //get http header
    size_t pos = response_str.find("\r\n\r\n");
    if(pos == std::string::npos){
        throw std::invalid_argument("Invalid response: No header found");
    }
    std::string header = response_str.substr(0,pos);

    //pharse the response line
    pos = header.find("\r\n");
    if(pos == std::string::npos){
        throw std::invalid_argument("Invalid response: No line found");
    }
    line = header.substr(0,pos);

    //pharse the response code
    size_t code_begin = line.find(" ");
    if(code_begin == std::string::npos){
        throw std::invalid_argument("Invalid response: No response code found");
    }
    response_code = line.substr(code_begin+1,3);


    //pharse cache control
    pos = header.find("Cache-Control: ");
    if(pos !=  std::string::npos) {
        size_t cache_control_end = header.find("\r\n",pos);
        if(cache_control_end == std::string::npos){
            throw std::invalid_argument("Invalid response: Malformed cache-control");
        }
        std::string cache_control_line = header.substr(pos+15,cache_control_end-pos-15);
        // std::cout << "cache_control_line: " << cache_control_line << std::endl;

        //1. pharse the max-age
        pos = cache_control_line.find("max-age=");
        if (pos != std::string::npos) {
            std::string max_age_str = cache_control_line.substr(pos + 8);

            size_t max_age_end = max_age_str.find_first_of(",");
            if (max_age_end == std::string::npos) {
                max_age_end = max_age_str.length();
            }

            std::string max_age_line = max_age_str.substr(0, max_age_end);
            if (max_age_line.empty()) {
                throw std::invalid_argument("Invalid response: Empty max-age value");
            }
            max_age = std::stoi(max_age_line);
        }

        //2. pharse the no-cache
        pos = cache_control_line.find("no-cache");
        if(pos != std::string::npos){
            no_cache = true;
        }

        //3. pharse the no-store
        pos = cache_control_line.find("no-store");
        if(pos != std::string::npos){
            no_store = true;
        }

        //4. pharse the private
        pos = cache_control_line.find("private");
        if(pos != std::string::npos){
            private_cache = true;
        }
    }


    //pharse the expires
    pos = header.find("Expires: ");
    if(pos != std::string::npos){
        std::string expires_str = header.substr(pos+9);
        size_t expires_end = expires_str.find_first_of("\r\n");
        if(expires_end == std::string::npos){
            throw std::invalid_argument("Invalid response: Malformed expires");
        }
        expires = expires_str.substr(0,expires_end);
    }



    //pharse the etag
    pos = header.find("ETag: ");
    if(pos != std::string::npos){
        std::string etag_str = header.substr(pos+6);
        size_t etag_end = etag_str.find_first_of("\r\n");
        if(etag_end == std::string::npos){
            throw std::invalid_argument("Invalid response: Malformed etag");
        }
        etag = etag_str.substr(0,etag_end);
    }

    //pharse the last-modified
    pos = header.find("Last-Modified: ");
    if(pos != std::string::npos){
        std::string last_modified_str = header.substr(pos+15);
        size_t last_modified_end = last_modified_str.find_first_of("\r\n");
        if(last_modified_end == std::string::npos){
            throw std::invalid_argument("Invalid response: Malformed last-modified");
        }
        last_modified = last_modified_str.substr(0,last_modified_end);
    }

    //pharse the response time from Date
    pos = header.find("Date: ");
    if(pos != std::string::npos){
        std::string date_str = header.substr(pos+6);
        size_t date_end = date_str.find_first_of("\r\n");
        if(date_end == std::string::npos){
            throw std::invalid_argument("Invalid response: Malformed date");
        }
        response_time = date_str.substr(0,date_end);
    }

    //pharse the transfer-encoding
    pos = header.find("Transfer-Encoding: ");
    if(pos != std::string::npos){
        std::string transfer_encoding_str = header.substr(pos+19);
        size_t transfer_encoding_end = transfer_encoding_str.find_first_of("\r\n");
        if(transfer_encoding_end == std::string::npos){
            throw std::invalid_argument("Invalid response: Malformed transfer-encoding");
        }
        std::string transfer_encoding_line = transfer_encoding_str.substr(0,transfer_encoding_end);
        if(transfer_encoding_line == "chunked"){
            chunked = true;
        }
    }

    origin_response = response_str;

}


