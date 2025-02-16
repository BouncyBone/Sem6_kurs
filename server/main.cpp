#include "head.h"

int main(int argc, char *argv[]) {
    string file_error = "error.txt";
    string base_file = "base.txt";
    string user_log = "usrlog.txt";

    cout << "Сервер успешно запущен" << endl;
    Server Server;
    int s = Server.self_addr(file_error);
    
    std::vector<std::thread> threads;

    while (true) {
        int work_sock = Server.client_addr(s, file_error);
        threads.emplace_back([work_sock, base_file, file_error, user_log]() {
            try {
                autorized(work_sock, base_file, file_error, user_log);
            } catch (const std::exception &e) {
                std::cerr << "Ошибка в потоке: " << e.what() << std::endl;
            }
            close(work_sock); // Закрываем сокет клиента после обработки
        });

        // Потоки теперь хранятся до завершения всей работы сервера
    }

    // Это не выполняется, так как while(true), но если сервер когда-то будет завершаться, это полезно
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    close(s);
    return 0;
}
