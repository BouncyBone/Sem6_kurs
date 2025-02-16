#include "head.h"
string file_error = "error.txt";
string base_file = "base.txt";
string user_log = "usrlog.txt";

void msgsend(int work_sock, string message){
    char *buffer = new char[4096];
    strcpy(buffer, message.c_str());
    send(work_sock, buffer, message.length(), 0);
}

void filesend(int work_sock, string filename, bool allow_txt, bool allow_bin, string login ){ 
    const int BUFFER_SIZE = 4096;
    filesystem::path filePath(filename);
    std::string extension = filePath.extension().string();

    if ((extension == ".txt" and allow_txt == 0) or (extension == ".bin" and allow_bin == 0)){ //Проверка привелегий доступа
        string error = "Версия клиента не соответствует";
        msgsend(work_sock, error);
        errors(error, file_error, login);
        throw AllowError(std::string("Permission Denied"));
        return;
    }

    else{ //Отправка файла
        std::ifstream file(filename, std::ios::binary);  // Открываем файл в бинарном режиме
        if (!file) {
            string error = "Ошибка отправки файла";
            msgsend(work_sock, error + ", попробуйте позднее.");
            errors(error, file_error, login);
            throw AllowError(std::string("Error open file"));
            return;
        }
        else{
            char buffer[BUFFER_SIZE];
            while (file.read(buffer, sizeof(buffer))) {
                send(work_sock, buffer, sizeof(buffer), 0);
            }
            send(work_sock, buffer, file.gcount(), 0);  // Отправка последнего блока

            file.close();
            msgsend(work_sock, "Загрузка завершена");
        }
    }
}

void interface(int work_sock, char arg,string path, string login, string version){
    std::vector<std::string> fileList;
    switch(arg){
        case 'l': //Список всех файлов
            msgsend(work_sock, "Список файлов: \n");
            for (const auto &entry : std::filesystem::directory_iterator(path)){
                fileList.push_back(entry.path().filename().string());
                string fileListStr;
                for (const auto &file : fileList) {
                    fileListStr += file + "   ";
                }
                msgsend(work_sock, fileListStr);
            }
            
        case 'd': //Запрос загрузки файла
            msgsend(work_sock, "Введите имя файла для загрузки:");
            string filename;
            recv(work_sock, &filename, sizeof(filename), 0);
            string fullPath = path + filename;

            if (std::filesystem::exists(fullPath)) { //Проверка на существование искомого файла
                bool allow_txt, allow_bin;
                std::tie(allow_txt, allow_bin) = version_check(version, work_sock);
                filesend(work_sock, filename, allow_txt, allow_bin, login);
            } else {
                string error = "Файл с таким именем не существует";
                msgsend(work_sock, error);
                errors(error, file_error, login);
                throw InterfaceError(std::string("File not exist"));
                return;
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

int Server::self_addr(string file_error){ 
    int s = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in * self_addr = new (sockaddr_in); //Структура сервера
    self_addr->sin_family = AF_INET;
    self_addr->sin_port = htons(8080);
    self_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
    
    cout << "Wait for connect client...\n";
    int b = bind(s,(const sockaddr*) self_addr,sizeof(sockaddr_in));
    if(b == -1) {
        cout << "Binding error\n";
        string error = "error binding";
        errors(error, file_error);
        throw BindingError(std::string("Binding error"));
        return false; //
    }
    listen(s, 5);
    return s;
}

int Server::client_addr(int s, string file_error){
    sockaddr_in * client_addr = new sockaddr_in; //Структура клиента
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

std::tuple<bool,bool> version_check(string version, int work_sock){
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

void authorization(int work_sock,string salt, string base_file){
    char mess[255];

    msgsend(work_sock, "Введите 'вход', если хотите зарегистрироваться или 'регистрация' для регистрации");
    recv(work_sock, &mess, sizeof(mess), 0);
    if(mess =="регистрация"){
        string new_log, new_pass;
        msgsend(work_sock, "Введите логин");
        recv(work_sock, &new_log, sizeof(new_log), 0);
        msgsend(work_sock,  salt);
        msgsend(work_sock, "Введите пароль");
        recv(work_sock, &new_pass, sizeof(new_pass), 0);
        fstream file1;
        file1.open(base_file, std::ios::app); //Запись в файл данных регистрации
        file1 << new_log,":",new_pass;
        file1.close();
        msgsend(work_sock, "Регистрация успешно завершена");
    }
    else if(mess != "вход"){
        string error = "Registration error";
        msgsend(work_sock, error);
        errors(error, file_error);
        throw AuthError(std::string("Auth error"));
    }
}

int autorized(int work_sock, string base_file, string file_error, string user_log){ //Авторизация
    string ok = "OK";
    string salt = "2D2D2D2D2D2D2D22";
    string err = "ERROR";
    string error;
    char msg[255];
    string path = "/client_files/"; //Путь к файлам пользователей

    bool allow_txt, allow_bin;
    string version;
    recv(work_sock, &version, sizeof(version), 0); //Проверка версии клиента
    std::tie(allow_txt, allow_bin) = version_check(version, work_sock);

    authorization(work_sock, salt, base_file);

    //Получение данных о пароле и логине
    msgsend(work_sock, "Введите логин");
    recv(work_sock, &msg, sizeof(msg), 0);
    string message = msg;
    memset(msg, 0, sizeof(msg));
    string login, pass;
    fstream file;
    file.open(base_file);
    getline (file, login, ':');
    getline(file, pass);

    if(message != login){
        msgsend(work_sock,  err);
        error = "Ошибка логина";
        errors(error, file_error);
        close(work_sock);
        file.close();
        throw AuthError(std::string("Login error"));
    }
    else{
        //Отправка соли клиенту
        msgsend(work_sock,  salt);
        char msg[255] = "";
        recv(work_sock, msg, sizeof(msg), 0);
        
    
        if(pass != msg){
            cout << pass << endl;
            cout << msg << endl;
            msgsend(work_sock,  err);
            error = "Ошибка пароля";
            errors(error, file_error);
            close(work_sock);
            file.close();
            throw AuthError(std::string("Password error"));
            return 1;

        }else{
            ofstream log;
            log.open(user_log, ios::app);
            if(log.is_open()){
                time_t seconds = time(NULL);
                tm* timeinfo = localtime(&seconds);
                log<<"Session started for user"<<":"<<login<<":"<<asctime(timeinfo)<<endl; //Запись времени начала сессии клиента
            }
            log.close();

            bool flag = 1;
            while(flag){
                msgsend(work_sock, "Введите нужный параметр: \n l - список файлов \n d - загрузка файлов \n q - выход");
                char arg;
                recv(work_sock, &arg, sizeof(arg), 0);
                if (arg != 'l' or arg !='d' or arg !='q'){
                    error = "Неправильный аргумент";
                    errors(error, file_error, login);
                    close(work_sock);
                    throw InterfaceError(std::string("Wrong argument"));
                }
                else if (arg == 'q'){
                    flag = 0;
                    exit(0);
                }
                else
                    interface(work_sock, arg, path, login, version);
            }
        }
    }
    ofstream log;
    log.open(user_log, ios::app);
    if(log.is_open()){
        time_t seconds = time(NULL);
        tm* timeinfo = localtime(&seconds);
        log<<"Session ended for user"<<":"<<login<<":"<<asctime(timeinfo)<<endl;
    }
    close(work_sock);
    return 1;
}