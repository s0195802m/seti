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





5 Задание
Сервер
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>


#pragma comment(lib, "ws2_32.lib")

#define PORT 8888
#define BUFFER_SIZE 1024

using namespace std;

// Глобальные переменные
vector<SOCKET> clientSockets;
vector<string> clientNames;
mutex clientsMutex;

// Функция для отправки сообщения всем клиентам
void SendToAll(const string& message, SOCKET sender = INVALID_SOCKET) {
    lock_guard<mutex> lock(clientsMutex);

    cout << message << endl;

    for (size_t i = 0; i < clientSockets.size(); i++) {
        if (clientSockets[i] != INVALID_SOCKET && clientSockets[i] != sender) {
            send(clientSockets[i], message.c_str(), message.length(), 0);
        }
    }
}

// Функция для отправки сообщения одному клиенту
void SendToOne(SOCKET socket, const string& message) {
    if (socket != INVALID_SOCKET) {
        send(socket, message.c_str(), message.length(), 0);
    }
}

// Функция для поиска клиента по нику
int FindClientByName(const string& name) {
    lock_guard<mutex> lock(clientsMutex);
    for (size_t i = 0; i < clientNames.size(); i++) {
        if (clientNames[i] == name && clientSockets[i] != INVALID_SOCKET) {
            return (int)i;
        }
    }
    return -1;
}

// Функция обработки клиента
void ClientThread(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    string clientName = "Неизвестный";

    // Получаем имя клиента
    int bytes = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

    if (bytes > 0) {
        buffer[bytes] = '\0';
        clientName = buffer;

        // Добавляем клиента в списки
        {
            lock_guard<mutex> lock(clientsMutex);
            clientSockets.push_back(clientSocket);
            clientNames.push_back(clientName);
        }

        // Уведомляем о подключении
        string msg = clientName + " присоединился к чату!";
        SendToAll(msg, clientSocket);

        // Отправляем приветствие
        string welcome = "Добро пожаловать, " + clientName + "!\n";
        welcome += "Команды:\n";
        welcome += "  /private [ник] [сообщение] - личное сообщение\n";
        welcome += "  /quit - выход\n";
        welcome += "  /list - список пользователей в сети\n";
        SendToOne(clientSocket, welcome);
    }

    // Основной цикл обработки сообщений
    bool running = true;
    while (running) {
        bytes = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0) {
            running = false;
            break;
        }

        buffer[bytes] = '\0';
        string message = buffer;

        if (message == "/quit") {
            running = false;
        }


        else if (message.find("/private ") == 0) {
            // Упрощённая версия - всё после /private обрабатываем
            string rest = message.substr(9); // Всё после "/private "

            // Ищем первый пробел для разделения ника и сообщения
            size_t spacePos = rest.find(' ');
            if (spacePos != string::npos) {
                string targetName = rest.substr(0, spacePos);
                string privateMsg = rest.substr(spacePos + 1);

                int targetIndex = FindClientByName(targetName);
                if (targetIndex != -1 && clientSockets[targetIndex] != clientSocket) {
                    string msg = "Лично от " + clientName + ": " + privateMsg;
                    SendToOne(clientSockets[targetIndex], msg);
                    SendToOne(clientSocket, "Сообщение отправлено!");
                }
                else {
                    SendToOne(clientSocket, "Пользователь не найден!");
                }
            }
            else {
                SendToOne(clientSocket, "Неверный формат: /private [ник] [сообщение]");
            }
        } 
задание 5
Сервер

        else if (message == "/list") {
            string userList = "Подключённые пользователи:\n";
            {
                lock_guard<mutex> lock(clientsMutex);
                for (size_t i = 0; i < clientSockets.size(); i++) {
                    if (clientSockets[i] != INVALID_SOCKET) {
                        userList += "  - " + clientNames[i] + "\n";
                    }
                }
            }
            SendToOne(clientSocket, userList);
        }
        else {
            // Публичное сообщение
            string publicMsg = clientName + ": " + message;
            SendToAll(publicMsg, clientSocket);
        }
    }

    // Удаление клиента
    string disconnectMsg;
    {
        lock_guard<mutex> lock(clientsMutex);
        for (size_t i = 0; i < clientSockets.size(); i++) {
            if (clientSockets[i] == clientSocket) {
                closesocket(clientSockets[i]);

                // Сохраняем сообщение, но НЕ отправляем здесь
                disconnectMsg = clientNames[i] + " покинул чат";

                // Удаляем клиента из векторов
                clientSockets.erase(clientSockets.begin() + i);
                clientNames.erase(clientNames.begin() + i);

                break;
            }
        }
    }

    // Теперь отправляем сообщение вне блокировки мьютекса
    if (!disconnectMsg.empty()) {
        SendToAll(disconnectMsg);
    }
}

int main() {
    setlocale(LC_ALL, "rus");


    // Инициализация Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "Ошибка Winsock" << endl;
        return 1;
    }

    // Создание сокета
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Ошибка создания сокета" << endl;
        return 1;
    }

    // Настройка адреса
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Привязка сокета
    if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cout << "Ошибка привязки сокета" << endl;
        closesocket(serverSocket);
        return 1;
    }

    // Прослушивание порта
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Ошибка прослушивания порта" << endl;
        closesocket(serverSocket);
        return 1;
    }

    cout << "Сервер чата запущен на порту " << PORT << endl;
    cout << "Ожидание подключений..." << endl;
    cout << "==============================" << endl;

    // Основной цикл сервера
    while (true) {
        sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);

        // Принимаем новое подключение
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

        if (clientSocket != INVALID_SOCKET) {
            // Запускаем новый поток для клиента
            thread(ClientThread, clientSocket).detach();
            cout << "Новый клиент подключился!" << endl;
        }
    }

    // Очистка (никогда не выполнится, но для порядка)
    closesocket(serverSocket);
    WSACleanup();
    return 0;
} 
   клиент 
Задание 5
Клиент:

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>


#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8888
#define BUFFER_SIZE 1024

using namespace std;

SOCKET clientSocket;
bool running = true;

void ReceiveMessages() {
    char buffer[BUFFER_SIZE];

    while (running) {
        int bytes = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
              cout << endl << buffer << endl;
        }
        else if (bytes == 0) {
            cout << "Сервер отключился" << endl;
            running = false;
            break;
        }
    }
}

int main() {
    setlocale(LC_ALL, "rus");
   
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "Ошибка Winsock" << endl;
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Ошибка создания сокета" << endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    cout << "Подключение к серверу..." << endl;
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Не удалось подключиться" << endl;
        return 1;
    }

    string nickname;
    cout << "Введите ваш ник: ";
    getline(cin, nickname);

    send(clientSocket, nickname.c_str(), nickname.length(), 0);

    

    thread receiver(ReceiveMessages);

    string message;
    while (running) {
        cout << "> ";
        getline(cin, message);

        if (message == "/quit") {
            send(clientSocket, "/quit", 5, 0);
            running = false;
            break;
        }

        if (!message.empty()) {
            send(clientSocket, message.c_str(), message.length(), 0);
        }
    }

    receiver.join();
    closesocket(clientSocket);
    WSACleanup();

    cout << "Нажмите Enter для выхода...";
    cin.get();
    return 0;
}


