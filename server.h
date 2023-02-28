//
// Created by Wonderland on 25.12.2022.
//
#include <vector>
#include <winsock2.h>
#include <WS2tcpip.h>
//#include <string>
#include <queue>
#include "nlohmann/json.hpp"


#ifndef UNTITLED_SERVER_H
#define UNTITLED_SERVER_H

class Handler {
public:


    int devid;
    double hum;
    double temp;

    nlohmann::json json_data;
    std::string str_buff;

    explicit Handler(std::string str = "");
};

int server(std::queue<Handler> &queue_data);
[[noreturn]] void check_queue(std::queue<Handler> &queue_data, bool& flag);
#endif //UNTITLED_SERVER_H
