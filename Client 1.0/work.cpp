#include "head.h"

#define BUFFER_SIZE 4096
string version = "1.0";

void receiveMessage(int sock) { //Прием сообщения
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, sizeof(buffer), 0);
    std::cout << buffer << std::endl;
}

void sendMessage(int sock, std::string message) { //Отправка сообщения
    send(sock, message.c_str(), message.size(), 0);
}

string MD(string password){ // Кодирование пароля
    Weak1::MD5 hash;
    string hashq;
    HexEncoder encoder(new StringSink(hashq));
    StringSource(password, true, new HashFilter(hash, new Redirector(encoder)));
    return(hashq);
}

int connection() { //Взаимодействие с сервером
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return -1;
    }

    sockaddr_in server_addr; //Структура сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка соединения с сервером" << std::endl;
        return -1;
    }

    sendMessage(sock,version); //Отправка версии клиента
    std::cout << "Успешное соединение с сервером" << std::endl;
    receiveMessage(sock);

    std::string command;
    std::cout << "Введите команду (вход/регистрация): ";
    std::cin >> command;
    sendMessage(sock, command);

    if (command == "регистрация") { //Регистрация
        std::string login, password;
        receiveMessage(sock);
        std::cin >> login;
        sendMessage(sock, login);
        receiveMessage(sock);
        std::cin >> password;
        string hashq = MD(password);
        sendMessage(sock, hashq);
        receiveMessage(sock);
    }

    std::string login, password; //Вход на сервер
    receiveMessage(sock);
    std::cin >> login;
    sendMessage(sock, login);
    receiveMessage(sock);
    std::cin >> password;
    string hashq = MD(password);
    sendMessage(sock, hashq);

    receiveMessage(sock);
    
    char option;
    while (true) {
        receiveMessage(sock);
        std::cin >> option;
        send(sock, &option, sizeof(option), 0);
        
        if (option == 'l') {
            receiveMessage(sock);
        } else if (option == 'd') {
            receiveMessage(sock);
            std::string filename;
            std::cin >> filename;
            sendMessage(sock, filename);
        } else if (option == 'q') {
            break;
        }
    }
    
    close(sock);
    return 0;
}



int main(int argc, char *argv[]){
    connection();
}