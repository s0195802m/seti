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
}