#include "head.h"

string version = "1.0";

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
    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(server_sock, buffer, sizeof(buffer), 0);
    /*if (bytesReceived <= 0) {
        std::cerr << "Ошибка при получении данных\n";
        return;
    }
    // Преобразуем полученное в строку
    */
    serverMessage.assign(buffer, bytesReceived);

    if (serverMessage == "Ошибка отправки файла" || serverMessage == "Версия клиента не соответствует") {
        cout<<serverMessage<<endl;
        std::cerr << "Ошибка на сервере: " << serverMessage << std::endl;
        return;  
    }
    else{
        file.write(buffer, bytesReceived); 

        while ((bytesReceived = recv(server_sock, buffer, sizeof(buffer), 0)) > 0) {
        if (std::string(buffer, bytesReceived) == "<END_OF_FILE>") {
            break;
        }
        file.write(buffer, bytesReceived);
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
    recv(sock, buffer, sizeof(buffer) -1, 0);
    std::cout << buffer << std::endl;
}

void sendMessage(int sock, std::string message) { //Отправка сообщения
    char buffer[4096];  // Используем локальный массив, чтобы избежать утечек памяти
    memset(buffer, 0, sizeof(buffer));  // Заполняем нулями
    strncpy(buffer, message.c_str(), sizeof(buffer) - 1);  // Копируем с ограничением
    send(sock, buffer, strlen(buffer)+1, 0);
}

string MD(string password,string salt){ // Кодирование пароля
    Weak::MD5 hash;
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

        char salt[1024] = {0};
        recv(sock, salt, sizeof(salt)-1, 0);
        cout<<"salt: \n"<<salt<<endl;
        receiveMessage(sock);
        //salt[1023] = '\0';
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

    char salt[1024] = {0};
    recv(sock, salt, sizeof(salt)-1, 0);
    receiveMessage(sock);
    std::cin >> password;
    password+=string(salt);
    string hashq = MD(password,salt);
    sendMessage(sock, hashq);
    
    char option;
    while (true) {
        sleep(1);
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
        sleep(1);
    }
    
    close(sock);
    return 0;
}



int main(int argc, char *argv[]){
    connection();
}