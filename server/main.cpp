#include "head.h"

int main(int argc, char *argv[]) {
    string file_error = "error.txt";
    string base_file = "base.txt";
    string user_log = "usrlog.txt";
    cout << "Сервер успешно запущен" << endl;
    Server Server;
    std::vector<std::thread> threads;
    int s = Server.self_addr(file_error);
    while (true) {
        int work_sock = Server.client_addr(s, file_error);
        threads.emplace_back([work_sock, base_file, file_error, user_log]() {
            try {
                autorized(work_sock, base_file, file_error, user_log);
            } catch (const std::exception &e) {
                std::cerr << "Ошибка в потоке: " << e.what() << std::endl;
            }
        });
        
        // Очистка завершённых потоков
        threads.erase(
            std::remove_if(threads.begin(), threads.end(), [](std::thread &t) {
                if (t.joinable()) {
                    t.join();
                    return true;
                }
                return false;
            }),
            threads.end()
        );
    }
    for (auto& thread : threads) {
        if (thread.joinable())
            thread.join();
    }
    close(s);
    return 0;
}