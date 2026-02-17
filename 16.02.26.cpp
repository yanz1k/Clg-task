#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <cstdio>
#include <Windows.h>

using namespace std;
/**
    @brief Простая функция для HTTP GET запроса через системные вызовы (для Windows)
    @param url Ссылка запроса
    @param headers Вектор с заголовками для запроса
    @return Строка с ответом от сервера, или пустая строка
    @note Эта функция работает только на windows OC
*/
string httpGet(const string& url, const vector<string>& headers) {
    string command = "C:/Windows/System32/curl.exe -s -H \"";

    // Добавляем заголовки
    for (size_t i = 0; i < headers.size(); i++) {
        command += headers[i];
        if (i < headers.size() - 1) {
            command += "\" -H \"";
        }
    }

    command += "\" \"" + url + "\"";

    // Выполняем команду и читаем результат (для Windows используем _popen)
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) return "";

    char buffer[128];
    string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    _pclose(pipe);

    return result;
}
/**
    @brief Простой парсер JSON
    @details Парсит только json код для расписания, не работает с дригими вариациями
*/
class SimpleJsonParser {
private:
    string json; /// Основаня json строка
    size_t pos = 0; /// Текущая позиция для кода
    /**
            @brief Пропускает лишние символы
    */
    void skipWhitespace() {
        while (pos < json.length() && isspace(static_cast<unsigned char>(json[pos]))) pos++;
    }
    /**
        @brief парсит строчку с экранированными символами
        @return Нормальная строка для дальшейшей обработки
    */
    string parseString() {
        if (json[pos] != '"') return "";
        pos++; // пропускаем открывающую кавычку

        string result;
        while (pos < json.length() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.length()) {
                // Обработка экранированных символов
                pos++;
                switch (json[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default: result += json[pos];
                }
            }
            else {
                result += json[pos];
            }
            pos++;
        }

        if (json[pos] == '"') pos++; // пропускаем закрывающую кавычку
        return result;
    }

public:
    /**
        @brief Конструктор класса, принимает ответ от сервера
    */
    SimpleJsonParser(const string& str) : json(str) {}

    /**
        @brief Парсим массив объектов и извлекаем значения subject_name
        @return словарь с отсортированными данными по расписанию
    */
    map<string, int> countSubjects() {
        map<string, int> subjects;
        skipWhitespace();
        if (json[pos] != '[') return subjects; // ожидаем начало массива
        pos++; // пропускаем '['

        while (pos < json.length()) {
            skipWhitespace();
            if (json[pos] == ']') break; // конец массива
            if (json[pos] == ',') {
                pos++;
                continue;
            }

            // Парсим объект
            if (json[pos] == '{') {
                pos++; // пропускаем '{'

                string subjectName;

                while (pos < json.length() && json[pos] != '}') {
                    skipWhitespace();

                    // Парсим ключ
                    string key = parseString();
                    skipWhitespace();

                    if (json[pos] != ':') {
                        // Пропускаем до следующей запятой или конца объекта
                        while (pos < json.length() && json[pos] != ',' && json[pos] != '}') pos++;
                        if (json[pos] == ',') pos++;
                        continue;
                    }
                    pos++; // пропускаем ':'
                    skipWhitespace();

                    // Если это subject_name - сохраняем значение
                    if (key == "subject_name") {
                        if (json[pos] == '"') {
                            subjectName = parseString();
                        }
                        else {
                            // null значение
                            subjectName = "";
                        }
                    }
                    else {
                        // Пропускаем значение
                        if (json[pos] == '"') {
                            parseString();
                        }
                        else {
                            // Пропускаем число, true, false или null
                            while (pos < json.length() && json[pos] != ',' && json[pos] != '}') pos++;
                        }
                    }

                    skipWhitespace();
                    if (json[pos] == ',') pos++;
                }

                if (json[pos] == '}') pos++;

                // Добавляем subject в подсчет
                if (!subjectName.empty()) {
                    subjects[subjectName]++;
                }
            }

            skipWhitespace();
            if (json[pos] == ',') pos++;
        }

        return subjects;
    }

    /**
        @brief Получить сырой JSON для сохранения
        @return json код ответа сервера, без спец символов
    */
    string getRawJson() {
        return json;
    }
};

int test()
{
    return 1 + 1;
}
/**
    @brief Запуск программы, запрашивает bearer у пользователя и выводит результат по поиску на экран
    @note url и референс для headers меняются в коде

*/
int main() {
    //setlocale(0, "");
    SetConsoleOutputCP(CP_UTF8);
    string url = "https://msapi.top-academy.ru/api/v2/schedule/operations/get-month?date_filter=2026-02-16"; /// Ссылка определенный хедер на сайте
    vector<string> headers{ "referer: https://journal.top-academy.ru/" }; /// Вектор хедеров
    cout << "Give a Bearer: " << endl;
    string Bearer; /// Bearer у хедера на сайте
    cin >> Bearer;
    headers.push_back(("authorization: Bearer " + Bearer));
    cout << "Request in api... " << url << "..." << endl;
    string response = httpGet(url, headers); /// Результат поиска
    //cout << "Ответ: " << response << endl;
    SimpleJsonParser parser(response);
    map<string, int> themes = parser.countSubjects();/// @param map для сортировки результатов парсинга результата и последующего извлечения из него отсортированного ответа
    for (auto it = themes.begin(); it != themes.end(); ++it) {
        cout << it->first << ": " << it->second << endl;
    }
    return 0;
}
