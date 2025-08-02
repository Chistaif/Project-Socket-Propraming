//lenh compile: g++ -std=c++17 -o clamav_agent clamav_agent.cpp scanwithclamav.cpp -lws2_32 -lstdc++fs
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>

#include "scanwithclamav.h"

#define DEFAULT_PORT "3310"
#define DEFAULT_IP "127.0.0.1"

#pragma comment(lib, "Ws2_32.lib")

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
    // khoi tao winsock version 2.2
    int error;
    WSADATA wsadata;
    error = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (error != 0)
    {
        std::cerr << "WSAStartup() Failed: " << error << std::endl;
        return 1;
    }
    else
    {
        std::cout << "The WinSock dll found!\n"
                  << "The status: " << wsadata.szSystemStatus << std::endl;
    }

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
        std::cerr << "getaddrinfor() Failed: " << error << std::endl;
        WSACleanup();
        return 1;
    }
    else
    {
        std::cout << "getaddrinfor() if OK!\n";
    }

    // tao socket lang nghe
    SOCKET listenSocket = INVALID_SOCKET;
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET)
    {
        std::cerr << "socket() Failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    else
    {
        std::cout << "socket() is OK!\n";
    }

    // rang buoc voi socket lang nghe
    error = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (error == SOCKET_ERROR)
    {
        std::cerr << "bind() Failed: " << error << std::endl;
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    else
    {
        std::cout << "bind() is OK!\n";
        freeaddrinfo(result);
    }

    // lang nghe yeu cau dc gui den
    error = listen(listenSocket, SOMAXCONN);
    if (error == SOCKET_ERROR)
    {
        std::cerr << "listen() Failed: " << error << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    else
    {
        std::cout << "listen() is OK!\n";
    }

    while (true)
    {
        // dong y ket noi voi client
        SOCKET clientSocket = INVALID_SOCKET;
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "accept() Failed: " << WSAGetLastError() << std::endl;
            continue;
        }
        else
        {
            std::cout << "accept() is OK!\n";
        }

        // Thiết lập timeout
        DWORD timeout = 30000;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

        std::cout << "Client connected\n";

        bool shouldClose = false;
        while(true)
        {
            //nhan yeu cau
            //khi client gui lenh put / mput thi se gui qua ClamAvAgent truoc de quet virus
            //sau do ClamAV se tra ve OK/INFECTED theo dung thu tu file da gui
            std::string header = recvLine(clientSocket); // Đảm bảo nhận đủ header từ client
            if(header.empty())
            {
                std::cout << "Client disconnected or error reading header.\n";
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
                std::cerr << "send() failed: " << WSAGetLastError() << std::endl;
                shouldClose = true;
                continue;
            }
        }

        closesocket(clientSocket);
        std::cout << "Connection closing ...\n";
    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}
