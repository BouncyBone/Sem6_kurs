#include "head.h"

string file_error = "error.txt"; //Файл для ошибок сетевого взаимодействия
string base_file = "base.txt"; //Файл с базой данных пользователей
string user_log = "usrlog.txt"; //Файл с данными о сессиях пользователей

void msgsend(int work_sock, string message){ //Отправка сообщения
    char buffer[4096];  
    memset(buffer, 0, sizeof(buffer));  
    strncpy(buffer, message.c_str(), sizeof(buffer) - 1);  
    if (send(work_sock, buffer, strlen(buffer)+1, 0) == -1) { //Проверка на корректность передачи
        cerr << "Ошибка отправки сообщения" << endl;
        close(work_sock);
        throw runtime_error("Client disconnected unexpectedly");
    }
}

void filesend(int work_sock, string filename, bool allow_txt, bool allow_bin, string login ){  //Функция отправки файла
    filesystem::path filePath(filename);
    std::string extension = filePath.extension().string();
    if ((extension == ".txt" and allow_txt == 0) or (extension == ".bin" and allow_bin == 0)){ //Проверка привелегий доступа
        string error = "Версия клиента не соответствует";
        msgsend(work_sock, error);
        errors(error, file_error, login);
    }

    else{ //Отправка файла
        std::ifstream file(filePath, std::ios::binary);  // Открываем файл в бинарном режиме
        if (!file) { //Удалось ли открыть файл
            string error = "Ошибка отправки файла";
            msgsend(work_sock, error);
            errors(error, file_error, login);
        }
        else{
            char buffer[4096];
            while (file.read(buffer, sizeof(buffer))) {
                send(work_sock, buffer, sizeof(buffer), 0);
            }
            const std::string EOF_MARKER = "<END_OF_FILE>";
            send(work_sock, EOF_MARKER.c_str(), EOF_MARKER.size(), 0); // Отправка последнего блока
            file.close();
        }
    }
}

void interface(int work_sock, char arg,string path, string login, string version){ //Интерфейс
    string fileListStr;
    switch(arg){
        case 'l': //Список всех файлов
            for (const auto &entry : std::filesystem::directory_iterator(path)) {
                fileListStr += entry.path().filename().string() + "   ";
            }
            if (!fileListStr.empty()) {
                msgsend(work_sock, "Список файлов: "+ fileListStr+"\n\n");
                return;
            } else {
                msgsend(work_sock, "Файлов нет.");
                return;
            }
            
        case 'd': //Запрос загрузки файла
            msgsend(work_sock, "Введите имя файла для загрузки:");
            char filename[1024];
            recv(work_sock, &filename, sizeof(filename), 0);
            string fullPath = path + string(filename);
            if (std::filesystem::exists(fullPath)) { //Проверка на существование искомого файла
                bool allow_txt, allow_bin;
                std::tie(allow_txt, allow_bin) = version_check(version, work_sock);
                sleep(1);
                filesend(work_sock, fullPath, allow_txt, allow_bin, login);
            } else {
                string error = "Файл с таким именем не существует";
                msgsend(work_sock, error);
                errors(error, file_error, login);
                throw InterfaceError(std::string("File not exist"));
                //return;
            }
    
    }
}

void errors(string error, string name, string login){ //Запись ошибки в файл
    ofstream file;
    file.open(name, ios::app);
    if(file.is_open()){
        time_t seconds = time(NULL);
        tm* timeinfo = localtime(&seconds);
        file<<error<<':'<<asctime(timeinfo) <<":"<<"user-"<<login<<endl;
        cout << "error: " << error << endl;
    }
}

void log_session_end(const string& login) { //Запись в файл сообщения о закрытии сессии пользователя
    ofstream log(user_log, ios::app);
    if (log.is_open()) {
        time_t seconds = time(NULL);
        tm* timeinfo = localtime(&seconds);
        log << "Session ended for user:" << login << ":" << asctime(timeinfo) << endl;
    }
}

