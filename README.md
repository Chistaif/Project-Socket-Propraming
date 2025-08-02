
# Ứng dụng FTP Client an toàn với tính năng quét virus bằng ClamAVAgent

## 📌 Giới thiệu

Project mô phỏng một hệ thống truyền file an toàn, trong đó mọi file được tải lên bởi FTP Client tùy chỉnh đều phải **được quét virus bởi ClamAV** trước khi được gửi tới FTP server. Hệ thống bao gồm:

- ✅ Một **FTP Client** tùy chỉnh với các lệnh FTP cơ bản.
- ✅ Một **ClamAVAgent server** dùng để nhận file và quét virus.
- ✅ Một **FTP server bên ngoài** (ví dụ: FileZilla Server) để lưu trữ các file an toàn.

Tất cả quá trình truyền file và giao tiếp lệnh được xây dựng bằng **lập trình Winsock C++**, không dùng lệnh hệ thống (trừ `clamscan`).

---

## 📁 Cấu trúc dự án

```
.
├───final
│   ├───ClamAV
│   │       clamav_agent.cpp
│   │       scanwithclamav.h
│   │       scanwithclamav.cpp
│   │       
│   └───FTP client
│           FtpClient.h
│           FtpClient.cpp
│           ftp_client.cpp
│           
├───README.md
└───REPORT.pdf
```

---

## ⚙️ Hướng dẫn cài đặt

### 1. 🧪 Cài đặt ClamAV (Chi tiết các bước xem ở bên dưới)

- Tải ClamAV từ: [https://www.clamav.net/downloads](https://www.clamav.net/downloads)
- Thêm `clamscan` vào biến môi trường PATH.
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

### Bước 1: Chạy ClamAV Agent trong terminal và nhập IP, PORT hiện tại

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
ftp> mkdir hello
257 "/hello" created successfully.

ftp> ls
-rw-rw-rw- 1 ftp ftp            3402 Jul 11 09:52 Coding_Standard.txt
drwxrwxrwx 1 ftp ftp               0 Jul 13 09:30 hello
-rw-rw-rw- 1 ftp ftp           13336 Jul 13 09:30 hello_world.docx
drwxrwxrwx 1 ftp ftp               0 Jul 13 09:27 test

ftp> rmdir hello
250 Directory deleted successfully

ftp> cd test
250 CWD command successful

ftp> pwd
257 "/test" is current directory.

ftp> delete Coding_Standard.txt 
250 File deleted successfully.

ftp> rename hello_world.docx hello.docx      
350 File exists, ready for destination name.
250 File or directory renamed successfully.

ftp> put Coding_Standard.txt
ClamAV response for Coding_Standard.txt : OK
Uploading: [=====================================================] 100.00% 
Uploaded: Coding_Standard.txt

ftp> mput new.pub test.docx 
Prompt is now ON.
Upload file new.pub?    [y/n]: y
Upload file test.docx?    [y/n]: y
ClamAV response for new.pub : OK
Uploading: [=====================================================] 100.00%
Uploaded: new.pub
ClamAV response for test.docx : OK
Uploading: [=====================================================] 100.00%
Uploaded: test.docx

ftp> get hello.docx
Downloading: 0.35 MB received
Downloaded: hello.docx

ftp> mget get1.xlsx get2.pptx
Prompt is now ON.
Download file get1.xlsx?    [y/n]: y
Download file get2.pptx?    [y/n]: y
Downloading: 7.34 MB received
Downloaded: get1.xlsx
Downloading: 0.76 MB received
Downloaded: get2.pptx 

ftp> prompt
Prompt is now OFF

ftp> ascii
Switched to ASCII mode.
200 Type set to A

ftp> binary
Switched to BINARY mode.
200 Type set to I

ftp> close
Disconnected from FTP server.

ftp> open
Connected successfully.

ftp> status
FTP Server: Connected

ftp> passive
Passive mode is always ON in this implementation.

ftp> help
Supported commands:
1: ls, cd <dir>, pwd,
mkdir <dir>, rmdir <dir>, delete <file>, rename <from> <to>,
get <file>, put <file>, mget <file...>, mput <file...>;
2: prompt             - Toggle interactive mode;
3: ascii / binary     - Switch file mode;
4: open / close       - Connect/disconnect server;
5: status             - Show current connection status;
6: passive            - Toggle passive mode (simulated);
7: quit / bye         - Exit;

ftp> quit
```

---

## ⚙️ Hướng dẫn cài đặt ClamAV chi tiết

### 1. Tải và settup ClamAV 

- Truy cập link sau và tải file .zip phù hợp: [https://www.clamav.net/downloads](https://www.clamav.net/downloads).
- Click chuột phải chọn giải nén file .zip vừa tải về.
- Vào folder vừa giải nén và chuyển 2 file `clamd.conf.sample`, `freshclam.conf.sample` ra khỏi folder `conf_examples`. Đồng thời đổi tên thành `clamd.conf`, `freshclam.conf`
- Mở lần lượt `clamd.conf`, `freshclam.conf` và xóa chữ `Example` (không được commend)
- Tạo file `clamscan.exe` bằng lệnh sau:
```bash
./freshclam.exe
```
- Kiểm tra lại:
```bash
clamscan --version
```

- Clip tham khảo: [https://www.youtube.com/watch?v=9gQXBUJbSHE&t=1s](https://www.youtube.com/watch?v=9gQXBUJbSHE&t=1s).

### 2. Thêm ClamAV vào PATH

- Mở Start Menu tìm kiểm `Edit the system environment variables` > `Environment Variables...`.
- Trong `System variables` > variable `PATH` > new > gắn đường dẫn đển file clamscan.exe ở bước 1 > OK > OK > OK. 

### 3. Kiểm tra

- Mở cmd ở bất kì đường dẫn nào và chạy lệnh:
```bash
clamscan --version
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

- Khi muốn gửi các file từ user -> server thì BẮT BUỘC các file phải ở trong cùng thư mục với file thực thi `ftp_client.exe`. Đồng thời các file được tải về từ server -> user cũng sẽ ở trong đó.

- *CHẮC CHẮN* rằng `FTP Server` và `clamav_agent.exe` (cũng như `clamscan.exe`) hoạt động trước khi thực thi `ftp_client.exe` để tránh lỗi không cần thiết.