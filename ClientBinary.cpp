#define _WINSOCK_DEPRECATED_NO_WARNINGS  // Чтобы inet_addr не ругался
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 8888
#define IP "127.0.0.1"
#define BUF_SIZE 1024

SOCKET sock;
bool running = true;

// Поток для приёма сообщений
void ReceiveThread() {
    char buf[BUF_SIZE];
    while (running) {
        int bytes = recv(sock, buf, BUF_SIZE - 1, 0);
        if (bytes > 0) {
            buf[bytes] = '\0';
            cout << buf << endl;
        }
        else {
            running = false;
        }
    }
}

int main() {
    setlocale(LC_ALL, "rus");

    // Инициализация Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "Ошибка WSAStartup\n";
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cout << "Ошибка сокета\n";
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP); // теперь не ругается

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cout << "Не удалось подключиться к серверу\n";
        return 1;
    }

    string name;
    cout << "Введите ник: ";
    getline(cin, name);

    send(sock, name.c_str(), (int)name.size(), 0);

    thread t(ReceiveThread); // запускаем поток для приёма сообщений

    string msg;
    while (running) {
        getline(cin, msg);
        if (!msg.empty()) send(sock, msg.c_str(), (int)msg.size(), 0);
        if (msg == "/quit") break;
    }

    t.join();
    closesocket(sock);
    WSACleanup();

    return 0;
} 
кдиент 
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 8888
#define BUF_SIZE 1024

vector<SOCKET> clients;
vector<string> names;
mutex mtx;

void SendAll(const string& msg, SOCKET skip = INVALID_SOCKET) {
    lock_guard<mutex> lock(mtx);
    for (auto client : clients) {
        if (client != skip) send(client, msg.c_str(), (int)msg.size(), 0);
    }
}

int FindUser(const string& name) {
    for (size_t i = 0; i < names.size(); i++)
        if (names[i] == name) return (int)i;
    return -1;
}

void ClientThread(SOCKET sock) {
    char buf[BUF_SIZE];
    int bytes;

    // Получаем ник
    bytes = recv(sock, buf, BUF_SIZE - 1, 0);
    if (bytes <= 0) { closesocket(sock); return; }
    buf[bytes] = '\0';
    string nickname = buf;

    {
        lock_guard<mutex> lock(mtx);
        clients.push_back(sock);
        names.push_back(nickname);
    }

    SendAll(nickname + " вошёл в чат", sock);

    while (true) {
        bytes = recv(sock, buf, BUF_SIZE - 1, 0);
        if (bytes <= 0) break;
        buf[bytes] = '\0';
        string msg = buf;

        if (msg == "/quit") break;

        if (msg.find("/private ") == 0) {
            string rest = msg.substr(9);
            size_t pos = rest.find(' ');
            if (pos != string::npos) {
                string to = rest.substr(0, pos);
                string text = rest.substr(pos + 1);
                int id = FindUser(to);
                if (id != -1) {
                    string out = "Лично от " + nickname + ": " + text;
                    send(clients[id], out.c_str(), (int)out.size(), 0);
                }
            }
        }
        else {
            SendAll(nickname + ": " + msg, sock);
        }
    }

    {
        lock_guard<mutex> lock(mtx);
        for (size_t i = 0; i < clients.size(); i++) {
            if (clients[i] == sock) {
                SendAll(names[i] + " вышел из чата");
                closesocket(clients[i]);
                clients.erase(clients.begin() + i);
                names.erase(names.begin() + i);
                break;
            }
        }
    }
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { cout << "WSAStartup error\n"; return 1; }

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) { cout << "Socket error\n"; return 1; }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) { cout << "Bind error\n"; return 1; }
    if (listen(server, SOMAXCONN) == SOCKET_ERROR) { cout << "Listen error\n"; return 1; }

    cout << "Server started on port " << PORT << endl;

    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        if (client != INVALID_SOCKET) {
            thread(ClientThread, client).detach();
        }
    }

    closesocket(server);
    WSACleanup();
    return 0;
}
