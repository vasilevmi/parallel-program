#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>

int main(int argc, char const* argv[])
{
    if (argc < 2)
    {
        std::cout << "Введите название файла для проверки" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]); // Открываем файл для чтения
    if (!file.is_open())
    {
        std::cerr << "Невозможно открыть файл." << std::endl;
        return 1;
    }

    int all = 0;
    int accepted = 0;
    std::string line;
    while (std::getline(file, line)) // Построчно считываем файл
    {
        std::istringstream iss(line); // Создаем поток для текущей строки
        std::vector<std::string> tokens;// Вектор для хранения токенов (слов) из строки

        std::string token;
        while (iss >> token) // Разбиваем строку на токены
        {
            tokens.push_back(token);
        }

        // Выводим полученные токены
        if (tokens.size() >= 3) // Проверяем, что в строке есть минимум три токена
        {
            double expected_result = std::stod(tokens.back());
            double actual_result = 0.0;

            if (tokens[0] == "pow" && tokens.size() == 4)
            {
                double base = std::stod(tokens[1]);
                double exponent = std::stod(tokens[2]);
                actual_result = std::pow(base, exponent);
            }
            else if (tokens[0] == "sin" && tokens.size() == 3)
            {
                double arg = std::stod(tokens[1]);
                actual_result = std::sin(arg);
            }
            else if (tokens[0] == "sqrt" && tokens.size() == 3)
            {
                double arg = std::stod(tokens[1]);
                actual_result = std::sqrt(arg);
            }

            std::cout << tokens[0] << " " << tokens[1] << " Expected: " << expected_result << " Actual: " << actual_result << std::endl;

            if (std::abs(expected_result - actual_result) < 10)
            {
                accepted++;
            }
            all++;
        }
    }

    std::cout << "Файл " << argv[1] << " acc = " << static_cast<double>(accepted) / static_cast<double>(all) << std::endl;

    file.close(); // Закрываем файл

    return 0;
}
