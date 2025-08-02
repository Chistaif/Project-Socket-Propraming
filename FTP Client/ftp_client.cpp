#include "FtpClient.h"
#include "conio.h"
#include "windows.h"
// Biên dịch: g++ FtpClient.cpp ftp_client.cpp -o ftp_client.exe -lws2_32

void ShowMenu(const vector<string> &options, int choice);
void handleSubMenu(const vector<string> &subOptions, FtpClient &ftp);
void Execute(const string &cmd, FtpClient &ftp);

int main()
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        cout << "WSAStarup failed: " << iResult << endl;
        return 1;
    }

    cout << "========================== CONNECT TO ==========================\n";
    string temp1, temp2;
    cout << "ENTER SERVER'S IP ADDRESS: ";
    cin >> temp1;
    cout << "ENTER SERVER'S PORT: ";
    cin >> temp2;

    FtpClient ftp(temp1, temp2);
    if (!ftp.Connected())
    {
        cout << "\nUnable to connect to server!\n";
        WSACleanup();
        return 1;
    }

    cout << "\n========================== LOGIN ==========================\n";
    cout << "ENTER USER NAME: ";
    cin >> temp1;
    cout << "ENTER PASSWORD: ";
    cin >> temp2;

    if (ftp.Login(temp1.c_str(), temp2.c_str()))
    {
        vector<string> mainMenu =
            {
                {"0. Exit"},
                {"1. Server Connection Management"},
                {"2. Directory/Path Operations"},
                {"3. File Operations"},
                {"4. Settings"},
                {"5. Help"}};
        vector<vector<string>> subMenu =
            {
                {"open", "close", "status"},
                {"ls", "cd", "pwd", "mkdir", "rmdir", "rename"},
                {"put", "get", "mput", "mget", "delete", "rename"},
                {"ascii", "binary", "prompt", "active" , "passive"}};

        int choice = 1;
        while (true)
        {
            ShowMenu(mainMenu, choice);
            int key = _getch();
            if (key == 0 || key == 224)
            {                   // Phát hiện phím đặc biệt
                key = _getch(); // Đọc mã thực sự
            }
            switch (key)
            {
            case 72:
                choice--;
                if (choice < 0)
                    choice = 5;
                break;
            case 80:
                choice++;
                if (choice > 5)
                    choice = 0;
                break;
            case 13:
                if (choice == 5)
                {
                    Execute("help", ftp);
                }
                else if (choice == 0)
                {
                    cout << "GoodBye!\n";
                    WSACleanup();
                    return 0;
                }
                else if (choice >= 1 && choice <= subMenu.size())
                {
                    handleSubMenu(subMenu[choice - 1], ftp);
                }
                break;
            default:
                cout << "Use UP_ARROW / DOWN_ARROW / ENTER to work with this menu\n";
                system("pause");
                break;
            }
        }
    }
    else
    {
        cout << "LOGIN FAILED!\nMaybe user name or password error!\n";
    }

    WSACleanup();
    return 0;
}

//==============================================================

void ShowMenu(const vector<string> &options, int choice)
{
    system("cls");
    cout << "========== FTP CLIENT MENU ==========\n";
    for (int i = 1; i < options.size(); i++)
    {
        if (i == choice)
            cout << "-> " << options[i] << endl;
        else
            cout << "   " << options[i] << endl;
    }
    cout << ((choice == 0) ? "-> " : "   ") << options[0] << endl;

    cout << "===============================\n";
    cout << "Navigate: Up/Down arrows | Select: Enter\n";
}

void handleSubMenu(const vector<string> &subOptions, FtpClient &ftp)
{
    int subChoice = 0;
    while (true)
    {
        system("cls");
        cout << "========== SUB-MENU ==========" << endl;
        for (int i = 0; i < subOptions.size(); ++i)
        {
            if (i == subChoice)
                cout << "-> " << subOptions[i] << endl;
            else
                cout << "   " << subOptions[i] << endl;
        }

        int key = _getch();
        if (key == 0 || key == 224)
        {                   // Phát hiện phím đặc biệt
            key = _getch(); // Đọc mã thực sự
        }
        switch (key)
        {
        case 72:
        {
            subChoice--;
            if (subChoice < 0)
                subChoice = subOptions.size() - 1;
            break;
        }
        case 80:
        {
            subChoice++;
            if (subChoice > subOptions.size() - 1)
                subChoice = 0;
            break;
        }
        case 13:
        {
            Execute(subOptions[subChoice], ftp);
            break;
        }
        case 27:
            return;
        default:
        {
            cout << "Use UP_ARROW / DOWN_ARROW / ENTER / ESC to work with this menu\n";
            system("pause");
            break;
        }
        }
    }
}

