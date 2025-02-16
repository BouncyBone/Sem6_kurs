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
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1;
using namespace CryptoPP;
using namespace std;
#include <cryptopp/md5.h>

string base_file = "base.txt";
string file_error = "error.txt";

std::string MD(std::string sah);
void errors(std::string error, std::string name, std::string login = "NONE");
void filesend(int work_sock, string filename);
int autorized(int work_sock, string file_name, string file_error);

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