int Server::self_addr(string file_error){  //Структура сервера и создание сокета
    int s = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in * self_addr = new (sockaddr_in); //Структура сервера
    self_addr->sin_family = AF_INET;
    self_addr->sin_port = htons(8080);
    self_addr->sin_addr.s_addr = inet_addr("192.168.1.52"); //IP хоста
    
    cout << "Wait for connect client...\n";
    int b = bind(s,(const sockaddr*) self_addr,sizeof(sockaddr_in));
    if(b == -1) {
        cout << "Binding error\n";
        string error = "error binding";
        errors(error, file_error);
        throw BindingError(std::string("Binding error"));
        return false; //
    }
    listen(s, 5); //Ожидание клиента с возможностью одновременного подключения 5 программ
    return s;
}

int Server::client_addr(int s, string file_error){ //Структура клиента
    sockaddr_in * client_addr = new sockaddr_in; 
    socklen_t len = sizeof (sockaddr_in);
    int work_sock = accept(s, (sockaddr*)(client_addr), &len);
    if(work_sock == -1) {
        cout << "Connection error\n";
        string error = "Connection error";
        errors(error, file_error);
        return 1;
    }
    else {
        //Успешное подключение к серверу
        cout << "Successfull client connection!\n";
        return work_sock;
    }
}

bool find_login(const std::string& base_file, const std::string& target_login) { //Поиск введенного логина в БД пользователей
    std::ifstream file(base_file);
    if (!file.is_open()) { //Удалось ли открыть файл
        std::cerr << "Ошибка открытия файла\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {  
        std::istringstream iss(line);
        std::string login;
        if (std::getline(iss, login, ':')) { 
            if (login == target_login) {
                file.close();
                return true; // Логин найден
            }
        }
    }
    file.close();
    return false; // Логин не найден
}

std::string find_password(const std::string& base_file, const std::string& target_login) { //Поиск по логину введенного пароля в БД пользователей
    std::ifstream file(base_file);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла\n";
        return "";
    }

    std::string line;
    while (std::getline(file, line)) {  
        std::istringstream iss(line);
        std::string login, password;
        
        if (std::getline(iss, login, ':') && std::getline(iss, password)) {
            if (login == target_login) {
                return password; //Найден пароль для нужного логина
            }
        }
    }
    return ""; //Если логин не найден
}


std::tuple<bool,bool> version_check(string version, int work_sock){ //Выдача привелегий в зависимости от версии
    if (version == "1.0"){
        bool allow_txt = 1;
        bool allow_bin = 0;
        return {allow_txt, allow_bin};
    }
    else if (version == "2.0"){
        bool allow_txt = 0;
        bool allow_bin = 1;
        return {allow_txt, allow_bin};
    }
    else{
        string error = "Неизвестная версия клиента"; //ОБработка ошибок по версии клиента
        errors(error, file_error);
        close(work_sock);
        throw AuthError(std::string("Unidentified version"));
    }
    
}

void authorization(int work_sock,string salt, string base_file){ //Функция регистрации пользователя
    string login = "SYSTEM";
    char mess[512];
    msgsend(work_sock, "Введите 'вход', если хотите зарегистрироваться или 'регистрация' для регистрации");
    recv(work_sock, &mess, sizeof(mess), 0);
    string mess_str(mess);

    if(mess_str == "регистрация"){ //Если пользователь хочет зарегистрироваться
        char *new_log = new char[1024];
        char *new_pass = new char[1024];
        
        msgsend(work_sock, "Введите логин");
        if (recv(work_sock, new_log, 1024, 0)<=0){
            cerr << "Client disconnected during registration" << endl;
            close(work_sock);
            return;
        }
        new_log[1023] = '\0';
        msgsend(work_sock, salt);

        msgsend(work_sock, "Введите пароль");
        if (recv(work_sock, new_pass, 1024, 0)<=0){
            cerr << "Client disconnected during registration" << endl;
            close(work_sock);
            return;
        }
        new_pass[1023] = '\0';
        
        fstream file1;
        file1.open(base_file, std::ios::app); //Запись в файл данных регистрации
        file1 << new_log<<":"<< new_pass<<"\n";
        file1.close();
        delete[] new_log;
        delete[] new_pass;
        msgsend(work_sock, "Регистрация успешно завершена\n");
        sleep(1);
    }
    else if(mess_str != "вход" && mess_str != "регистрация"){ //Введен некорректный параметр

        string error = "Registration error";
        msgsend(work_sock, error);
        errors(error, file_error);
        close(work_sock);
        throw AuthError(std::string("Auth error"));
    }
}

