#include "FtpClient.h"

bool firstTime = true;
string IP, PORT;

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
using namespace std;

// Hàm tìm IP LAN thật (bỏ qua 127.x.x.x, 169.254.x.x, 192.168.56.x)
bool GetLocalLanIP(unsigned char ip[4]) {
    char hostname[256];
    if(gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR){
        cerr << "[ERROR] gethostname() failed: " << WSAGetLastError() << endl;
        return false;
    }

    hostent* he = gethostbyname(hostname);
    if(!he){
        cerr << "[ERROR] gethostbyname() failed\n";
        return false;
    }

    for(int i = 0; he->h_addr_list[i] != NULL; i++){
        struct in_addr* addr_in = (struct in_addr*)he->h_addr_list[i];
        unsigned char* cand = (unsigned char*)&addr_in->s_addr;

        // Debug in tất cả IP tìm được
        cout << "[DEBUG] Found IP: " 
             << (int)cand[0] << "."
             << (int)cand[1] << "."
             << (int)cand[2] << "."
             << (int)cand[3] << endl;

        // Bỏ qua loopback
        if(cand[0] == 127) continue;

        // Bỏ qua APIPA (169.254.x.x)
        if(cand[0] == 169 && cand[1] == 254) continue;

        // Bỏ qua VirtualBox (192.168.56.x)
        if(cand[0] == 192 && cand[1] == 168 && cand[2] == 56) continue;

        // Nếu qua hết filter thì chọn IP này
        memcpy(ip, cand, 4);
        return true;
    }

    return false; // Không tìm thấy IP phù hợp
}


FtpClient::FtpClient(const string& IP, const string& PORT){
    serverIP = IP; 
    serverPORT = PORT;
    CtrlSocket = INVALID_SOCKET;
    prompt = true;
}

FtpClient::~FtpClient(){
    if(CtrlSocket != INVALID_SOCKET) closesocket(CtrlSocket);
}

void FtpClient::SetServerInfo(const string &ip, const string &port)
{
    serverIP = ip;
    serverPORT = port;
}

SOCKET FtpClient::ConnectToServer(const char* IP, const char* PORT){
    addrinfo *result = nullptr; 
    addrinfo hints; 

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // Cho phép IPv4 hoặc IPv6
    hints.ai_socktype = SOCK_STREAM; // Giao thức TCP
    hints.ai_protocol = IPPROTO_TCP; // Chỉ định dùng TCP

    //1. Lấy thông tin địa chỉ socket từ IP và PORT, lưu vào result dựa trên cấu hình gợi ý từ hints 
    if(getaddrinfo(IP, PORT, &hints, &result) != 0) return INVALID_SOCKET;

    //2. Khởi tạo socket với thông số result
    SOCKET sock = INVALID_SOCKET;
    for(addrinfo* ptr = result; ptr != nullptr ; ptr = ptr->ai_next){
        sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(sock == INVALID_SOCKET) continue;

        //3. Kết nối socket đến server
        if(connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR){
            cout << "Connect failed to one addr, trying next...\n";
            closesocket(sock);
            sock = INVALID_SOCKET;
            continue;
        }

        break;// Kết nối thành công
    }

    freeaddrinfo(result);
    return sock;
}

