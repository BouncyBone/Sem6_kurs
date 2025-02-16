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
        autorized(work_sock, base_file, file_error, user_log);
        threads.emplace_back(filesend, work_sock);
        threads.erase(std::remove_if(threads.begin(), threads.end(), [](std::thread &t) { return t.joinable(); }), threads.end());
    }
    for (auto& thread : threads) {
        if (thread.joinable())
            thread.join();
    }
    close(s);
    return 0;
}