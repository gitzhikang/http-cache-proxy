//
// Created by 宋志康 on 2025/2/15.
//
#include "utils.h"
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <sstream>
using namespace std;

int build_server_socket(const char *port){
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;

    memset(&host_info, 0, sizeof(host_info));  //initialize, write 0 to &hostinfo
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    // get a list of IP addresses and port numbers for host hostname and service port
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    //get random port from kernel
    if(strcmp(port,"")==0){
        struct sockaddr_in * addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
        addr_in->sin_port = 0;
    }

    socket_fd = socket(host_info_list->ai_family,
               host_info_list->ai_socktype,
               host_info_list->ai_protocol);
    //creates an endpoint for communication and returns a descriptor
    //return value is a descriptor referencing the socket.

    if (socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    //allow address reuse
    status = ::bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    status = listen(socket_fd, 100);
    // accept incoming connections, define a queue limit for incoming connections
    if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    freeaddrinfo(host_info_list);
    return socket_fd;
}

int build_client_socket(const char *hostname, const char *port){
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    // get a list of IP addresses and port numbers for host hostname and service port
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    socket_fd = socket(host_info_list->ai_family,
               host_info_list->ai_socktype,
               host_info_list->ai_protocol);
    //creates an endpoint for communication and returns a descriptor
    //return value is a descriptor referencing the socket.

    if (socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    cout << "Connecting to " << hostname << " on port " << port << "..." << endl;

    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    //stream sockets may successfully connect() only once; Y
    //datagram sockets may use connect() multiple times to change their association.
    if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        return -1;
    } //if

    return socket_fd;
}

int block_accept(int socket_fd,std::string* ip){
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    //extracts first connection request on queue ,creates new socket , llocates a new file descriptor for the socket
    //select a socket for the purposes of doing an accept() by selecting it for read
    //returns a non-negative integer that is a descriptor for the accepted socket

    if (client_connection_fd == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;
        return -1;
    } //if

    struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
    *ip = inet_ntoa(addr->sin_addr);

    return client_connection_fd;
}

int get_local_socket_port(int socket_fd){
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getsockname(socket_fd, (struct sockaddr *)&addr, &addr_size);
    return ntohs(addr.sin_port);
}

std::string get_local_socket_ip(int socket_fd){
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getsockname(socket_fd, (struct sockaddr *)&addr, &addr_size);
    return std::string(inet_ntoa(addr.sin_addr));
}

int get_peer_socket_port(int socket_fd){
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(socket_fd, (struct sockaddr *)&addr, &addr_size);
    return ntohs(addr.sin_port);
}

std::string get_peer_socket_ip(int socket_fd) {
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int ret = getpeername(socket_fd, (struct sockaddr *)&addr, &addr_size);
    if (ret < 0) {
        throw std::invalid_argument("getpeername failed");
    }

    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN) == nullptr) {
        throw std::invalid_argument("inet_ntop failed");
    }

    return std::string(ip_str);
}

std::string get_current_time(){
    time_t currTime = time(0);
    struct tm * nowTime = gmtime(&currTime);
    const char * t = asctime(nowTime);
    return std::string(t);
}

void get_entire_http_request_by_content_length(int client_socket_fd, std::string &req_msg,size_t content_length){
  //get the entire http request accoding to the content-length
    size_t headers_end = req_msg.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        throw std::runtime_error("Invalid HTTP request format: missing headers end");
    }
    // calculate the length of recieved body
    size_t headers_length = headers_end + 4; // 加上 \r\n\r\n 的长度
    size_t received_body_length = 0;
    if (req_msg.length() > headers_length) {
        received_body_length = req_msg.length() - headers_length;
    }

    while(content_length>received_body_length){
         char buffer[1024];
         int len = recv(client_socket_fd,buffer,sizeof(buffer),0);
         if(len == -1){
              throw std::invalid_argument("Error: cannot receive request");
         }
         std::string buffer_str(buffer,len);
         received_body_length += len;
         req_msg += std::string(buffer,len);
    }

}

