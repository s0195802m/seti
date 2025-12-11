#include <iostream>
#include <fstream>
#include <windows.h>
#include <cstring>
using namespace std;

struct Student {
    char name[32];
    int height;
    int weight;
};

long getSize(const char* fname) {
    ifstream f(fname, ios::binary | ios::ate);
    if (!f) return 0;
    return (long)f.tellg();
}

int main() {
    setlocale(LC_ALL, "rus");

    const char* fileRequest = "C:/temp/f1.bin";
    const char* fileAnswer = "C:/temp/f2.bin";

    cout << "Клиент запущен..." << endl;

    while (true) {
        Student s{};

        cout << "\nВведите имя: ";
        cin >> s.name;
        cout << "Введите рост: ";
        cin >> s.height;
        cout << "Введите вес: ";
        cin >> s.weight;

        ofstream out(fileRequest, ios::binary | ios::app);
        out.write((char*)&s, sizeof(Student));
        out.close();

        cout << "Запрос отправлен.\nОжидание ответа...\n";

        long prevSize = getSize(fileAnswer);

        while (true) {
            Sleep(500);
            long currSize = getSize(fileAnswer);
            if (currSize > prevSize) {
                ifstream in(fileAnswer, ios::binary);
                in.seekg(prevSize);

                Student ans{};
                in.read((char*)&ans, sizeof(Student));
                in.close();

                cout << "Ответ сервера: " << ans.name << endl;
                break;
            }
        }
    }

    return 0;


    server 
        #include <iostream>
#include <fstream>
#include <windows.h>
#include <cstring>
using namespace std;

struct Student {
    char name[32];
    int height;
    int weight;
};

int main() {
    setlocale(LC_ALL, "rus");

    const char* fileRequest = "C:/temp/f1.bin";
    const char* fileAnswer = "C:/temp/f2.bin";

    cout << "Сервер запущен..." << endl;

    long prevSize = 0;

    while (true) {
        ifstream in(fileRequest, ios::binary | ios::ate);
        if (!in) {
            Sleep(500);
            continue;
        }

        long currSize = (long)in.tellg();

        if (currSize > prevSize) {
            in.seekg(prevSize);

            Student s;
            in.read((char*)&s, sizeof(Student));
            prevSize = currSize;
            in.close();

            double ideal = (s.height - 100) * 0.9;
            const char* result;

            if (s.weight > ideal + 5)
                result = "Избыточный вес";
            else if (s.weight < ideal - 5)
                result = "Недостаток веса";
            else
                result = "Норма";

            cout << "Получен запрос: " << s.name << endl;

            Student answer{};
            strcpy_s(answer.name, sizeof(answer.name),result);

            ofstream out(fileAnswer, ios::binary | ios::app);
            out.write((char*)&answer, sizeof(Student));
            out.close();
        }

        Sleep(500);
    }

    return 0;
}
}
