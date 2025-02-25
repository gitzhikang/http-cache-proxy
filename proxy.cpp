//
// Created by 宋志康 on 2025/2/19.
//

#include "proxy.h"
#include "utils.h"
#include "info.h"
#include "request.h"
#include <pthread.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>

std::string response_400 = "HTTP/1.1 400 Bad Request\r\n\r\n";
std::string response_200 = "HTTP/1.1 200 OK\r\n\r\n";
std::string response_502 = "HTTP/1.1 502 Bad Gateway\r\n\r\n";

void Proxy::run() {
  std::cout<<"Proxy server is running on port "<<port_num<<std::endl;
  int proxy_socket_fd = build_server_socket(port_num);
  if (proxy_socket_fd == -1) {
    write_log_with_lock("(no-id): Error: cannot create proxy server socket");
  }

  int client_id = 0;
  while (true) {
    std::string client_ip;
    int client_socket_fd = block_accept(proxy_socket_fd, &client_ip);
    if (client_socket_fd == -1) {
      write_log_with_lock("(no-id): Error: cannot accept connection on proxy server socket");
      continue;
    }

    // 创建新的线程数据
    ThreadData* thread_data = new ThreadData{
      this,                  // proxy指针
      client_socket_fd,      // 客户端socket
      client_ip,            // 客户端IP
      client_id++                   // 客户端ID
  };

    pthread_t thread;
    pthread_create(&thread, NULL, &Proxy::thread_handler, thread_data);
  }
}

// 静态线程处理函数，用于启动实际的处理方法
void* Proxy::thread_handler(void* arg) {
  ThreadData* data = static_cast<ThreadData*>(arg);
  void* result = data->proxy->handle_request(data);
  delete data;
  return result;
}

void * Proxy::handle_request(void * info){
  ThreadData* thread_data = static_cast<ThreadData*>(info);
  int client_socket_fd = thread_data->client_socket_fd;

  //the length of HTTP head is less than 8KB, to fetch the http head
  char req_msg_buffer[8192];
  int bytes_received = recv(client_socket_fd,req_msg_buffer,sizeof(req_msg_buffer),0);
  if(bytes_received == -1){
    write_log_with_lock("(no-id): Error: cannot receive request message from client");
    close(client_socket_fd);
    return NULL;
  }
  std::string req_msg_partial(req_msg_buffer,bytes_received);
  if (req_msg_partial == "" || req_msg_partial == "\r" || req_msg_partial == "\n" || req_msg_partial == "\r\n") {
    close(client_socket_fd);
    return NULL;
  }

  //parse head info of request message
  Request req;
  try{
    req.pharse_request(req_msg_partial);
    if(req.get_method() != "GET" && req.get_method() != "POST" && req.get_method() != "CONNECT"){
      throw std::invalid_argument("Invalid request: Method is not GET");
    }
  }catch(std::invalid_argument &e){
    write_log_with_lock("(no-id): Error: " + std::string(e.what()));
    //send 400 to the client
    std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
    send(client_socket_fd,response.c_str(),response.size(),0);
    close(client_socket_fd);
    return NULL;
  }

  //log the request
  std::string request_log = std::to_string(thread_data->client_id) + ": \"" + req.get_line()+"\" from " + thread_data->client_ip + " @ " + get_current_time().append("\0");
  write_log_with_lock(request_log);

  //handle request according to the method
  const char * host = req.get_host().c_str();
  const char * port = req.get_port().c_str();
  int origin_server_fd = build_client_socket(host,port);
  try{
    if(origin_server_fd == -1){
      throw std::invalid_argument("cannot connect to origin server");
    }
    if(req.get_method() == "CONNECT"){
      handle_connect(client_socket_fd,origin_server_fd,thread_data->client_id);
    }else if(req.get_method() == "POST"){
      handle_post(client_socket_fd,origin_server_fd,thread_data->client_id,req,req_msg_partial);
    }else{
      handle_get(client_socket_fd,origin_server_fd,thread_data->client_id,req,req_msg_partial);
    }
  }catch (std::invalid_argument &e){
    write_log_with_lock("(no-id): Error: " + std::string(e.what()));
    //send 400 to the client
    close(client_socket_fd);
    close(origin_server_fd);
    return NULL;
  }


}

