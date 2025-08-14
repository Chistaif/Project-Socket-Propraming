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
    int ret = system("clamdscan --version");
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


std::string ScanWithClamav::SaveFile(const std::string &filePath, const int &fileSize, SOCKET clientSocket)
{
    try
    {
        // 1. Trích xuất chỉ tên file từ đường dẫn đầy đủ
        std::filesystem::path pathObj(filePath);
        std::string fileName = pathObj.filename().string(); // Lấy tên file cuối cùng
        std::cout << "Receiving file from path: " << filePath << std::endl; // Ghi log
        std::cout << "Extracted filename: " << fileName << std::endl;
        std::cout << "Filesize: " << fileSize << std::endl;
        
        // 2. Kiểm tra kích thước file
        if(fileSize <= 0 || fileSize > MAX_FILESIZE)
        {
            throw std::runtime_error("Invalid file size: " + std::to_string(fileSize));
        }

        // 3. Tạo tên file tạm an toàn
        std::string safeName;
        for(char c : fileName)
        {
            if(isalnum(c) || c == '.' || c == '-' || c == '_')
            {
                safeName += c;
            }
        }
        if(safeName.empty()) safeName = "unnamed";

        // 4. Tạo đường dẫn file tạm
        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        std::filesystem::path savePath = tempDir / ("clamav_" + token() + "_" + safeName);

        // 5. Đảm bảo thư mục tạm tồn tại
        if(!std::filesystem::exists(tempDir))
        {
            std::filesystem::create_directories(tempDir);
        }

        // 6. Mở file để ghi
        std::ofstream file(savePath, std::ios::binary);
        if(!file.is_open())
        {
            throw std::runtime_error("Cannot open file for writing: " + savePath.string());
        }

        // 7. Nhận dữ liệu từ client
        char buffer[4096];
        int byteRecv = 0;
        while(byteRecv < fileSize)
        {
            int bytes = recv(clientSocket, buffer, std::min(4096, fileSize - byteRecv), 0);
            if(bytes <= 0)
            {
                throw std::runtime_error("Connection error during transfer");
            }
            file.write(buffer, bytes);
            byteRecv += bytes;
        }

        file.close();

        // 8. Kiểm tra dữ liệu nhận đủ
        if(byteRecv != fileSize)
        {
            std::filesystem::remove(savePath);
            throw std::runtime_error("File transfer incomplete");
        }

        return savePath.string();
    } 
    catch(const std::exception& e) {
        std::cerr << "[ERROR] SaveFile failed: " << e.what() << std::endl;
        throw;
    }
}


//------------------------------scan logic------------------------------------
std::string ScanWithClamav::ScanFile(const std::string &filePath)
{
    //nhap lenh len command line de scan
    std::string command = "clamdscan --no-summary " + filePath;
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
std::string ScanWithClamav::HandleScan(const std::string &filePath, const int &fileSize, SOCKET clientSocket)
{
    std::string res;
    try
    {
        std::string path = SaveFile(filePath, fileSize, clientSocket);
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
        std::string filePath, fileSize;
        iss >> filePath >> fileSize;
        return HandleScan(filePath, stoi(fileSize), clientSocket);
    }
    
    // Lệnh mput bị lỗi, chuyển lượng lớn data sẽ bị trộn dẫn đến đọc data bị thiếu byte 
    //protocol: mput <number Of File> <fileName1> <fileSize1> <fileName2> <fileSize2> ect...
    else if(cmd == "mput")
    {
        std::string num;
        iss >> num;
        int n = stoi(num); // number Of File

        std::vector<std::string> filePath(n); // vector lưu tên file
        std::vector<int> fileSize(n); // vector lưu kích thước của từng file
        
        // Lưu từng cặp filePath và fileSize 
        for(int i = 0 ; i < n ; i++)
        {
            std::string size;
            iss >> filePath[i] >> size;
            fileSize[i] = stoi(size);
        }

        std::string res;
        for(int i = 0 ; i < n ; i++){
            try
            {
                std::string path = SaveFile(filePath[i], fileSize[i], clientSocket);
                std::string result = ScanFile(path);
                std::filesystem::remove(path);
                res += (result == "OK") ? "OK" : "INFECTED";
            }
            catch(const std::exception& e)
            {
                std::cerr << "Error in mput file " << filePath[i] << ": " << e.what() << "\n";
                res += "INFECTED";
            }
        }
        return res;
    }
    return "INVALID_COMMAND";
}