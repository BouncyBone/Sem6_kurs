#include "head.h"

string version = "2.0";

void receiveFile(int server_sock, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла для записи\n";
        return;
    }
    char buffer[4096];
    ssize_t bytesReceived;
    std::string serverMessage;
    // Получаем первое сообщение от сервера (может быть как ошибка, так и начало файла)
    serverMessage = recv(server_sock, buffer, sizeof(buffer), 0);
    /*if (bytesReceived <= 0) {
        std::cerr << "Ошибка при получении данных\n";
        return;
    }
    // Преобразуем полученное в строку
    serverMessage.assign(buffer, bytesReceived);*/

    if (serverMessage == "Ошибка отправки файла" || "Версия клиента не соответствует") {
        std::cerr << "Ошибка на сервере: " << serverMessage << std::endl;
        return;  
    }
    else{
        file.write(buffer, bytesReceived); 

        while ((bytesReceived = recv(server_sock, buffer, sizeof(buffer), 0)) > 0) {
            file.write(buffer, bytesReceived);
            
            if (bytesReceived < sizeof(buffer)) {
                break;
            }
        }

        if (bytesReceived < 0) {
            std::cerr << "Ошибка при получении данных\n";
        } else {
            std::cout << "Файл успешно загружен: " << filename << std::endl;
        }

        file.close();
    }
}


void receiveMessage(int sock) { //Прием сообщения
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, sizeof(buffer), 0);
    std::cout << buffer << std::endl;
}

void sendMessage(int sock, std::string message) { //Отправка сообщения
    char *buffer = new char[1024];
    strcpy(buffer, message.c_str());
    send(sock, buffer, message.length(), 0);
}

string MD(string password,string salt){ // Кодирование пароля
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
    std::cin >> command;
    sendMessage(sock, command);

    if (command == "регистрация") { //Регистрация
        std::string login, password;
        receiveMessage(sock);
        std::cin >> login;
        sendMessage(sock, login);

        char salt[512];
        recv(sock, salt, sizeof(salt), 0);
        receiveMessage(sock);
        std::cin >> password;
        password+=string(salt);
        string hashq = MD(password,salt);
        sendMessage(sock, hashq);
        receiveMessage(sock);
    }

    std::string log, password; //Вход на сервер
    receiveMessage(sock); //Введите логин
    std::cin >> log;
    sendMessage(sock, log);

    char salt[512];
    recv(sock, salt, sizeof(salt), 0);
    receiveMessage(sock);
    std::cin >> password;
    password+=string(salt);
    string hashq = MD(password,salt);
    sendMessage(sock, hashq);
    
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
            sleep(1);
            receiveFile(sock, filename);
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