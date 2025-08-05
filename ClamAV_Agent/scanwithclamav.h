#pragma once

#include <ws2tcpip.h>
#include <string>

#define MAX_FILESIZE 100 * 1024 * 1024 //100MB

class ScanWithClamav
{
private:
    //nhan va luu tam file, output ra duong dan den file luu tam
    std::string SaveFile(const std::string& fileName, const int& fileSize, SOCKET clientSocket);

    std::string ScanFile(const std::string& filePath);

public:
    ScanWithClamav() {};
    ~ScanWithClamav() {};

    bool ExistClamAV();

    //business code
    std::string HandleScan(const std::string& fileName, const int& fileSize, SOCKET clientSocket);

    std::string HandleCommand(std::string command, SOCKET clientSocket);
};