void Execute(const string &cmd, FtpClient &ftp)
{
    if (!ftp.isConnected() && cmd != "open" && cmd != "help")
    {
        cout << "Not connected to server. Use 'open' first.\n";
    }

    if (cmd == "ls")
    {
        ftp.List();
    }
    else if (cmd == "cd")
    {
        string path;
        cout << "Enter directory path to change to: ";
        cin >> path;
        ftp.SendCmd("CWD " + path + "\r\n");
    }
    else if (cmd == "pwd")
    {
        cout << "Current directory:\n";
        ftp.SendCmd("PWD\r\n");
    }
    else if (cmd == "mkdir")
    {
        string dir;
        cout << "Enter directory name to create: ";
        cin >> dir;
        ftp.SendCmd("MKD " + dir + "\r\n");
    }
    else if (cmd == "rmdir")
    {
        string dir;
        cout << "Enter directory name to remove: ";
        cin >> dir;
        ftp.SendCmd("RMD " + dir + "\r\n");
    }
    else if (cmd == "delete")
    {
        string fileName;
        cout << "Enter file name to delete: ";
        cin >> fileName;
        ftp.SendCmd("DELE " + fileName + "\r\n");
    }
    else if (cmd == "rename")
    {
        string from, to;
        cout << "Enter current file name: ";
        cin >> from;
        cout << "Enter new file name: ";
        cin >> to;
        string temp = ftp.SendCmd("RNFR " + from + "\r\n");
        if(temp.substr(0, 3) != "350")
        {
            cout << "RNFR failed: " << temp << endl;
            return;
        }
        ftp.SendCmd("RNTO " + to + "\r\n");
    }
    else if (cmd == "put")
    {
        string path;
        cout << "Enter local file/folder path to upload: ";
        cin >> path;
        if (!fs::exists(path))
        {
            cout << "Path doesn't exists.\n";
            return;
        }

        // Nếu là thư mục
        if (fs::is_directory(path))
        {
            ftp.putDir(path, fs::path(path).filename().string()); // Gọi putDir()
        }
        // Nếu là file
        else if (fs::is_regular_file(path))
        {
            if (!ftp.ScanWithClamAV(path, ftp.ReadFile(path)))
            {
                cout << "File is INFECTED or sending error. Upload aborted.\n";
                return;
            }
            if (ftp.Upload(path))
            {
                cout << "Uploaded: " << path << endl;
            }
            else
                cout << "Failed: " << path << endl;
        }
        else
        {
            cout << "Unsupported file type.\n";
        }
    }
    else if (cmd == "mput")
    {
        vector<string> files;
        string input;
        cout << "Enter file names to upload (separated by spaces):\n";
        getline(cin >> ws, input);

        stringstream ss(input);
        string temp;
        while (ss >> temp)
            files.push_back(temp);
        ftp.mput(files);
    }
    else if (cmd == "get" || cmd == "recv")
    {
        string remotePath;
        cout << "Enter remote file/folder path to download: ";
        cin >> remotePath;
        char type = ftp.GetRemoteType(remotePath); // Phân loại kiểu

        string currDir = ftp.GetCurrentDir();
        if (type == 'd')
        {
            ftp.getDir(fs::path(remotePath).filename().string(), remotePath);
        }
        else if (type == '-')
        {
            if (ftp.Download(remotePath))
                cout << "Downloaded: " << remotePath << endl;
            else
                cout << "Failed: " << remotePath << endl;
        }
        else
            cout << "Unknown remote type or path does not exist.\n";
        ftp.SendCmd("CWD " + currDir + "\r\n");
    }
    else if (cmd == "mget")
    {
        vector<string> files;
        string input;
        cout << "Enter file names to download (separated by spaces):\n";
        getline(cin >> ws, input);

        stringstream ss(input);
        string temp;
        while (ss >> temp)
            files.push_back(temp);
        ftp.mget(files);
    }
    else if (cmd == "prompt")
    {
        cout << "Toggled interactive mode.\n";
        ftp.togglePrompt();
    }
    else if (cmd == "ascii")
    {
        cout << "Switched to ASCII mode." << endl;
        ftp.SetTransferMode(true);
    }
    else if (cmd == "binary")
    {
        cout << "Switched to BINARY mode." << endl;
        ftp.SetTransferMode(false);
    }
    else if (cmd == "status")
    {
        cout << "FTP Server: " << (ftp.isConnected() ? "Connected\n" : "Disconnected\n");
    }
    else if (cmd == "passive")
    {   
        ftp.SetPassiveMode();
    }
    else if (cmd == "active")
    {
        ftp.SetActiveMode();
    }
    else if (cmd == "open")
    {
        if (ftp.isConnected())
        {
            cout << "Already connected to server." << endl;
        }
        else
        {
            cout << "========================== CONNECT TO ==========================\n";
            string temp1, temp2;
            cout << "ENTER SERVER'S IP ADDRESS: ";
            cin >> temp1;
            cout << "ENTER SERVER'S PORT: ";
            cin >> temp2;

            ftp.Disconnect();
            ftp.SetServerInfo(temp1, temp2);
            if (!ftp.Connected())
            {
                cout << "\nUnable to connect to server!\n";
            }
            else
            {
                cout << "\n========================== LOGIN ==========================\n";
                cout << "ENTER USER NAME: ";
                cin >> temp1;
                cout << "ENTER PASSWORD: ";
                cin >> temp2;

                if(!ftp.Login(temp1.c_str(), temp2.c_str()))
                    cout << "LOGIN FAILED!\nMaybe user name or password error!\n";
                else
                    cout << "Login successful!\n";
            }
        }
    }
    else if (cmd == "close")
    {
        ftp.Disconnect();
        cout << "Disconnected from FTP server..." << endl;
    }
    else if(cmd == "help" || cmd == "?")
    {
        cout << "\nFTP CLIENT COMMAND REFERENCE\n";
        cout << "1. Connection: open, close, status, login\n";
        cout << "2. Directories: ls, cd, pwd, mkdir, rmdir\n";
        cout << "3. Files: put, get, mput, mget, delete, rename\n";
        cout << "4. Settings: ascii, binary, prompt, passive\n";
        cout << "5. Navigation: Press ESC to exit submenus\n";
    }
    else
        cout << "Invalid command. Type 'help' for supported commands.\n";

    system("pause");
}