void get_entire_http_content(int recv_fd, std::string &msg){
    //1. get the head of the response
    char buffer[8192];
    int len = recv(recv_fd,buffer,sizeof(buffer),0);
    if(len == -1){
        throw std::invalid_argument("Error: cannot receive response from origin server");
    }
    msg += std::string(buffer,len);

    // 2. check if the response is complete
    size_t header_end = msg.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        throw std::invalid_argument("Error: incomplete HTTP header");
    }

    // 3. check if the message is chunked
    bool is_chunked = false;
    size_t chunked_pos = msg.find("Transfer-Encoding: chunked");
    if (chunked_pos != std::string::npos && chunked_pos < header_end) {
        is_chunked = true;
    }
    if(is_chunked) {
        get_entire_chunked_response(recv_fd, msg, header_end + 4); // +4 for "\r\n\r\n"
    }else {
        //2. get the content length
        int content_length = get_content_length_from_head(msg);
        get_entire_http_request_by_content_length(recv_fd,msg,content_length);
    }
}

size_t get_content_length_from_head(std::string &http_head){
    size_t pos = http_head.find("Content-Length: ");
    size_t content_length = 0;
    if(pos == std::string::npos){
        return content_length;
    }
    std::string content_length_str = http_head.substr(pos+16);
    size_t content_length_end = content_length_str.find_first_of("\r\n");
    if(content_length_end == std::string::npos){
        throw std::invalid_argument("Invalid request: Malformed Content-Length header");
    }
    content_length_str = content_length_str.substr(0,content_length_end);
    if(content_length_str.empty() || !std::all_of(content_length_str.begin(),content_length_str.end(),::isdigit)){
        throw std::invalid_argument("Invalid request: Invalid Content-Length format");
    }
    for (size_t i = 0; i < content_length_str.length(); i++) {
        content_length = content_length * 10 + (content_length_str[i] - '0');
    }
    return content_length;
}

void get_entire_chunked_response(int recv_fd, std::string &req_msg, size_t content_start) {
    // 保存原始headers以便后续修改
    std::string headers = req_msg.substr(0, content_start);

    // 用于存储解码后的响应体
    std::string decoded_body;

    // 保存当前处理位置
    size_t current_pos = content_start;

    while (true) {
        bool need_more_data = false;

        // 尝试处理当前缓冲区中的所有块
        while (true) {
            // 查找块大小行结束位置
            size_t chunk_size_end = req_msg.find("\r\n", current_pos);
            if (chunk_size_end == std::string::npos) {
                need_more_data = true;
                break;
            }

            // 解析块大小(十六进制)
            std::string chunk_size_str = req_msg.substr(current_pos, chunk_size_end - current_pos);
            int chunk_size = 0;
            std::istringstream(chunk_size_str) >> std::hex >> chunk_size;

            // 如果块大小为0，表示结束
            if (chunk_size == 0) {
                // 检查是否有最终CRLF
                if (chunk_size_end + 4 <= req_msg.length()) {
                    // 响应完成，现在去除Transfer-Encoding头并添加Content-Length
                    std::string modified_headers;
                    std::istringstream header_stream(headers);
                    std::string line;
                    bool has_empty_line = false;

                    // 处理每一行头部
                    while (std::getline(header_stream, line)) {
                        // 检查是否是空行
                        if (line.empty() || (line.size() == 1 && line[0] == '\r')) {
                            // 记录找到空行，但暂不添加
                            has_empty_line = true;
                            continue;
                        }

                        // 恢复可能被getline移除的\r
                        if (!line.empty() && line.back() != '\r') {
                            line += '\r';
                        }

                        // 跳过Transfer-Encoding头
                        if (line.find("Transfer-Encoding:") == std::string::npos) {
                            modified_headers += line + "\n";
                        }
                    }

                    // 添加Content-Length头
                    modified_headers += "Content-Length: " + std::to_string(decoded_body.size()) + "\r\n";

                    // 添加空行分隔头部和响应体
                    modified_headers += "\r\n";

                    // 重建完整响应
                    req_msg = modified_headers + decoded_body;
                    return;
                } else {
                    need_more_data = true;
                    break;
                }
            }

            // 检查是否有完整的数据块
            if (chunk_size_end + 2 + chunk_size + 2 > req_msg.length()) {
                need_more_data = true;
                break;
            }

            // 提取块数据并添加到解码内容
            size_t chunk_data_start = chunk_size_end + 2; // 跳过CRLF
            decoded_body.append(req_msg.substr(chunk_data_start, chunk_size));

            // 移动到下一块
            current_pos = chunk_data_start + chunk_size + 2; // 跳过块数据和尾部CRLF
        }

        if (need_more_data) {
            // 接收更多数据
            char buffer[8192];
            int len = recv(recv_fd, buffer, sizeof(buffer), 0);
            if (len <= 0) {
                if (len == 0) {
                    throw std::invalid_argument("Error: connection closed before complete chunked response");
                } else {
                    throw std::invalid_argument("Error: cannot receive response from origin server");
                }
            }
            req_msg += std::string(buffer, len);
        } else {
            // 所有块都已处理
            break;
        }
    }
}