void Proxy::handle_get(int client_socket_fd, int origin_socket_fd, int client_id, Request& req,std::string &req_msg){
  //get the entire http request by content length
  //will throw exception if the request is invalid
  get_entire_http_request_by_content_length(client_socket_fd,req_msg,req.get_content_length());

  //query the cache
  std::string query_key = get_cache_query_key(client_socket_fd,req);
  Response cached_res;
  if(cache.find(query_key)){

    cached_res = *cache.get(query_key);
    if(cached_res.need_revalidation()){
      //if the no-cache is true, send the request to the origin server to validate the cache
      //write the log of cache validation
      write_log_with_lock(std::to_string(client_id) + ": in cache, requires validation");

      //send the request to the origin server
      send(origin_socket_fd,req_msg.c_str(),req_msg.size(),0);
      write_log_with_lock(std::to_string(client_id) + ": Requesting \"" + req.get_line() + "\" from " + req.get_host());

      //receive the response from the origin server
      std::string response_buffer;
      get_entire_http_content(origin_socket_fd,response_buffer);
      Response validation_res;
      validation_res.pharse_response(response_buffer);

      write_log_with_lock(std::to_string(client_id) + ": Received \"" + validation_res.get_line() + "\" from " + req.get_host());

      //process response with differnt code
      if(validation_res.get_response_code() == "304"){
        //if the response code is 304, send the cached response to the client
        send_response_to_client(client_socket_fd,true,cached_res.get_origin_response().c_str(),cached_res);
        write_log_with_lock(std::to_string(client_id) + ": Responding \"" + cached_res.get_line());
      }else if(validation_res.get_response_code() == "200"){
        //if the response code is 200, update the cache and send the response to the client
        save_response_to_cache(query_key,validation_res,client_id);
        send_response_to_client(client_socket_fd,false,validation_res.get_origin_response().c_str(),cached_res);
        write_log_with_lock(std::to_string(client_id) + ": Responding \"" + validation_res.get_line());
      }else{
        //if the response code is 400 or other error codes, send the 502 response to the client
        send(client_socket_fd,response_502.c_str(),response_502.size(),0);
        write_log_with_lock(std::to_string(client_id) + ": Responding \"" + response_502);
      }
    }else{
      //verify the time of the cached response
      if(cached_res.get_max_age() != -1){
        //if the max-age is set, verify the age of the cached response（has higer priority）
        std::string respnse_time =  cached_res.get_response_time();
        std::string max_age_expire_time = max_age_to_GMT(respnse_time,cached_res.get_max_age());
        if(is_GMT_time_greater_than_current_time(max_age_expire_time)){
          //if the response is not expired, send the cached response to the client
          send_response_to_client(client_socket_fd,true,cached_res.get_origin_response().c_str(),cached_res);
          write_log_with_lock(std::to_string(client_id) + ": Responding \"" + cached_res.get_line());
        }else{
          handle_cache_miss(client_socket_fd,origin_socket_fd,client_id,req,req_msg);
        }
      }else if(cached_res.get_expires() != ""){
        //if has expires, verify the expires time of the cached response
        std::string expires = cached_res.get_expires();
        if(is_GMT_time_greater_than_current_time(expires)){
          //if the response is not expired, send the cached response to the client
          send_response_to_client(client_socket_fd,true,cached_res.get_origin_response().c_str(),cached_res);
          write_log_with_lock(std::to_string(client_id) + ": Responding \"" + cached_res.get_line());
        }else{
          handle_cache_miss(client_socket_fd,origin_socket_fd,client_id,req,req_msg);
        }
      }else{
        //if the response has no max-age and expires, send the cached response to the client
        send_response_to_client(client_socket_fd,true,cached_res.get_origin_response().c_str(),cached_res);
        write_log_with_lock(std::to_string(client_id) + ": Responding \"" + cached_res.get_line());
      }
    }
  }else{
    //not find in cache, send the request to the origin server
    //1. write the log of cache miss
    write_log_with_lock(std::to_string(client_id) + ": not in cache");
    //2. handle cache miss, send the request to the origin server
    handle_cache_miss(client_socket_fd,origin_socket_fd,client_id,req,req_msg);
  }
}

void Proxy::handle_cache_miss(int client_socket_fd, int origin_socket_fd, int client_id, Request& req,std::string &req_msg){
  //1. send the request to the origin server
  send(origin_socket_fd,req_msg.c_str(),req_msg.size(),0);

  //2. write the log of requesting
  write_log_with_lock(std::to_string(client_id) + ": Requesting \"" + req.get_line() + "\" from " + req.get_host());

  //3. receive the response from the origin server
  std::string response_buffer;
  Response res;
  get_entire_http_content(origin_socket_fd,response_buffer);
  res.pharse_response(response_buffer);

  //test
  // write_log_with_lock("test response content");
  // write_log_with_lock(response_buffer);

  write_log_with_lock(std::to_string(client_id) + ": Received \"" + res.get_line() + "\" from " + req.get_host());

  //4. save in the cache
  if(res.get_response_code() == "200"){
    save_response_to_cache(get_cache_query_key(client_socket_fd,req),res,client_id);
  }else{
    //if the response code is not 200, send 502 to the client
    send(client_socket_fd,response_502.c_str(),response_502.size(),0);
    write_log_with_lock(std::to_string(client_id) + ": Responding \"" + response_502);
    return;
  }

  //5. send the response to the client
//  send(client_socket_fd,response_buffer.c_str(),response_buffer.size(),0);
  send_response_to_client(client_socket_fd,false,response_buffer.c_str(),res);
  write_log_with_lock(std::to_string(client_id) + ": Responding \"" + res.get_line());

}

