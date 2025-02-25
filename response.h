//
// Created by 宋志康 on 2025/2/19.
//

#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>

class Response {
    std::string origin_response;
    std::string response_time;
    std::string line;
    std::string response_code;
    int max_age;
    std::string expires;
    bool no_cache;
    bool no_store;
    bool private_cache;
    std::string etag;
    std::string last_modified;
    bool chunked;
public:
    Response() :
      origin_response(""),
      response_time(""),
      line(""),
      response_code(""),
      max_age(-1),
      expires(""),
      no_cache(false),
      no_store(false),
      private_cache(false),
      etag(""),
      last_modified(""),
    chunked(false)
    {}

    void pharse_line(std::string &response_str);
    void pharse_response(std::string &response_str);
    std::string get_line() { return line; }
    bool is_no_cache() { return no_cache; }
    std::string get_etag() { return etag; }
    std::string get_last_modified() { return last_modified; }
    std::string get_origin_response() { return origin_response; }
    std::string get_response_time() { return response_time; }
    int get_max_age() { return max_age; }
    std::string get_expires() { return expires; }
    std::string get_response_code() { return response_code; }
    bool is_chunked() { return chunked; }
    bool is_no_store() { return no_store; }
    bool is_private_cache() { return private_cache; }
    bool is_cacheable() {
      //if no-cache is true, and etag or last-modified is empty
        return !(no_cache && etag == "" && last_modified == "")&& !no_store && !private_cache;
    }
    bool need_revalidation() {
        return etag != "" || last_modified != "";
    }


};



#endif //RESPONSE_H
