#include "scanwithclamav.h"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <cctype>
#include <vector>
#include <filesystem>


//-----------------------------check ClamAV was installed-------------------------------------
bool ScanWithClamav::ExistClamAV()
{
    int ret = system("clamscan --version");
    if(!ret)
    {
        return true;
    }
    return false;
}

//-------------------------------save temp file------------------------------
std::string token()
{
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    for (int i = 0; i < 5; ++i) {
        result += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return result;
}


std::string ScanWithClamav::SaveFile(const std::string &fileName, const int &fileSize, SOCKET clientSocket)
{
    std::cout << "[DEBUG] File size: " << fileName << " (" << fileSize << " bytes)" << std::endl;

    //kiem tra kich thuoc file duoi 100MB
    if(fileSize <= 0 || fileSize > MAX_FILESIZE)
    {
        throw std::runtime_error("Invalid file size!");
    }

    //tao duong dan den file luu tam ko trung lap
    //luu ý: -std=c++17 -lstdc++fs
    std::filesystem::path filePath = std::filesystem::temp_directory_path() / ("clamav_" + token() + fileName);

    //nhan file va luu vao file qua duong dan
    std::ofstream file(filePath, std::ios::binary);
    if(!file)
    {
        throw std::runtime_error("Can not open file!");
    }

    char buffer[4096];
    int byteRecv= 0;
    while(byteRecv < fileSize)
    {
        //chia nho du lieu de nhan dam bao bo dem < 4MB
        int bytes = recv(clientSocket, buffer, std::min(4096, fileSize - byteRecv), 0);
        if(bytes <= 0)
        {
            break;
        }
        file.write(buffer, bytes);
        byteRecv += bytes;
    }
    std::cout << "[DEBUG] Receiving file: " << fileName << " (" << byteRecv << " bytes)" << std::endl;

    file.close();

    //kiem tra co bi mat du lieu tren duong truyen ko
    if(byteRecv != fileSize)
    {
        std::filesystem::remove(filePath);
        throw std::runtime_error("File transfer failed!");
    }

    return filePath.string();
}


//------------------------------scan logic------------------------------------
std::string ScanWithClamav::ScanFile(const std::string &filePath)
{
    //nhap lenh len command line de scan
    std::string command = "clamscan --no-summary " + filePath;
    FILE* pipe = popen(command.c_str(), "r");
    if(!pipe)
    {
        return "ERROR: Scan Failed!";
    }

    //nhan ket qua
    char buffer[512];
    std::string res;
    while(fgets(buffer, sizeof(buffer), pipe))
    {
        res += buffer;
    }
    pclose(pipe);

    if(res.find("OK") != std::string::npos)
    {
        return "OK";
    }
    else if(res.find("FOUND") != std::string::npos)
    {
        return "INFECTED";
    }
    else
    {
        return "ERROR: Scan Failed!";
    }
}


//-------------------------------scan bussiness code-----------------------------------
std::string ScanWithClamav::HandleScan(const std::string &fileName, const int &fileSize, SOCKET clientSocket)
{
    std::string res;
    try
    {
        std::string path = SaveFile(fileName, fileSize, clientSocket);
        res = ScanFile(path);
        std::filesystem::remove(path);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return "INFECTED";
    }

    if(res == "OK")
    {
        return "OK";
    }
    else
    {
        return "INFECTED";
    }
}


//-------------------------------handle command(put | mput) from client--------------------------------
std::string ScanWithClamav::HandleCommand(std::string command, SOCKET clientSocket)
{
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    //protocol: put <fileName> <fileSize> | ex: put hello.txt 1
    if(cmd == "put")
    {
        std::string fileName, fileSize;
        iss >> fileName >> fileSize;
        return HandleScan(fileName, stoi(fileSize), clientSocket);
    }
    
    // Lệnh mput bị lỗi, chuyển lượng lớn data sẽ bị trộn dẫn đến đọc data bị thiếu byte 
    //protocol: mput <number Of File> <fileName1> <fileSize1> <fileName2> <fileSize2> ect...
    else if(cmd == "mput")
    {
        std::string num;
        iss >> num;
        int n = stoi(num); // number Of File

        std::vector<std::string> fileName(n); // vector lưu tên file
        std::vector<int> fileSize(n); // vector lưu kích thước của từng file
        
        // Lưu từng cặp fileName và fileSize 
        for(int i = 0 ; i < n ; i++)
        {
            std::string size;
            iss >> fileName[i] >> size;
            fileSize[i] = stoi(size);
        }

        std::string res;
        for(int i = 0 ; i < n ; i++){
            try
            {
                std::string path = SaveFile(fileName[i], fileSize[i], clientSocket);
                std::string result = ScanFile(path);
                std::filesystem::remove(path);
                res += (result == "OK") ? "OK" : "INFECTED";
            }
            catch(const std::exception& e)
            {
                std::cerr << "Error in mput file " << fileName[i] << ": " << e.what() << "\n";
                res += "INFECTED";
            }
        }
        return res;
    }
    return "INVALID_COMMAND";
}