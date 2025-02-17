#include "head.h"

string version = "1.0";

void receiveFile(int server_sock, const std::string& filename) {
        char buffer[4096];
    ssize_t bytesReceived;
    std::string serverMessage;
    // Получаем первое сообщение от сервера (может быть как ошибка, так и начало файла)
    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(server_sock, buffer, sizeof(buffer), 0);
    serverMessage.assign(buffer, bytesReceived);
    if (serverMessage.find("Ошибка отправки файла") != std::string::npos || 
        serverMessage.find("Версия клиента не соответствует") != std::string::npos) {
        cout << serverMessage << std::endl;
        return;  // Завершаем функцию, НЕ создавая файл
    }
    else{
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Ошибка открытия файла для записи\n";
            return;
        }
        file.write(buffer, bytesReceived); 

        while ((bytesReceived = recv(server_sock, buffer, sizeof(buffer), 0)) > 0) {
        if (std::string(buffer, bytesReceived).find("<END_OF_FILE>") != std::string::npos) {
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
    char buffer[4096] = {0};
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
    server_addr.sin_addr.s_addr = inet_addr("192.168.1.52"); //IP хоста

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка соединения с сервером" << std::endl;
        return -1;
    }

    sendMessage(sock,version); //Отправка версии клиента
    std::cout << "Успешное соединение с сервером" << std::endl;
    receiveMessage(sock);
    std::string command;
    while (true){
        std::cin >> command;
        if (command == "регистрация" || command == "вход"){
            break;
        } 
        else{
            cout<<"Введен неправильный аргумент"<<endl;
        }
    }
    sendMessage(sock, command);

    if (command == "регистрация") { //Регистрация
        std::string login, password;
        receiveMessage(sock);
        std::cin >> login;
        sendMessage(sock, login);

        char salt[1024] = {0};
        recv(sock, salt, sizeof(salt)-1, 0);
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
    while(true){
        std::cin >> log;
        sendMessage(sock, log);
        char flag[1024]={0};
        recv(sock, flag, sizeof(flag)-1,0);
        string flg(flag);
        //sleep(1);
        if (flg.find("OK")!= std::string::npos){
            break;
        }
        else{
            cout<<flg<<endl;
        }
    }
    char salt[2048] = {0};
    recv(sock, salt, sizeof(salt) - 1, 0);
    sleep(1);
    receiveMessage(sock); //Введите пароль
    while (true){
        std::cin >> password;
        password+=string(salt);
        string hashq = MD(password,salt);
        sendMessage(sock, hashq);
        //sleep(1);
        char flag[1024]={0};
        recv(sock, flag, sizeof(flag)-1,0);
        string flg(flag);
        //sleep(1);
        if (flg.find("OK")!= std::string::npos){
            break;
        }
        else if (flg.find("Ошибка пароля. Превышено количество попыток")!= std::string::npos){
            cout<<flg<<endl;
            close(sock);
            return 1;
        }
        else{
            cout<<flg<<endl;
        }
    }
    
    char option;
    while (true) {
        receiveMessage(sock);

        while (true){ //Проверка правильности введенного аргумента
            std::cin >> option;
            if (option != 'q' && option != 'd' && option != 'l'){ 
                cout<<"Введен неправильный аргумент"<<endl;
            }
            else{
                break;
            }
        }
        
        send(sock, &option, sizeof(option), 0);

        if (option == 'l') {
            receiveMessage(sock);
        } else if (option == 'd') {
            receiveMessage(sock);
            std::string filename;
            std::cin >> filename;
            sendMessage(sock, filename);
            //sleep(1);
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