int autorized(int work_sock, string base_file, string file_error, string user_log){ //Авторизация
    string ok = "OK";
    string salt = "2D2D2D2D2D2D2D22";
    string err = "ERROR";
    string error;
    char msg[255];
    string path = "Client_files/"; //Путь к файлам пользователей

    bool allow_txt, allow_bin;
    char version[255];
    recv(work_sock, version, sizeof(version), 0); //Проверка версии клиента
    std::tie(allow_txt, allow_bin) = version_check(version, work_sock);

    authorization(work_sock, salt, base_file);
    string login;
    //Получение данных о пароле и логине
    msgsend(work_sock, "Введите логин");
    while (true) {
        memset(msg, 0, sizeof(msg));
        if (recv(work_sock, msg, sizeof(msg), 0) <= 0) {
            cerr << "Client disconnected during login input." << endl;
            close(work_sock);
            return 1;
        }
        login = string(msg);
        bool log_exist = find_login(base_file, login);
        if (!log_exist) { //Введен несуществующий логин
            error = "Ошибка логина";
            errors(error, file_error);
            msgsend(work_sock, "Введите логин заново");
        } else {
            msgsend(work_sock, "OK");
            break;
        }
    }
    // Отправка соли клиенту
    sleep(1); //Пауза между пересылками, чтобы не перепутать пакеты
    msgsend(work_sock, salt);
    string pass = find_password(base_file, login);
    sleep(1);
    msgsend(work_sock, "Введите пароль");

    /*Даем пользователю 3 попытки на ввод пароля
    Если все 3 попытки неудачные - закрываем соединение*/
    int attempts = 0;
    while (attempts < 3) {
        char get_pass[2048] = {0};
        if (recv(work_sock, get_pass, sizeof(get_pass) - 1, 0) <= 0) {
            cerr << "Client disconnected during password input." << endl;
            log_session_end(login);
            close(work_sock);
            return 1;
        }
        string received_pass(get_pass);
        received_pass.erase(received_pass.find_last_not_of("\r\n") + 1); //Обработка полученных данных для корректной работы с ними
    
        if (pass == received_pass) { //Если введен верный пароль
            msgsend(work_sock, "OK");
            ofstream log;
            log.open(user_log, ios::app);
            if (log.is_open()) {
                time_t seconds = time(NULL);
                tm* timeinfo = localtime(&seconds);
                log << "Session started for user" << ":" << login << ":" << asctime(timeinfo) << endl; // Запись времени начала сессии клиента
            }
            log.close();

            bool flag = 1;
            while (flag) { //Запускаем интерфейс пока клиент не захочет выйти из него
                //sleep(1);
                msgsend(work_sock, "Введите нужный параметр: \n l - список файлов \n d - загрузка файлов \n q - выход");
                char arg[512] = {0};
                if (recv(work_sock, arg, sizeof(arg), 0) <= 0) {
                    cerr << "Client disconnected during command selection." << endl;
                    log_session_end(login);
                    close(work_sock);
                    return 1;
                }
                string argg(arg);
                char argc = argg[0];
                if (argc == 'q') {
                    flag = 0;
                } else {
                    interface(work_sock, argc, path, login, version);
                    sleep(1);
                }
            }
            return 0;
        } else { //Если введен неправильный пароль
            attempts++;
            if (attempts < 3) {
                msgsend(work_sock, "Неверный пароль. Попробуйте снова.");
                error = "Wrond password for user: ";
                errors(error+login, file_error);
            } else { //Использовано 3 попытки входа
                error = "Ошибка пароля. Превышено количество попыток";
                msgsend(work_sock, error);
                errors(error, file_error);
                close(work_sock);
                throw AuthError(std::string("Password error"));
            }
        }
    }
    log_session_end(login); //Запись сообщения о закрытии сессии
    close(work_sock);
    return 1;
}