std::string add_Via_to_response(const char * content,std::string &host){
    std::string modified_content = content;
    std::string via_header = "Via: ";

    size_t via_pos = modified_content.find(via_header);
    if(via_pos != std::string::npos) {
        // Via header exists, append host to it
        size_t line_end = modified_content.find("\r\n", via_pos);
        if(line_end != std::string::npos) {
            // Insert host at the end of existing Via line
            modified_content.insert(line_end, ", " + host);
        }
    } else {
        // Via header doesn't exist, add new Via header
        // Find the end of headers (marked by double CRLF)
        size_t headers_end = modified_content.find("\r\n\r\n");
        if(headers_end != std::string::npos) {
            // Insert new Via header before the end of headers
            std::string new_via = via_header + host + "\r\n";
            modified_content.insert(headers_end, "\r\n" + new_via);
        }
    }

    return modified_content;
}

std::string add_Age_to_response(const char * content,Response &res){
    std::string modified_content = content;
    std::string age_header = "Age: ";

    // 计算age值（当前时间 - 响应生成时间）
    time_t now;
    time(&now);
    struct tm response_tm = {};
    strptime(res.get_response_time().c_str(), "%a, %d %b %Y %H:%M:%S GMT", &response_tm);
    time_t response_time = timegm(&response_tm);
    int age = difftime(now, response_time);

    size_t age_pos = modified_content.find(age_header);
    if(age_pos != std::string::npos) {
        // Age header exists, update it
        size_t line_end = modified_content.find("\r\n", age_pos);
        if(line_end != std::string::npos) {
            // Replace existing Age value
            std::string new_age = age_header + std::to_string(age);
            modified_content.replace(age_pos, line_end - age_pos, new_age);
        }
    } else {
        // Age header doesn't exist, add new Age header
        size_t headers_end = modified_content.find("\r\n\r\n");
        if(headers_end != std::string::npos) {
            // Insert new Age header before the end of headers
            std::string new_age = age_header + std::to_string(age) + "\r\n";
            modified_content.insert(headers_end, "\r\n" + new_age);
        }
    }

    return modified_content;
}

std::string get_cache_query_key(int client_socket_fd,Request& req){
    std::string client_ip = get_peer_socket_ip(client_socket_fd);
    std::string client_port = std::to_string(get_peer_socket_port(client_socket_fd));
    return client_ip+client_port+req.get_line();
}

std::string max_age_to_GMT(std::string& response_time_str,int max_age){
    struct tm tm = {};

    // 解析GMT时间字符串
    if (strptime(response_time_str.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &tm) == nullptr) {
        return ""; // 解析失败
    }

    // 设置为GMT时间（不使用夏令时）
    tm.tm_isdst = 0;

    // 计算过期时间
    time_t response_time = timegm(&tm);
    time_t expiration_time = response_time + max_age;

    // 转换为GMT格式字符串
    struct tm* gmt = gmtime(&expiration_time);
    if (!gmt) {
        return "";
    }

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buffer);
}

bool is_GMT_time_greater_than_current_time(std::string &time_str){
    //1. 解析GMT时间字符串
    struct tm tm = {};
    if (strptime(time_str.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &tm) == nullptr) {
        return false; // 解析失败
    }
    tm.tm_isdst = 0;
    time_t gmt_time = timegm(&tm);

    time_t now = time(nullptr);

    return gmt_time > now;
}
