#include "head.h"

string version = "1.0";

void receiveFile(int server_sock, const std::string& filename) { //Функция приема файла
    char buffer[4096];
    ssize_t bytesReceived;
    std::string serverMessage;

    // Получаем первое сообщение от сервера (может быть как ошибка, так и начало файла)
    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(server_sock, buffer, sizeof(buffer), 0);
    serverMessage.assign(buffer, bytesReceived);
    if (serverMessage.find("Ошибка отправки файла") != std::string::npos || 
        serverMessage.find("Версия клиента не соответствует") != std::string::npos) { //Проверка на наличие ошибки в сообщении
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

        //Записываем принимаемые сообщения, пока не поступит сообщение об окончании передачи
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
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer)); 
    strncpy(buffer, message.c_str(), sizeof(buffer) - 1);
    send(sock, buffer, strlen(buffer)+1, 0);
}

string MD(string password,string salt){ // Кодирование пароля
    Weak::MD5 hash;
    string hashq;
    HexEncoder encoder(new StringSink(hashq));
    StringSource(password, true, new HashFilter(hash, new Redirector(encoder)));
    return(hashq);
}

void disableEcho() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void enableEcho() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void resetTerminal() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

int connection() { //Взаимодействие с сервером
    int sock = socket(AF_INET, SOCK_STREAM, 0); //Создание сокета
    if (sock == -1) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return -1;
    }

    sockaddr_in server_addr; //Структура сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //IP хоста

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
        if (command == "регистрация" || command == "вход"){ // Проверка корректности введенного аргумента
            break;
        } 
        else{
            cout<<"Введен неправильный аргумент"<<endl;
        }
    }
    sendMessage(sock, command);

    if (command == "регистрация") { //Регистрация
        std::string login, password;
        char ans[1024] = {0};
        receiveMessage(sock); //Введите логин
        while(true){
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin >> login;
            sendMessage(sock, login); //Отправка логина
            char flag[1024]={0};
            recv(sock, flag, sizeof(flag)-1,0); //Проверка корректности логина
            string flg(flag);
            if (flg.find("OK")!= std::string::npos){
                break;
            }
            else{
                cout<<flg<<endl;
            }
        }

        char salt[1024] = {0};
        recv(sock, salt, sizeof(salt)-1, 0); //Прием соли
        receiveMessage(sock); // Введите пароль
        disableEcho();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin >> password;
        enableEcho();
        password+=string(salt);
        string hashq = MD(password,salt); //Хэшироавние пароля
        sendMessage(sock, hashq);
        receiveMessage(sock);
    }

    std::string log, password; 
    //Вход на сервер
    receiveMessage(sock); //Введите логин
    while(true){
        std::cin >> log;
        sendMessage(sock, log); //Отправка логина
        char flag[1024]={0};
        recv(sock, flag, sizeof(flag)-1,0); //Проверка корректности логина
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
    recv(sock, salt, sizeof(salt) - 1, 0); //Прием соли
    sleep(1);
    receiveMessage(sock); //Введите пароль
    while (true){
        disableEcho();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin >> password;
        enableEcho();
        password+=string(salt);
        string hashq = MD(password,salt); //Хэширование пароля
        sendMessage(sock, hashq);
        char flag[1024]={0};
        recv(sock, flag, sizeof(flag)-1,0); //Прием сообщения о статусе пароля
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
    
    string option;
    while (true) { //Интерфейс
        receiveMessage(sock);
        while (true){ //Проверка правильности введенного аргумента
            std::getline(std::cin, option);
            if (option.length()!= 1 || (option[0] != 'q' && option[0] != 'd' && option[0] != 'l')){ 
                cout<<"Введен неправильный аргумент"<<endl;
            }
            else{
                break;
            }
        }
        char opt = option[0];
        send(sock, &opt, sizeof(opt), 0); //Отправка запроса в интерфейсе
        if (option[0] == 'l') {
            receiveMessage(sock);
        } else if (option[0] == 'd') {
            receiveMessage(sock);
            std::string filename;
            std::cin >> filename;
            sendMessage(sock, filename);
            receiveFile(sock, filename);
        } else if (option[0] == 'q') {
            resetTerminal();
            break;
        }
        sleep(1);
    }
    
    close(sock);
    return 0;
}



int main(int argc, char *argv[]){
    connection();
    resetTerminal();
}