SOCKET FtpClient::EnterPASVMode(string& IP, string& PORT){
    int iResult;
    char recvbuff[512];

    //1. Gửi lệnh PASV
    iResult = send(CtrlSocket, "PASV\r\n", 6, 0);
    if(iResult == SOCKET_ERROR){
        cout << "Send PASV failed: " << GetLastError() << endl;
        return INVALID_SOCKET;
    }

    //2. Nhận phản hồi sau khi gửi lệnh PASV 
    iResult = recv(CtrlSocket, recvbuff, sizeof(recvbuff) - 1, 0); // Nhận chuỗi phản hồi server
    if(iResult <= 0){
        cout << "Receive PASV response failed.\n";
        return INVALID_SOCKET;
    }
    recvbuff[iResult] = '\0';
    // cout << "PASV response: " << recvbuff;

    //3. Trích xuất IP và PORT từ phản hồi, có dạng "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)"
    int h1, h2, h3, h4, p1, p2;
    iResult = sscanf(recvbuff, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    if(iResult != 6){
        cout << "Cannot parse PASV response.\n";
        return INVALID_SOCKET;
    }

    //4. Nhập địa chỉ IP và PORT
    char IP_Address[32], PORT_Address[6];
    sprintf(IP_Address, "%d.%d.%d.%d", h1, h2, h3, h4);
    sprintf(PORT_Address, "%d", p1 * 256 + p2);
    
    //5. Cập nhật IP và PORT
    IP = string(IP_Address);
    PORT = string(PORT_Address);
    return ConnectToServer(IP.c_str(), PORT.c_str());
}

SOCKET FtpClient::EnterPORTMode() {
    // 1. Tạo socket listen
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listenSock == INVALID_SOCKET){
        cerr << "[ERROR] socket() failed: " << WSAGetLastError() << endl;
        return INVALID_SOCKET;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(50000); // cố định dễ debug

    if(bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR){
        cerr << "[ERROR] bind() failed: " << WSAGetLastError() << endl;
        closesocket(listenSock);
        return INVALID_SOCKET;
    }

    if(listen(listenSock, SOMAXCONN) == SOCKET_ERROR){
        cerr << "[ERROR] listen() failed: " << WSAGetLastError() << endl;
        closesocket(listenSock);
        return INVALID_SOCKET;
    }

    // 2. Lấy IP LAN thật (logic mình viết trước đó)
    unsigned char ip[4];
    if(!GetLocalLanIP(ip)){ 
        cerr << "[ERROR] no LAN IP found\n";
        closesocket(listenSock);
        return INVALID_SOCKET;
    }

    int p1 = 50000 / 256, p2 = 50000 % 256;
    char cmd[64];
    sprintf(cmd, "PORT %d,%d,%d,%d,%d,%d\r\n",
            ip[0], ip[1], ip[2], ip[3], p1, p2);

    string resp = SendCmd(cmd);
    if(resp.substr(0,3) != "200"){
        cerr << "[ERROR] Server rejected PORT\n";
        closesocket(listenSock);
        return INVALID_SOCKET;
    }

    cout << "[INFO] Waiting for FTP server (Active Mode)...\n";

    // 3. Accept kết nối từ server (giống clamav_agent accept client)
    SOCKET dataSock = accept(listenSock, NULL, NULL);
    if(dataSock == INVALID_SOCKET){
        cerr << "[ERROR] accept() failed: " << WSAGetLastError() << endl;
        closesocket(listenSock);
        return INVALID_SOCKET;
    }

    cout << "[INFO] FTP server connected back successfully\n";
    closesocket(listenSock); // đóng listen, chỉ giữ dataSock

    return dataSock;
}

vector<char> FtpClient::ReadFile(const string& fileName){
    ifstream inFile(fileName, ios::binary | ios::ate); // Đọc file binary và chuyển con trỏ về cuối file (hỗ trợ lấy kích thước)
    if(!inFile.is_open()){
        cout << "Open file error.\n";
        return {};
    }

    streamsize sizeFile = inFile.tellg(); // Lấy kích thước của inFile
    inFile.seekg(0); // Chuyển con trỏ về đầu file
    vector<char> buffer(sizeFile);

    if(!inFile.read(buffer.data(), sizeFile)){ // Đọc sizeFile byte dữ liệu từ inFile và lưu vào buffer
        cout << "Cannot read file.\n";
        return {};
    } 
    return buffer;
}

string FtpClient::GetCurrentDir(){
    string resp = SendCmd("PWD\r\n");
    if(resp.substr(0, 3) != "257"){
        cout << "PWD failed" << endl;
        return "";
    }

    // Tìm vị trí dấu ngoặc kép để tách đường dẫn
    int start = resp.find("\"");
    int end = resp.find("\"", start + 1);
    if (start == string::npos || end == string::npos) return "";

    string path = resp.substr(start + 1, end - start - 1);
    return path.empty() ? "/" : path;
}

bool FtpClient::ScanWithClamAV(const string& fileName, vector<char> fileData){
    //1. Kiểm tra file data
    if(fileData.empty()){
        cout << "fileData is empty.\n";
        return false;
    }

    //2. Khởi tạo kết nối đến ClamAV Agent
    if(firstTime){
        cout << "ENTER CLAMAV AGENT IP ADDRESS: "; cin >> IP;
        cout << "ENTER CLAMAV AGENT PORT: "; cin >> PORT;
        firstTime = false;
    }
    SOCKET clamSock = ConnectToServer(IP.c_str(), PORT.c_str());
    if(clamSock == INVALID_SOCKET){
        cout << "Connect to ClamAV error.\n";
        firstTime = true;
        return false;
    }

    //3. Gửi header cho ClamAV Agent
    string header = "put " + fileName + " " + to_string(fileData.size()) + "\n";
    if(send(clamSock, header.c_str(), header.size(), 0) == SOCKET_ERROR){ 
        cout << "Send command error.\n";
        closesocket(clamSock);
        return false;
    }

    //4. Gửi file data cho ClamAV Agent
    int totalSent = 0;
    while(totalSent < fileData.size()){
        int sent = send(clamSock, fileData.data() + totalSent, fileData.size() - totalSent, 0);
        if(sent <= 0){
            cout << "Send file data error.\n";
            closesocket(clamSock);
            return false;
        }
        totalSent += sent;
    }
    cout << "File sent to ClamAV Agent: " << fileName << endl;
    cout << "Waiting for ClamAV response...\n";
    
    //5. Nhận phản hồi từ ClamAV Agent
    char recvbuf[512];
    int iResult = recv(clamSock, recvbuf, sizeof(recvbuf) - 1, 0);
    if(iResult <= 0){
        cout << "Receive failed.\n";
        closesocket(clamSock);
        return false;
    }
    recvbuf[iResult] = '\0';
    cout << "ClamAV response for " << fileName << " : " << recvbuf << endl;
    closesocket(clamSock);

    //6. Kiểm tra phản hồi ("OK" or "INFECTED")
    string response = recvbuf;
    return response.find("OK") != string::npos;
}

bool FtpClient::isConnected(){
    return CtrlSocket != INVALID_SOCKET;
}

bool FtpClient::Connected(){
    CtrlSocket = ConnectToServer(serverIP.c_str(), serverPORT.c_str());
    if(CtrlSocket == INVALID_SOCKET) return false;
    return true;
}

bool FtpClient::Login(const char* username, const char* password){
    char buffer[512];
    int iResult;
    
    //1. Nhận phản hồi chào mừng từ server
    iResult = recv(CtrlSocket, buffer, sizeof(buffer) - 1 , 0);
    if(iResult > 0){
        buffer[iResult] = '\0';
        cout << "Welcome: " << buffer;
    }
    
    //2. Gửi lệnh USER
    snprintf(buffer, sizeof(buffer), "USER %s\r\n", username);
    iResult = send(CtrlSocket, buffer, (int)strlen(buffer), 0);
    if(iResult == SOCKET_ERROR){
        cout << "Send USER failed: " << WSAGetLastError() << endl;
        return false;
    }
    
    //3. Nhận phản hồi sau khi gửi lệnh USER
    char recvbuf[512];
    iResult = recv(CtrlSocket, recvbuf, sizeof(recvbuf) - 1, 0);
    if(iResult <= 0){
        cout << "Receive after USER failed: " << WSAGetLastError() << endl;
        return false;
    }
    recvbuf[iResult] = '\0';
    // cout << "Server: " << recvbuf;
    
    //4. Kiểm tra xem phản hồi có phải mã 331 không
    if(strncmp(recvbuf, "331", 3) != 0){
        cout << "Unexpected response after USER.\n";
        return false;
    }
    
    //5. Gửi lệnh PASS
    snprintf(buffer, sizeof(buffer), "PASS %s\r\n", password);
    iResult = send(CtrlSocket, buffer, (int)strlen(buffer), 0);
    if(iResult == SOCKET_ERROR){
        cout << "Send PASS failed: " << WSAGetLastError() << endl;
        return false;
    }
    
    //6. Nhận phản hồi sau khi gửi lệnh PASS
    iResult = recv(CtrlSocket, recvbuf, sizeof(recvbuf) - 1, 0);
    if(iResult <= 0){
        cout << "Receive after PASS failed: " << WSAGetLastError() << endl;
        return false;
    }
    recvbuf[iResult] = '\0';
    // cout << "Server: " << recvbuf;
    
    //7. Kiểm tra phản hồi có phải mã 230 không
    if(strncmp(recvbuf, "230", 3) != 0){
        cout << "Unexpected response after PASS.\n";
        return false;
    }
    
    return true;
}

bool FtpClient::Upload(const string& fileName){
    int iResult;
    char buffer[512];
    char recvbuff[512];
    
    //1. Kiểm tra file
    vector<char> fileData = ReadFile(fileName);
    if(fileData.empty()){
        cout << "fileData empty.\n";
        return false;
    }
    
    //2. Kết nối với data socket
    string IP, PORT;
    SOCKET dataSock = (isPassive ? EnterPASVMode(IP, PORT) : EnterPORTMode());
    if(dataSock == INVALID_SOCKET){
        cout << (isPassive ? "EnterPASVMode error.\n" : "EnterPORTMode error.\n");
        return false;
    }
    
    //3. Gửi lệnh STOR
    // Trích tên file
    fs::path p(fileName);
    string justName = p.filename().string();

    snprintf(buffer, sizeof(buffer), "STOR %s\r\n", justName.c_str()); // Lệnh có dạng STOR "filename", được lưu vào buffer
    iResult = send(CtrlSocket, buffer, (int)strlen(buffer), 0);
    if(iResult == SOCKET_ERROR){
        cout << "Send STOR failed: " << GetLastError() << endl;
        closesocket(dataSock);
        return false;
    }
    
    iResult = recv(CtrlSocket, recvbuff, sizeof(recvbuff) - 1, 0);
    if(iResult <= 0){
        cout << "Receive STOR response failed.\n";
        closesocket(dataSock);  
        return false;
    }
    recvbuff[iResult] = '\0';
    // cout << "STOR response: " << recvbuff;
    
    // Kiểm tra mã phản hồi
    if(strncmp(recvbuff, "150", 3) != 0 && strncmp(recvbuff, "125", 3) != 0){
        cout << "Server rejected STOR request.\n";
        closesocket(dataSock);
        return false;
    }
    
    //4. Gửi nội dung file qua data socket
    int totalSent = 0;
    int totalSize = fileData.size();
    while(totalSent < fileData.size()){
        int sent = send(dataSock, fileData.data() + totalSent, (int)fileData.size() - totalSent, 0);
        if(sent <= 0){
            cout << "Sent file data error.\n";
            closesocket(dataSock);
            return false;
        }
        totalSent += sent;

        float persent = 100.0f * totalSent / totalSize;
        printf("\rUploading: [%-50s] %.2f%%", string((int)(persent / 2), '=').c_str(), persent);
        fflush(stdout);
        Sleep(30);
    }   
    printf("\n");
    closesocket(dataSock);
    
    //5. Nhận phản hồi từ server
    iResult = recv(CtrlSocket, recvbuff, sizeof(recvbuff) - 1, 0);
    if(iResult > 0){
        recvbuff[iResult] = '\0';
        // cout << "Server after transfer: " << recvbuff;
        return true;
    }
    
    return false; 
}

bool FtpClient::Download(const string& remotePath){
    string resp;
    char buffer[2048];

    //1. Tách thư mục và tên file
    fs::path p(remotePath);
    string dir = p.parent_path().string();
    string fileName = p.filename().string();


    //2. CWD đến thư mục chứa file
    if(!dir.empty()){
        SendCmd("CWD /\r\n");
        resp = SendCmd("CWD " + dir + "\r\n");
        if(resp.substr(0, 3) != "250"){
            cout << "CWD failed: " << dir << endl;
            return false;
        }
    }

    //3. Kết nối với data socket
    string IP, PORT;
    SOCKET dataSock = (isPassive ? EnterPASVMode(IP, PORT) : EnterPORTMode());
    if(dataSock == INVALID_SOCKET){
        cout << (isPassive ? "EnterPASVMode error.\n" : "EnterPORTMode error.\n");
        return false;
    }
    
    //4. Gửi lệnh RETR
    string cmd = "RETR " + fileName + "\r\n";
    resp = SendCmd(cmd);
    if(resp.substr(0, 3) != "150" && resp.substr(0, 3) != "125"){
        cout << "Server rejected RETR request.\n";
        closesocket(dataSock);
        return false;
    }
    // cout << "RETR response: " << resp;
    
    //5. Nhận dữ liệu và lưu
    string savePath = (remotePath[0] == '/') ? remotePath.substr(1) : remotePath;
    fs::path parent = fs::path(savePath).parent_path();
    if(!parent.empty()){ // Chỉ tạo thư mục cha nếu nó không rỗng
        fs::create_directories(parent);
    }

    ofstream outFile(savePath, ios::binary);
    if(!outFile.is_open()){
        cout << "Cannot open file to save: " << fileName << endl;
        closesocket(dataSock);
        return false;
    }
    
    int totalRecv = 0;
    while(true){
        int bytes = recv(dataSock, buffer, sizeof(buffer), 0);
        if(bytes <= 0) break;

        outFile.write(buffer, bytes);
        totalRecv += bytes;

        float percent = (float)totalRecv / 1024 / 1024; // tính MB đã tải
        printf("\rDownloading: %.2f MB received", percent);
        fflush(stdout);
    }
    printf("\n");
    outFile.close();
    closesocket(dataSock);

    // Nhận phản hồi cuối 
    SendCmd("");

    return true;;
}

void FtpClient::List(){
    char buffer[2048];
    char recvbuff[2048];
    
    //1. Gửi lệnh PASV
    string IP, PORT;
    SOCKET dataSock = (isPassive ? EnterPASVMode(IP, PORT) : EnterPORTMode());
    if(dataSock == INVALID_SOCKET){
        cout << (isPassive ? "EnterPASVMode error.\n" : "EnterPORTMode error.\n");
        return;
    }
    
    //2. Gửi lệnh LIST
    int iResult = send(CtrlSocket, "LIST\r\n", 6, 0);
    if(iResult == SOCKET_ERROR){
        cout << "Send LIST command failed.\n";
        closesocket(dataSock);
        return;
    }
    //3. Nhận phản hồi sau khi gửi lệnh LIST
    iResult = recv(CtrlSocket, buffer, sizeof(recvbuff) - 1, 0);
    if (iResult > 0){
        buffer[iResult] = '\0';
        // cout << "Server response: " << buffer;
    }
    
    //4. Nhận danh sách
    int bytes = recv(dataSock, buffer, sizeof(buffer) - 1, 0); 
    while(bytes > 0){
        buffer[bytes] = '\0';
        cout << buffer;
        bytes = recv(dataSock, buffer, sizeof(buffer) - 1, 0);
    }
    
    closesocket(dataSock);
    //5. Nhận phản hồi cuối cùng
    iResult = recv(CtrlSocket, buffer, sizeof(buffer) - 1, 0);
    if (iResult > 0) {
        buffer[iResult] = '\0';
        // cout << "Final server message: " << buffer;
    }
}

string FtpClient::SendCmd(const string& cmd){
    //1. Gửi command cho server
    send(CtrlSocket, cmd.c_str(), (int)cmd.size(), 0);
    
    //2. Nhận và in phản hồi từ server
    char recvbuf[1024];
    int iResult = recv(CtrlSocket, recvbuf, sizeof(recvbuf) - 1, 0);
    if(iResult > 0){
        recvbuf[iResult] = '\0';
        cout << recvbuf << endl;
        return string(recvbuf);
    }
    return "";
}

void FtpClient::togglePrompt(){
    prompt = !prompt;
    cout << "Prompt is now " << (prompt ? "ON" : "OFF") << endl;
}

void FtpClient::mput(const vector<string>& paths){
    cout << (prompt ? "Prompt is now ON.\n" : "Prompt is now OFF.\n");

    // Danh sách các file/thư mục đồng ý
    vector<string> acceptPath;
    if(prompt){
        for(const auto& path : paths){
            cout << "Upload " << path << "?    [y/n]: ";
            string ans;
            while(true){
                cin >> ans;
                if(ans == "y" || ans == "Y"){
                    acceptPath.push_back(path);
                    break;
                }
                else if(ans == "n" || ans == "N"){
                    break;
                }
                else cout << "Invalid command.";
            }
        }
        if(acceptPath.empty()){
            cout << "No path selected for upload.\n";
            return;
        }
    }

    // Duyệt danh sách
    for(const auto& path : (prompt ? acceptPath : paths)){
        if(!fs::exists(path)){
            cout << "Path doesn't exists.\n";
            continue;
        }

        // Nếu là thư mục
        if(fs::is_directory(path)){
            putDir(path, fs::path(path).filename().string()); // Gọi putDir()
        }
        // Nếu là file
        else if(fs::is_regular_file(path)){
            if(!ScanWithClamAV(path, ReadFile(path))){
                cout << "File is INFECTED or sending error. Upload aborted.\n";
                continue; 
            }
            
            if(Upload(path)) cout << "Uploaded: " << path << endl; 
            else cout << "Failed: " << path << endl;
        }
        else cout << "Unsupported file type.\n";
    }
}

void FtpClient::mget(const vector<string>& paths){
    cout << (prompt ? "Prompt is now ON.\n" : "Prompt is now OFF.\n");

    vector<string> acceptPath;
    if(prompt){
        for(const auto& path : paths){
            cout << "Download " << path << "?    [y/n]: ";
            string ans;
            while(true){
                cin >> ans;
                if(ans == "y" || ans == "Y"){
                    acceptPath.push_back(path);
                    break;
                }
                else if(ans == "n" || ans == "N"){
                    break;
                }
                else cout << "Invalid command.";
            }
        }
        if(acceptPath.empty()){
            cout << "No path selected for download.\n";
            return;
        }
    }

    for(const auto& path : (prompt ? acceptPath : paths)){
        string currDir = GetCurrentDir();
        char type = GetRemoteType(path);

        if(type == 'd'){
            getDir(fs::path(path).filename().string(), path);
        }
        else if(type == '-'){
            if(Download(path)) cout << "Downloaded: " << path << endl;
            else cout << "Failed: " << path << endl;
        }
        else cout << "Unknown remote type or path does not exist.\n";
        SendCmd("CWD " + currDir + "\r\n");
    }
}

void FtpClient::SetTransferMode(bool ascii){
    isAcsiiMode = ascii;
    string cmd = (ascii ? "TYPE A\r\n" : "TYPE I\r\n");
    SendCmd(cmd);
} 

void FtpClient::Disconnect(){
    if(CtrlSocket != INVALID_SOCKET){
        closesocket(CtrlSocket);
    }
    CtrlSocket = INVALID_SOCKET;
}

void FtpClient::putDir(const string& localPath, const string& remoteBase){
    vector<pair<string, string>> filesToUpload; // Danh sách file và địa chỉ của nó

    // Tạo thư mục gốc
    SendCmd("MKD " + remoteBase + "\r\n");

    // Tạo toàn bộ thư mục trước
    for(const auto& entry : fs::recursive_directory_iterator(localPath)){ // Đệ quy các thư mục tính từ địa chỉ localPath
        fs::path relPath = fs::relative(entry.path(), localPath);
        string remotePath = remoteBase + "/" + relPath.generic_string(); 

        /*
        Ví dụ:
        - localPath = "./myfolder"
        - entry.path() = "./myfolder/images/logo.png"
        => relPath = "images/logo.png"  (đường dẫn con bên trong localPath)
        => remotePath = "myfolder/images/logo.png" (đường dẫn tương ứng trên FTP server)
        */
        
        if(fs::is_directory(entry)){
            // Tạo thư mục theo đúng thứ tự
            SendCmd("MKD " + remotePath + "\r\n");
        }
        else if(fs::is_regular_file(entry)){
            // Ghi nhớ file và nơi cần upload
            filesToUpload.emplace_back(entry.path().generic_string(), remotePath);
        }
    }

    // Upload tất cả các file đã duyệt
    for(const auto& [fullPath, remotePath] : filesToUpload){
        cout << "Uploading: " << remotePath << endl;

        // Lấy tên file
        string fileName = fs::path(fullPath).filename().string();

        if(!ScanWithClamAV(fileName, ReadFile(fullPath))){
            cout << "File " << fileName << " is INFECTED or error, skipping.\n";
            continue;
        }   

        fs::path parent = fs::path(remotePath).parent_path(); // Trích địa chỉ phần cha. Ví dụ: "./myfolder/images/logo.png" => "./myfolder/images"
        // Đảm bảo cha thư mục tồn tại và đúng vị trí
        SendCmd("MKD " + parent.generic_string() + "\r\n"); // Gửi lệnh tạo thư mục cha
        SendCmd("CWD " + parent.generic_string() + "\r\n"); // Chuyển cd về thư mục cha

        if(Upload(fullPath)){
            cout << "Uploaded: " << remotePath << endl;
        }
        else{
            cout << "Failed: " << remotePath << endl;
        }

        SendCmd("CWD /\r\n");
    }
}

char FtpClient::GetRemoteType(const string& path){
    // Kiểm thử bằng cách chuyển thư mục
    string currDir = GetCurrentDir(); // Lưu địa chỉ thư mục hiện tại
    string resp = SendCmd("CWD " + path + "\r\n");
    if(resp.substr(0, 3) == "250"){
        SendCmd("CWD " + currDir + "\r\n");
        return 'd'; // Nếu chuyển được thì nó là thư mục
    }

    // Kiểm thử bằng cách gửi lệnh SIZE
    resp = SendCmd("SIZE " + path + "\r\n");
    if(resp.substr(0, 3) == "213") return '-';

    return 'x';
}

void FtpClient::getDir(const string& localBase, const string& remotePath){
    // Tạo thư mục hiện tại
    fs::create_directories(localBase);
    SendCmd("CWD /\r\n");

    string resp = SendCmd("CWD " + remotePath + "\r\n");
    if(resp.substr(0, 3) != "250"){
        cout << "CWD failed: " << remotePath << endl;
        return;
    }

    resp = SendCmd("PWD\r\n");
    if(resp.substr(0, 3) != "257"){
        cout << "PWD failed at: " << remotePath << endl;
        return;
    }

    // Gửi lệnh PASV
    string IP, PORT;
    SOCKET dataSock = (isPassive ? EnterPASVMode(IP, PORT) : EnterPORTMode());
    if(dataSock == INVALID_SOCKET){
        cout << (isPassive ? "EnterPASVMode error.\n" : "EnterPORTMode error.\n");
        return;
    }

    // Gửi lệnh lấy danh sách thư mục
    resp = SendCmd("LIST\r\n");
    if(resp.substr(0, 3) != "150" && resp.substr(0, 3) != "125"){
        cout << "LIST failed at" << localBase << endl;
        closesocket(dataSock);
        return;
    }

    // Nhận danh sách
    char buffer[2048];
    string listData;
    int bytes;
    while((bytes = recv(dataSock, buffer, sizeof(buffer) - 1, 0)) > 0){
        buffer[bytes] = '\0';
        listData += buffer;
    }
    
    SendCmd(""); // Nhận phản hồi

    // Xử lí từng dòng trong danh sách
    stringstream ss(listData);
    string line;
    while(getline(ss, line)){
        if(line.empty()) continue; // Bỏ qua dòng trống

        /* Ví dụ danh sách như sau
            drwxrwxrwx 1 ftp ftp               0 Jul 11 05:01 test
            -rw-rw-rw- 1 ftp ftp               0 Jul 11 05:31 test.txt
        */
        // Trích tên file
        stringstream s(line);
        string temp, name;
        for(int i = 0 ; i < 8 ; i++) s >> temp;
        getline(s, name); // Trường hợp "te st.txt"
        if(!name.empty() && name[0] == ' ') name = name.substr(1); // Trường hợp " test.txt"

        // Làm sạch tên
        string clear;
        for(char c : name){
            if(c != '\r' && c != '\n' && c != '\t'){
                clear += c;
            }
        }
        name = clear;
        if(name == "." || name == "..") continue;

        // Lấy kiểu file
        char type;
        type = line[0];

        string remoteSub = remotePath + (remotePath.back() == '/' ? "" : "/") + name; // Đường dẫn trên FTP server
        string localSub = localBase + "/" + name; // Đường dẫn trên máy hiện tại

        if(type == 'd'){    
            // Nếu là thư mục, gọi đệ qui
            cout << "Save at: " << remoteSub << endl;
            getDir(localSub, remoteSub);
        }
        else if(type == '-'){
            cout << "Downloading: " << remoteSub << endl;
            if(Download(remoteSub)){
                cout << "Downloaded: " << remoteSub << endl;
            }
            else cout << "Failed: " << remoteSub << endl;
        }
    }
    SendCmd("CWD /\r\n");
}

void FtpClient::SetPassiveMode(){
    if(isPassive){
        cout << "Already in passive mode.\n";
        return;
    }
    isPassive = true;
    cout << "Passive mode is now ON." << endl;
}

void FtpClient::SetActiveMode(){
    if(!isPassive){
        cout << "Already in active mode.\n";
        return;
    }
    isPassive = false;
    cout << "Active mode is now ON." << endl;
}