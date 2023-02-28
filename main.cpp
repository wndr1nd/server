#include <iostream>
#include "pahosub.h"
#include "server.h"
#include <queue>
#include <thread>
#include "pahosub.h"


int main()
{

	std::queue <Handler> data_queue;

    bool flag = false;  //Что-то вроде event в питоне(незнаю делают ли так)

	std::thread th(server, std::ref(data_queue));

	std::thread thr(check_queue, std::ref(data_queue), std::ref(flag));

    std::thread thre(pahosub, std::ref(data_queue), std::ref(flag));

    std::cout << "Press Enter to exit" << std::endl;
	getchar(); //вместо бесконечного цикла

    flag = true;
	thre.join();
    thr.join();
	th.detach();    //очень плохо если делать detach а не join?

	return 0;
}



