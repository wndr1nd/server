#include <iostream>
#include <utility>
#include <vector>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <queue>
#include <windows.h>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>


#pragma comment(lib, "Ws2_32.lib")
using namespace std;




const string con = "host=localhost port=5500 dbname=dbtst user=postgres password =1234";
// подключение к дб


class Handler {
public:
// класс обработчик
//nlohmann для работы с json
//pqxx для работы с бд

    int devid;
    double hum;
    double temp;

    nlohmann::json json_data;
    string str_buff;


    explicit Handler(std::string str = "") :str_buff(std::move(str)) {
    // IDE(или компилятор) советуют использовать move для инициализации(меньше копий?)

        if (!str_buff.empty())
        {
            this->json_data = nlohmann::json::parse(str_buff);
        }

        devid = this->json_data["devid"];
        hum = this->json_data["humidity"];
        temp = this->json_data["temperature"];

        cout << devid << " " << hum << " " << temp << endl;

    }


};

//функция подключения к дб
// вызывается в функции check_queue(ниже) при появлении данных в очереди
// возможно стоит это все разнести на отдельные .cpp и .h файлы?

void conndb(Handler *hand)
{
    pqxx::connection data_db(con);

    pqxx::work worker(data_db); //вроде как через него(worker) работаем с бд
    cout << "Connect to db success" << endl;

    auto devid = hand->devid;
    auto hum = hand->hum;
    auto temp = hand->temp;

    data_db.prepare("insert", "INSERT INTO devicedata ("
                              "deviceid, temperature, humidity, d_time)"
                              "VALUES ("
                              "$1, $2, $3, $4)"); //я так понял это что-то вроде макроса
    //потом можно использовать insert

    cout << "prepare success" << endl;

    auto now = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(now);
    //для того чтобы в бд заполнить время

    worker.exec_prepared("insert", devid, hum, temp, ctime(&end_time));
    //ctime помечена устаревшей но она хорошо подходит

    cout << "PREPARED" << endl;
    worker.commit();
    cout << "commit success" << endl;
}

//проверка очереди на появление данных
//увидел мельком про conditional_variables, вроде как им можно заменить цикл проверки
//возможно стоит использовать mutex для синхронизации?
[[noreturn]] void check_queue(std::queue<Handler> &queue_data, bool & flag)
{
    while (true) {

        if (!queue_data.empty()) {
            cout << "NEW DATA!" << endl;
            conndb(&queue_data.front());
            queue_data.pop();
        }
        Sleep(1000);
        //cout << "NO DATA!" << endl;
        if (flag)
        {
            cout << "GOODBYE" << endl;
            break;
        }
    }
}



int server(std::queue<Handler> &queue_data)
{
    vector<char> buff(4096); // для хранения полученной информации от клиента
    WSADATA wsData;


    int erStat = WSAStartup(MAKEWORD(2, 2), &wsData);
    //вызывается для использования WS2_32.dll

    if (erStat)
    {
        cout << "Error please try again" << endl;
        cout << WSAGetLastError() << endl;
        return 1;
    }
    else
        cout << "WinSock initialization is OK" << endl;

    SOCKET ServSock = socket(AF_INET, SOCK_STREAM, 0);
    //серверный сокет
    if (ServSock == INVALID_SOCKET)
    {
        cout << "Error initialization socket # " << WSAGetLastError() << endl;
        closesocket(ServSock);
        WSACleanup();
        return 1;
    }
    else
        cout << "Server socket initialization is OK" << endl;

    in_addr ip_to_num{};
    erStat = inet_pton(AF_INET, "127.0.0.1", &ip_to_num);
    // локальный сервер
    if (erStat <= 0)
    {
        cout << "Error in IP translation to special numeric format" << endl;
        return 1;
    }

    sockaddr_in servInfo{};
    ZeroMemory(&servInfo, sizeof(servInfo));

    servInfo.sin_addr = ip_to_num;
    servInfo.sin_family = AF_INET;
    servInfo.sin_port = htons(1234);
    //заполняем информацию о серверном сокете
    erStat = ::bind(ServSock, (sockaddr*)&servInfo, sizeof(servInfo));

    if ( erStat != 0 ) {
        cout << "Error Socket binding to server info. Error # " << WSAGetLastError() << endl;
        closesocket(ServSock);
        WSACleanup();
        return 1;
    }
    else
        cout << "Binding socket to Server info is OK" << endl;


    erStat = listen(ServSock, SOMAXCONN);

    if ( erStat != 0 ) {
        cout << "Can't start to listen to. Error # " << WSAGetLastError() << endl;
        closesocket(ServSock);
        WSACleanup();
        return 1;
    }
    else {
        cout << "Listening..." << endl;
    }

    sockaddr_in clientInfo{};
    ZeroMemory(&clientInfo, sizeof(clientInfo));
    int clientInfo_size = sizeof(clientInfo);

    SOCKET ClientConn;
    //создание клиентского сокета

    for (int i = 0; i < 100; ++i) {
        //возможно тут лучше вынести accept в отдельный поток? так как этот блокируется
        //кто-то советовал использовать boost::asio
        ClientConn = accept(ServSock, (sockaddr *) &clientInfo, &clientInfo_size);
        if (ClientConn == INVALID_SOCKET)
        {
            cout << "ERROR" << endl;
            closesocket(ServSock);
            closesocket(ClientConn);
            WSACleanup();
            return 1;
        }
        cout << "Connection to a client established successfully" << endl;

        recv(ClientConn, buff.data(), buff.size(), 0);
        // получение данных в вектор

        Handler hand(buff.data());
        //обработка
        queue_data.push(hand);
        cout << "QUEUE PUSH SUCCESS" << endl;
   }


    return 0;

}







//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) handler, &ClientConn, NULL, NULL); создание потока(v2)