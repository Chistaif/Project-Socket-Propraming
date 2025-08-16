//lenh compile: g++ -std=c++17 -o clamav_agent clamav_agent.cpp scanwithclamav.cpp -lws2_32 -lstdc++fs
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <atomic>
#include <csignal>
#include "scanwithclamav.h"

#define DEFAULT_PORT "3310"
#define DEFAULT_IP "127.0.0.1"

#pragma comment(lib, "Ws2_32.lib")

// Biến global để điều khiển việc chạy server
std::atomic<bool> serverRunning(true);
SOCKET gListenSocket = INVALID_SOCKET;

// Bắt sự kiện Ctrl+C hoặc đóng console
BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if(signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT)
    {
        std::cout << "\n[INFO] Stopping server...\n";
        serverRunning = false;

        // Đóng socket listen để unblock accept()
        if(gListenSocket != INVALID_SOCKET)
        {
            closesocket(gListenSocket);
            gListenSocket = INVALID_SOCKET;
        }

        WSACleanup();
        return TRUE; // Đã xử lý
    }
    return FALSE;
}

std::string recvLine(SOCKET sock)
{   
    // Chia nhỏ header thành nhiều kí tự rồi lưu vào line
    std::string line;
    char ch; 
    int ret; // Biến lưu số byte
    while((ret = recv(sock, &ch, 1, 0)) > 0){
        if(ch == '\n') break;
        if(ch != '\r') line += ch;
    }
    return (ret <= 0) ? "" : line;  
}

int main()
{
    // Đăng ký handler Ctrl+C
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        std::cerr << "[ERROR] Could not set control handler\n";
        return 1;
    }

    // khoi tao winsock version 2.2
    int error;
    WSADATA wsadata;
    error = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (error != 0)
    {
        std::cerr << "[ERROR] WSAStartup() Failed: " << error << std::endl;
        return 1;
    }
    std::cout << "[INFO] Winsock initialized. Status: " << wsadata.szSystemStatus << std::endl;


    // luu dia chi ip va port vao result
    addrinfo hints, *result = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    std::string IP, PORT;
    std::cout << "ENTER SERVER'S IP ADDRESS: "; std::cin >> IP;
    std::cout << "ENTER SERVER'S PORT: "; std::cin >> PORT;

    error = getaddrinfo(IP.c_str(), PORT.c_str(), &hints, &result);
    if (error != 0)
    {
        std::cerr << "[ERROR] getaddrinfor() Failed: " << error << std::endl;
        WSACleanup();
        return 1;
    }
    std::cout << "[INFO] getaddrinfor() if OK!\n";
    

    // tao socket lang nghe
    gListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (gListenSocket == INVALID_SOCKET)
    {
        std::cerr << "[ERROR] socket() Failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    std::cout << "[INFO] socket() OK\n";

    // rang buoc voi socket lang nghe
    error = bind(gListenSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    if (error == SOCKET_ERROR)
    {
        std::cerr << "[ERROR] bind() Failed: " << error << std::endl;
        closesocket(gListenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "[INFO] bind() OK\n";

    // lang nghe yeu cau dc gui den
    error = listen(gListenSocket, SOMAXCONN);
    if (error == SOCKET_ERROR)
    {
        std::cerr << "[ERROR] listen() Failed: " << error << std::endl;
        closesocket(gListenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "[INFO] listen() OK\n";

    while(serverRunning)
    {
        std::cout << "[INFO] Waiting for client...\n";

        // dong y ket noi voi client
        SOCKET clientSocket = INVALID_SOCKET;
        clientSocket = accept(gListenSocket, NULL, NULL);
        if (!serverRunning) break; // Nếu đang tắt server, thoát

        if (clientSocket == INVALID_SOCKET)
        {
            int err = WSAGetLastError();
            if(!serverRunning || err == WSAENOTSOCK || err == WSAESHUTDOWN)
            {
                std::cout << "[INFO] Server socket closed, stopping accept loop.\n";
                break;
            }
            else
            {
                std::cerr << "[ERROR] accept() Failed: " << err << std::endl;
                continue;
            }
        }
        std::cout << "[INFO] Client connected\n";

        // Thiết lập timeout
        DWORD timeout = 30000;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

        std::cout << "Client connected\n";

        bool shouldClose = false;
        while(!shouldClose && serverRunning)
        {
            //nhan yeu cau
            //khi client gui lenh put / mput thi se gui qua ClamAvAgent truoc de quet virus
            //sau do ClamAV se tra ve OK/INFECTED theo dung thu tu file da gui
            std::string header = recvLine(clientSocket); // Đảm bảo nhận đủ header từ client
            if(header.empty())
            {
                std::cout << "[INFO] Client disconnected or error reading header.\n";
                break;
            }

            if(std::string(header) == "quit")
            {
                shouldClose = true;
                continue;
            }

            ScanWithClamav scanner;
            std::string result;

            if(scanner.ExistClamAV())
            {
                result = scanner.HandleCommand(header, clientSocket);
            }
            else
            {
                result = "Error! Please check: ClamAV is installed | ClamAV in PATH | Permission accept ?";
            }

            //gửi kết quả về client
            if (send(clientSocket, result.c_str(), result.size(), 0) == SOCKET_ERROR)
            {
                std::cerr << "[ERROR] send() failed: " << WSAGetLastError() << std::endl;
                shouldClose = true;
                continue;
            }
        }

        shutdown(clientSocket, SD_SEND);  // Gửi FIN để thông báo ngắt kết nối
        closesocket(clientSocket);
        std::cout << "[INFO] Client connection closed.\n";
    }

    if(gListenSocket != INVALID_SOCKET)
    {
        closesocket(gListenSocket);
    }
    WSACleanup();
    std::cout << "[INFO] Server stopped cleanly.\n";

    return 0;
}