void Proxy::handle_post(int client_socket_fd, int origin_socket_fd, int client_id, Request& req,std::string &req_msg){
    // get the entire http request by content length
    get_entire_http_request_by_content_length(client_socket_fd,req_msg,req.get_content_length());

    //send the request to the origin server
    send(origin_socket_fd,req_msg.c_str(),req_msg.size(),0);
    write_log_with_lock(std::to_string(client_id) + ": Requesting \"" + req.get_line() + "\" from " + req.get_host());

   //receive the response from the origin server
    std::string response_buffer;
    get_entire_http_content(origin_socket_fd,response_buffer);
    Response res;
    res.pharse_line(response_buffer);
    write_log_with_lock(std::to_string(client_id) + ": Received \"" + res.get_line() + "\" from " + req.get_host());

    //send the response to the client
    send_response_to_client(client_socket_fd,false,response_buffer.c_str(),res);
//    send(client_socket_fd,,response_buffer.size(),0);
    write_log_with_lock(std::to_string(client_id) + ": Responding \"" + res.get_line() );

}


void Proxy::handle_connect(int client_socket_fd, int origin_socket_fd, int client_id){
  //send 200 to the client
  send(client_socket_fd,response_200.c_str(),response_200.size(),0);
  // write log
  write_log_with_lock(std::to_string(client_id) + ": Responding \"HTTP/1.1 200 OK\"");
  //use select to monitor multiple file descriptors
  fd_set readfds;
  int max_fd = std::max(client_socket_fd,origin_socket_fd);
  while(true){
    FD_ZERO(&readfds);
    FD_SET(client_socket_fd,&readfds);
    FD_SET(origin_socket_fd,&readfds);

    int status = select(max_fd+1,&readfds,NULL,NULL,NULL);
    if(status == -1){
      write_log_with_lock(std::to_string(client_id) + ": Error when select");
      break;
    }
    int fd[2] = {client_socket_fd,origin_socket_fd};

    for(int i = 0;i<2;i++){
      char msg_buffer[8192] = {0};
      if(FD_ISSET(fd[i], &readfds)){
        int len = recv(fd[i],msg_buffer,sizeof(msg_buffer),0);
        if(len<=0){
          return;
        }else{
          if(send(fd[1-i],msg_buffer,len,0) == -1){
            return;
          }
        }
      }
    }
  }
}

void Proxy::send_response_to_client(int client_socket_fd, bool is_cache_hit,const char * content,Response &res){
  std::string host = get_local_socket_ip(client_socket_fd);
  std::string modified_content = add_Via_to_response(content,host);
  if(is_cache_hit){
    //set Age in the response head
    modified_content = add_Age_to_response(content,res);
  }
  send(client_socket_fd,modified_content.c_str(),modified_content.size(),0);
}

void Proxy::write_log_with_lock(const std::string &log){
  pthread_mutex_lock(&log_mutex);
  log_file << log << std::endl;
  pthread_mutex_unlock(&log_mutex);
}

void Proxy::save_response_to_cache(std::string key,Response &res,int client_id){
  if(!res.is_cacheable()){
    write_log_with_lock(std::to_string(client_id)+": not cacheable because " + (res.is_no_cache()?"no-cache":res.is_no_store()?"no-store":res.is_private_cache()?"private":res.get_max_age() == 0?"max-age=0":"Error"));
    return;
  }
  std::string response_time = res.get_response_time();
  if(res.get_max_age() != -1){
    write_log_with_lock(std::to_string(client_id)+": cached, expires at"+ max_age_to_GMT(response_time,res.get_max_age()));
  }else if(!res.get_expires().empty()){
    write_log_with_lock(std::to_string(client_id)+": cached, expires at"+ res.get_expires());
  }
  if(res.get_etag() != "" || res.get_last_modified() != ""){
    write_log_with_lock(std::to_string(client_id)+": cached, but requires re-validation");
  }
  cache.insert(key,res);
}


