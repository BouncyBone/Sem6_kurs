#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
//Сетевые библиотеки
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>

//Типы данных
#include <string>
#include <vector>
#include <tuple>

#include <iostream> //Ввод-вывод
#include <cassert> //Отладка
#include <cstdlib> //Работа с памятью
#include <ctime> // Работа со временем
#include <stdexcept> //Обработка исключений
#include <fstream> //Работа с файлами
#include <sstream> //Строковые потоки
#include <getopt.h> //Обработка аргументов
#include <thread> //Многопоточность
#include <filesystem> //Работа с файлами
using namespace std;
#include <cryptopp/md5.h>

void msgsend(int work_sock, string message); //Отправка сообщений
void errors(std::string error, std::string name, std::string login = "NONE"); //Обработка ошибок
void filesend(int work_sock, string filename, bool allow_txt, bool allow_bin, string login ); //Отправка файлов
int autorized(int work_sock, string file_name, string file_error,string user_log); //Взаимодействие с авторизованным пользователем
void interface(int work_sock, char arg,string path, string login, string version); //Интерфейс пользователя
void authorization(int work_sock,string salt, string base_file); //Авторизация пользователя
std::tuple<bool,bool> version_check(string version, int work_sock); //Проверка версии пользователя

extern std::string base_file;
extern std::string file_error;

//Класс сервера
class Server{
private:

public:
    int self_addr(string file_error);
    int client_addr(int s, string file_error);
};

class BindingError: public std::invalid_argument //Обработка ошибок при бинде сокета
{
public:
    BindingError(const std::string& s) : std::invalid_argument(s) {}
    BindingError(const char * s) : std::invalid_argument(s) {}
};

class AuthError: public std::invalid_argument //Обработка ошибок при авторизации
{
public:
    AuthError(const std::string& s) : std::invalid_argument(s) {}
    AuthError(const char * s) : std::invalid_argument(s) {}
};

class AllowError: public std::invalid_argument //Обработка ошибок при проверке привелегий
{
public:
    AllowError(const std::string& s) : std::invalid_argument(s) {}
    AllowError(const char * s) : std::invalid_argument(s) {}
};

class InterfaceError: public std::invalid_argument //Обработка ошибок при проверке привелегий
{
public:
    InterfaceError(const std::string& s) : std::invalid_argument(s) {}
    InterfaceError(const char * s) : std::invalid_argument(s) {}
};

