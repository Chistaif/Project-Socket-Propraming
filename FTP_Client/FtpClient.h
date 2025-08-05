#include <winsock2.h>
#include <ws2tcpip.h>
#include <filesystem>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <string>
#include <algorithm>
using namespace std;
namespace fs = std::filesystem;;

#pragma comment(lib, "ws2_32.lib")

class FtpClient{
    private:
        string serverIP;
        string serverPORT;

        bool prompt;
        bool isAcsiiMode = false;
        bool isPassive = true;

        SOCKET CtrlSocket;
        SOCKET ConnectToServer(const char* IP, const char* Port);
        SOCKET EnterPASVMode(string& IP, string& PORT);
        SOCKET EnterPORTMode();
    public:
        FtpClient(const string& IP, const string& PORT);
        ~FtpClient();
        void SetServerInfo(const string& ip, const string& port);

        bool isConnected();
        bool Connected();
        bool Login(const char* username, const char* password);
        bool Upload(const string& fileName);
        bool Download(const string& fileName);
        bool ScanWithClamAV(const string& fileName, vector<char> fileData);

        string GetCurrentDir();
        string SendCmd(const string& cmd);
        char GetRemoteType(const string& path);
        vector<char> ReadFile(const string& fileName);

        void List();        
        void mput(const vector<string>& files);
        void mget(const vector<string>& files);
        void togglePrompt();
        void SetTransferMode(bool ascii);
        void Disconnect();
        void putDir(const string& localPath, const string& remoteBase);
        void getDir(const string& localBase, const string& remotePath);
        void SetPassiveMode();
        void SetActiveMode();
};