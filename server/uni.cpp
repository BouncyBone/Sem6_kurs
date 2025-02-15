#include <UnitTest++/UnitTest++.h>
#include "head.h"

using namespace std;
Server Server;

SUITE(BindingTests)
{
    TEST(CorrectPort) {
        // Тест проверяет корректное создание сокета
        string file_file = "base.txt";
        int port = 33333;
        string error_file = "error.txt";
        Server.self_addr(error_file, port);
        CHECK(true);
    }
    TEST(BadPort) {
        // Тест проверяет, что исключение выбрасывается при попытке использовать некорректный порт
        string error_file = "error.txt";
        string base_file = "base.txt";
        int port = 58;
        CHECK_THROW(Server.self_addr(error_file, port), BindingError);
    }
}

SUITE(TestHashGeneration)
{
    TEST(SamePass) {
        // Тест проверяет сравнение хешей для одинаковых паролей
        string pass_1 = "P@ssW0rd";
        string pass_2 = "P@ssW0rd";
        CHECK_EQUAL(true, MD(pass_1) == MD(pass_2));
    }
    TEST(DiffPass) {
        // Тест проверяет, что разные пароли имеют разные хеши
        string pass_1 = "P@ssW0rd";
        string pass_2 = "WRONGPASS";
        CHECK_EQUAL(false, MD(pass_1) == MD(pass_2));
    }
}

int main(int argc, char **argv)
{
    return UnitTest::RunAllTests();
}