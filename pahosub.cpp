#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include "mqtt/async_client.h"
#include "server.h"
#include <queue>


const std::string SERVER_ADDRESS("tcp://192.168.0.104:1883");
const std::string CLIENT_ID("sub");
const std::string TOPIC("/data");

const int QUAL = 0;
//качество обслуживания(QoS)
const int RETRY = 5;
//число попыток реконнекта


//коллбэки связанные с подпиской на топик(используются ниже в классе callback)
class action_listener : public virtual mqtt::iaction_listener
{
    std::string name_;

    void on_failure(const mqtt::token& tok) override {
        std::cout << name_ << " failure";
        if (tok.get_message_id() != 0)
            std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
    }

    void on_success(const mqtt::token& tok) override {
        std::cout << name_ << " success";
        if (tok.get_message_id() != 0)
            std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
    }

public:
    explicit action_listener(std::string name) : name_(std::move(name)) {}
};


//основные коллбэки
class callback : public virtual mqtt::callback,
                 public virtual mqtt::iaction_listener

{
    int retry_;

    mqtt::async_client& cli_;

    mqtt::connect_options& connOpts_;

    action_listener subListener_;
    //подписываемся на топик и используем коллбэки класса action_listener

    std::queue<Handler> &data_queue;

    void reconnect() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        try {
            cli_.connect(connOpts_, nullptr, *this);
        }
        catch (const mqtt::exception& exc) {
            std::cerr << "Error: " << exc.what() << std::endl;
            exit(1);
        }
    }

    void on_failure(const mqtt::token& tok) override {
        std::cout << "Connection attempt failed" << std::endl;
        if (++retry_ > RETRY)
            exit(1);
        reconnect();
    }

    void on_success(const mqtt::token& tok) override {
        std::cout << "Reconnected" << std::endl;
    }

    void connected(const std::string& cause) override {
        std::cout << "\nConnection success" << std::endl;
        std::cout << "\nSubscribing to topic '" << TOPIC << std::endl;

        //сама подписка при подключении к брокеру
        cli_.subscribe(TOPIC, QUAL, nullptr, subListener_);
    }

    void connection_lost(const std::string& cause) override {
        std::cout << "\nConnection lost" << std::endl;
        if (!cause.empty())
            std::cout << "\tcause: " << cause << std::endl;

        //ручной реконнект
//        std::cout << "Reconnecting..." << std::endl;
//        retry_ = 0;
//        reconnect();
    }

    //при получении данных кладем их в очередь
    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "Message arrived" << std::endl;

        std::cout << "\ttopic: '" << msg->get_topic() << std::endl;

        std::cout << "\tpayload: '" << msg->to_string() << std::endl;


        auto x = msg->to_string();


        Handler hand(x);

        data_queue.push(hand);
        std::cout << "SUCCESS DELIVERY TO QUEUE!" << std::endl;
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {}

public:
    callback(mqtt::async_client& cli, mqtt::connect_options& connOpts, std::queue<Handler> &queue_data)
            : retry_(0), cli_(cli), connOpts_(connOpts), subListener_("Sub"), data_queue(queue_data) {}
};


int pahosub(std::queue<Handler> &queue_data, bool& flag)
{

    mqtt::async_client cli(SERVER_ADDRESS, CLIENT_ID);



    mqtt::connect_options connOpts;

    connOpts.set_clean_session(false);

    callback cb(cli, connOpts, queue_data);
    cli.set_callback(cb);
    //установка коллбэков

    try {
        std::cout << "Connecting to the MQTT..." << std::flush;
        cli.connect(connOpts, nullptr, cb);
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "\nERROR: "
                  << SERVER_ADDRESS << " " << exc << std::endl;
        return 1;
    }


    //проверка флага для выхода
    while (!flag) {
        Sleep(1000);
    }


    try {
        std::cout << "\nDisconnecting from the MQTT..." << std::flush;
        cli.disconnect()->wait();
        std::cout << "OK" << std::endl;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << exc << std::endl;
        return 1;
    }

    return 0;
}