#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define DEFAULT_PORT "21"
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib") //yêu cầu trình biên dịch link chương trình với thư viện Ws2_32.lib

int main(){
    //1. Khởi tạo winsock
    WSADATA wsaData; // Biến lưu cấu hình Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData); //Khởi tạo Winsock 
    if(iResult != 0){
        std::cout << "WSAStarup failed: " << iResult << std::endl;
        return 1;
    }

    //2. Cấu hình và lấy địa chỉ server
    struct addrinfo *result = NULL;
    struct addrinfo  *ptr = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints)); // Xóa sạch bộ nớ để tránh giá trị rác

    hints.ai_family = AF_UNSPEC; // Cho phép địa chỉ IPv4 và IPv6
    hints.ai_socktype = SOCK_STREAM; // Yêu cầu socket kiểu TCP
    hints.ai_protocol = IPPROTO_TCP; // Yêu cầu socket dùng TCP protocol

    iResult = getaddrinfo("ftp.example.com", DEFAULT_PORT, &hints, &result); // lấy địa chỉ lưu vào result
    if(iResult != 0){
        std::cout << "Get address infomation false: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    //3. Tạo socket
    ptr = result;
    SOCKET ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if(ConnectSocket == INVALID_SOCKET){
        std::cout << "Error at socket: " << GetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    
    //4. Kết nối với server
    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if(iResult == SOCKET_ERROR){
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if(ConnectSocket == INVALID_SOCKET){
        std::cout << "Error at socket: " << GetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    return 0;
}