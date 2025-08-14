
# Ứng dụng FTP Client an toàn với tính năng quét virus bằng ClamAVAgent

## 📌 Giới thiệu

Project mô phỏng một hệ thống truyền file an toàn, trong đó mọi file được tải lên bởi FTP Client tùy chỉnh đều phải **được quét virus bởi ClamAV** trước khi được gửi tới FTP server. Hệ thống bao gồm:

- ✅ Một **FTP Client** tùy chỉnh với các lệnh FTP cơ bản.
- ✅ Một **ClamAVAgent server** dùng để nhận file và quét virus.
- ✅ Một **FTP server bên ngoài** (ví dụ: FileZilla Server) để lưu trữ các file an toàn.

Tất cả quá trình truyền file và giao tiếp lệnh được xây dựng bằng **lập trình Winsock C++**, không dùng lệnh hệ thống (trừ `clamdscan`).

---

## 📁 Cấu trúc dự án

```
.
│   README.md
│   REPORT.pdf
│
├───ClamAV_Agent
│       clamav_agent.cpp
│       scanwithclamav.cpp
│       scanwithclamav.h
│
└───FTP_Client
        FtpClient.cpp
        FtpClient.h
        ftp_client.cpp
```

---

## ⚙️ Hướng dẫn cài đặt

### 1. 🧪 Cài đặt ClamAV (Chi tiết các bước xem ở bên dưới)

