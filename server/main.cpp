#include "head.h"

int main(int argc, char *argv[]) {
    string file_error = "error.txt"; //Файл для ошибок сетевого взаимодействия
    string base_file = "base.txt"; //Файл с базой данных пользователей
    string user_log = "usrlog.txt"; //Файл с данными о сессиях пользователей

    Server Server;
    int s = Server.self_addr(file_error);
    cout << "Сервер успешно запущен" << endl;
    
    std::vector<std::thread> threads; //Создание вектора потоков

    while (true) {
        int work_sock = Server.client_addr(s, file_error);
        threads.emplace_back([work_sock, base_file, file_error, user_log]() { //Создание потока
            try {
                autorized(work_sock, base_file, file_error, user_log);
            } catch (const std::exception &e) { //Обработка ошибок сетевого взаимодействия
                std::cerr << "Ошибка в потоке: " << e.what() << std::endl;
            }
            close(work_sock); //Закрываем сокет клиента после обработки
        });
        //Потоки хранятся до завершения всей работы сервера
    }
    // Это не выполняется, так как while(true), но если сервер когда-то будет завершаться, это полезно
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    close(s); //Закрытие серверного сокета
    return 0;
}