- Tải ClamAV từ: [https://www.clamav.net/downloads](https://www.clamav.net/downloads)
- Thêm `clamdscan` vào biến môi trường PATH.
- Kiểm tra:
  ```bash
  clamscan --version
  ```
 
### 2. 🖥️ Cài đặt FTP Server

Sử dụng bất kỳ phần mềm FTP nào, ví dụ:

- FileZilla Server (Windows) (recommend) (**có hướng dẫn download và settup bên dưới**)
- `vsftpd` (Linux)

Cấu hình user: `user`, password: `123`, bật chế độ Passive Mode.

### 3. 🛠️ Biên dịch

Yêu cầu dùng trình biên dịch hỗ trợ C++17 trở lên. Ví dụ trên Windows (MinGW):

**ClamAVAgent**
```bash
g++ -std=c++17 -o clamav_agent clamav_agent.cpp scanwithclamav.cpp -lws2_32 -lstdc++fs
```

**FTP Client**
```bash
g++ -std=c++17 -o ftp_client ftp_client.cpp FtpClient.cpp -lws2_32
```

---

## ▶️ Chạy chương trình

### Bước 1: Chạy clamd + ClamAV Agent trong terminal và nhập IP, PORT hiện tại

```bash
./clamd
```
```bash
./clamav_agent
```

### Bước 2: Chạy FTP Client và nhập IP, PORT server

```bash
./ftp_client
```

- Nhập lệnh từ giao diện dòng lệnh.

---

## 💻 Các lệnh hỗ trợ

| Lệnh             | Chức năng                                   |
|------------------|---------------------------------------------|
| `ls`             | Liệt kê file và thư mục trên server         |
| `cd <thư_mục>`   | Đổi thư mục hiện tại trên server            |
| `pwd`            | Hiển thị thư mục hiện tại                   |
| `mkdir <tên>`    | Tạo thư mục mới trên server                 |
| `rmdir <tên>`    | Xóa thư mục trên server                     |
| `delete <file>`  | Xóa file trên server                        |
| `rename <a> <b>` | Đổi tên file                                |
| `put <file>`     | Upload file (quét virus trước khi gửi)      |
| `mput <f1> ...`  | Upload nhiều file (quét virus trước khi gửi)|
| `get <file>`     | Tải file từ server                          |
| `mget <f1> ...`  | Tải nhiều file                              |
| `prompt`         | Bật/tắt xác nhận khi mput/mget              |
| `ascii`          | Chuyển sang chế độ ASCII                    |
| `binary`         | Chuyển sang chế độ Binary                   |
| `open`           | Kết nối lại tới FTP server                  |
| `close`          | Ngắt kết nối FTP server                     |
| `status`         | Trạng thái kết nối hiện tại                 |
| `passive`        | Passive mode luôn bật (mô phỏng)            |
| `help / ?`       | Hiển thị danh sách lệnh hỗ trợ              |
| `quit / bye`     | Thoát chương trình                          |

---

## 📸 Ví dụ đầu ra

```
========== FTP CLIENT MENU ==========
-> 1. Server Connection Management
   2. Directory/Path Operations
   3. File Operations
   4. Settings
   5. Help
   0. Exit
===============================
Navigate: Up/Down arrows | Select: Enter



========== SUB-MENU ==========
-> open
   close
   status
Already connected to server.


========== SUB-MENU ==========
   open
-> close
   status
Disconnected from FTP server...


========== SUB-MENU ==========
-> open
   close
   status
========================== CONNECT TO ==========================
ENTER SERVER'S IP ADDRESS: 127.0.0.1
ENTER SERVER'S PORT: 21

========================== LOGIN ==========================
ENTER USER NAME: user
ENTER PASSWORD: 123
Welcome: 220-FileZilla Server 1.10.3
220 Please visit https://filezilla-project.org/
Login successful!


========== SUB-MENU ==========
-> ls
   cd
   pwd
   mkdir
   rmdir
   rename
drwxrwxrwx 1 ftp ftp               0 Aug 14 10:38 hello
-rw-rw-rw- 1 ftp ftp               3 Aug 14 10:38 test.txt


========== SUB-MENU ==========
   ls
-> cd
   pwd
   mkdir
   rmdir
   rename
Enter directory path to change to: hello
250 CWD command successful


========== SUB-MENU ==========
   ls
   cd
-> pwd
   mkdir
   rmdir
   rename
Current directory:
257 "/hello" is current directory.


========== SUB-MENU ==========
   ls
   cd
   pwd
-> mkdir
   rmdir
   rename
Enter directory name to create: hi
257 "/hello/hi" created successfully.


========== SUB-MENU ==========
   ls
   cd
   pwd
   mkdir
-> rmdir
   rename
Enter directory name to remove: hi
250 Directory deleted successfully.


========== SUB-MENU ==========
   ls
   cd
   pwd
   mkdir
   rmdir
-> rename
Enter current file name: hello
Enter new file name: hi
350 Directory exists, ready for destination name.

250 File or directory renamed successfully.


========== SUB-MENU ==========
-> put
   get
   mput
   mget
   delete
   rename
Enter local file/folder path to upload: 
test.txt
File sent to ClamAV Agent: test.txt
Waiting for ClamAV response...
ClamAV response for test.txt : OK
Uploading: [==================================================] 100.00%
Uploaded: test.txt


========== SUB-MENU ==========
   put
-> get
   mput
   mget
   delete
   rename
Enter remote file/folder path to download: test.txt
257 "/" is current directory.

550 Couldn't open the directory

213 3

257 "/" is current directory.

150 Starting data transfer.

Downloading: 0.00 MB received
226 Operation successful

Downloaded: test.txt
250 CWD command successful
```

---

## ⚙️ Hướng dẫn cài đặt ClamAV chi tiết

### 1. Tải và settup ClamAV 

- Truy cập link sau và tải file .zip phù hợp: [https://www.clamav.net/downloads](https://www.clamav.net/downloads).
- Click chuột phải chọn giải nén file .zip vừa tải về.
- Vào folder vừa giải nén và chuyển 2 file `clamd.conf.sample`, `freshclam.conf.sample` ra khỏi folder `conf_examples`. Đồng thời đổi tên thành `clamd.conf`, `freshclam.conf`
- Mở lần lượt `clamd.conf`, `freshclam.conf` và xóa chữ `Example` (không được commend)
- Sau đó nhập các lệnh sau:
```bash
./freshclam.exe
```
- Kiểm tra lại:
```bash
clamdscan --version
```

- Clip tham khảo: [https://www.youtube.com/watch?v=9gQXBUJbSHE&t=1s](https://www.youtube.com/watch?v=9gQXBUJbSHE&t=1s).

### 2. Thêm ClamAV vào PATH

- Mở Start Menu tìm kiểm `Edit the system environment variables` > `Environment Variables...`.
- Trong `System variables` > variable `PATH` > new > gắn đường dẫn đển file clamscan.exe ở bước 1 > OK > OK > OK. 

### 3. Kiểm tra

- Mở cmd ở bất kì đường dẫn nào và chạy lệnh:
```bash
clamdscan --version
```
- Ví dụ trả về nếu settup chuẩn
```bash
ClamAV 1.4.3/27679/Tue Jun 24 15:44:43 2025
```

---

## ⚙️ Hướng dẫn cài đặt FTP Server chi tiết


### 1. Tải và settup FileZilla Server

Làm theo hướng dẫn sau: [https://helpdesk.inet.vn/knowledgebase/huong-dan-thiet-lap-ftp-server-bang-filezilla-windows](https://helpdesk.inet.vn/knowledgebase/huong-dan-thiet-lap-ftp-server-bang-filezilla-windows)

### 2. Settup IP và PORT listen

Ở Menu chính, chọn  Server -> Configure -> Server listeners, thêm/sửa ip, port, đồng thời chỉnh Protocol thành `Explicit FTP over TLS  and insecure plain FTP` (Nếu không có tùy chọn này thì bạn chỉ cần chọn tùy chọn nào có `plain FTP` là được)
![Ảnh minh họa](https://i.postimg.cc/zXsbDHcR/download.png)

## 📎Note

- Project này được **xây dựng trên Windows** và cần một vài thư viện của Windows nên khi chạy trên các hệ điều hành khác thì có thể xảy ra lỗi.

- Khi muốn gửi các file từ user -> server thì nên ở trong cùng thư mục với file thực thi `ftp_client.exe` (hoặc nhập đầy đủ đường dẫn đến file muốn gửi đi). Đồng thời các file được tải về từ server -> user sẽ ở trong thư mục chứa `ftp_client.exe`.

- *CHẮC CHẮN* rằng `FTP Server` và `clamav_agent.exe` (cũng như `clamdscan.exe`) hoạt động trước khi thực thi `ftp_client.exe` để tránh lỗi không cần thiết.