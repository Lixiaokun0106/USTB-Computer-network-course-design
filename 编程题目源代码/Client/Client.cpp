#include "client.h"

Client::Client() {
    WSADATA wsaData;
    // 创建日志文件
    log.open("clientLog.txt", ios::app);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "错误：无法初始化Winsock。" << std::endl;
        exit(EXIT_FAILURE);
    }
    else{std::cout << "Winsock 初始化成功。" << std::endl;}

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "错误：无法创建套接字。" << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else{std::cout << "套接字创建成功。" << std::endl;}

    log << getCurrentTimeAsString() << "：客户端初始化完毕。"<<std::endl;
    log.close();
}

void Client::connectToServer() {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    serverAddr.sin_port = htons(PORT);

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "错误：无法连接到服务器。" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    std::cout << "已连接到服务器。" << std::endl;
    log.open("clientLog.txt", ios::app);
    log << getCurrentTimeAsString() << "：已连接到服务器。" << std::endl;
    log.close();

    // 初始化发送文件
    cout<< "初始化发送文件..." << endl;
    InitSendFile();
    cout << "初始化发送文件完成。" << endl;
    
    log.open("clientLog.txt", ios::app);
    log << getCurrentTimeAsString() << "：初始化发送文件完成。" << std::endl;
    log.close();


    //无限循环处理用户命令
    while (true)
    {
        std::string command;
		std::cout << "请输入命令：";
		std::cin >> command;

		if (command == "exit") {
            log.open("clientLog.txt", ios::app);
            log << getCurrentTimeAsString() << "：退出客户端。" << std::endl;
            log.close();

			break;
		}
		else if (command == "send") {
            // 发送"send"字段，告知服务端接下来的操作
            std::string message = "send";
            send(clientSocket, message.c_str(), message.size(), 0);

            // 等待确认
            recvConfirm();

            // 输入文件名
            std::string fileName;
            std::cout << "请输入文件名：";
            std::cin >> fileName;

            // 发送文件名
            send(clientSocket, fileName.c_str(), fileName.size(), 0);

            // 等待确认
            recvConfirm();

            // 将该文件发送给服务端
            sendOneFile(fileName);

            log.open("clientLog.txt", ios::app);
            log << getCurrentTimeAsString() << "：进行send操作，发送文件：" << fileName<<"。" << std::endl;
            log.close();

        }
        else if (command == "show") {
            //查看当前目录下的文件列表
            std::cout << "----------------------------------当前目录----------------------------------" << std::endl;
            system("dir");
            std::cout << "----------------------------------------------------------------------------" << std::endl;
            
            log.open("clientLog.txt", ios::app);
            log << getCurrentTimeAsString() << "：进行show操作，查看本地文件列表。" << std::endl;
            log.close();
        }
        else if (command == "flush")
        {
            // 要完成2个任务：1、根据当前目录下的文件，发送新文件给服务端；2、接收服务端的新文件
            // 发送“flush”字段，告知服务端接下来的操作
            std::string message = "flush";
            send(clientSocket, message.c_str(), message.size(), 0);

            // 等待确认
            recvConfirm();

            // 更新列表并发送相应文件
            InitSendFile();
            
            // 接下来是接收有关的操作
            // 收到即将接收文件的数量
            int numFiles;
            cout<< "准备接收文件数量..." << endl;
            recv(clientSocket, (char*)&numFiles, sizeof(int), 0);
            std::cout << numFiles << endl;
            // 发送确认
            sendConfirm();

            // 循环接收文件
            for (int i = 0; i < numFiles; i++)
            {
                recvOneFile();
            }
            log.open("clientLog.txt", ios::app);
            log << getCurrentTimeAsString() << "：进行flush操作，发送全部文件到服务端并更新文件列表。" << std::endl;
            log.close();

        }
		else {
			std::cerr << "错误：无效的命令。" << std::endl;
		}
	}
	closesocket(clientSocket);
	WSACleanup();
	std::cout << "已断开连接。" << std::endl;
}


void Client::sendFile(std::string fileName) {
    char buffer[BUFFER_SIZE] = {0};
    int bytesRead;

    // 发送文件名
    strncpy_s(buffer, fileName.c_str(), BUFFER_SIZE - 1);  // 留出一个位置给终止符
    buffer[BUFFER_SIZE - 1] = '\0';  // 手动添加终止符

    send(clientSocket, buffer, strlen(buffer), 0);

    // 清空 buffer
    memset(buffer, 0, BUFFER_SIZE);

    // 将文件进行加密与编码
    aes.encryptAndEncode(aes.key, fileName, fileName + ".enc");


    // 发送文件大小
    struct stat statbuf;
    stat((fileName+".enc").c_str(), &statbuf);
    std::string fileSize = std::to_string(statbuf.st_size);
    
    strncpy_s(buffer, fileSize.c_str(), fileSize.size());
    send(clientSocket, buffer, strlen(buffer), 0);
    
    std::cout << fileSize << std::endl;
    std::cout << buffer << std::endl;

    // 清空 buffer
    memset(buffer, 0, BUFFER_SIZE);

    // 发送文件内容
    std::ifstream file(fileName+".enc", std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "错误：无法打开文件。" << std::endl;
        return;
    }

    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
                
        int bytesSent = send(clientSocket, buffer, file.gcount(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "错误：无法发送文件内容。" << std::endl;
            break;
        }
    }

    file.close();

    // 删除加密文件
    std::string deleteCommand = "del " + fileName + ".enc";
    system(deleteCommand.c_str());

}

void Client::recvFile() {

    while (true) {
        // 文件名
        char buffer[BUFFER_SIZE] = { 0 };
        int bytesRead;

        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "错误：无法接收文件名。" << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "服务器已断开连接。" << std::endl;
            break;
        }

        std::string fileName = buffer;
        std::cout << "将接收文件：" << fileName << std::endl;

        // 清空 buffer
        memset(buffer, 0, BUFFER_SIZE);

        // 文件大小
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "错误：无法接收大小。" << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "服务器已断开连接。" << std::endl;
            break;
        }
        // 将接收到的数据转换为整数
        int fileSize = std::stoi(std::string(buffer, bytesRead));
        int circle_num = fileSize / BUFFER_SIZE;
        int last_num = fileSize % BUFFER_SIZE;

        if (last_num != 0)
        {
            circle_num++;
        }
        std::cout << circle_num << std::endl;

        // 清空 buffer
        memset(buffer, 0, BUFFER_SIZE);


        // 接收文件内容
        std::ofstream file(fileName + ".enc", std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "错误：无法打开文件。" << std::endl;
            return;
        }

        int totalBytesReceived = 0;
        while (totalBytesReceived < fileSize) {
            bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytesRead == SOCKET_ERROR) {
                std::cerr << "错误：无法接收文件内容。" << std::endl;
                break;
            }
            else if (bytesRead == 0) {
                std::cout << "文件接收完毕：" << fileName + ".enc" << std::endl;
                break;
            }

            totalBytesReceived += bytesRead;
            // 写入文件
            file.write(buffer, bytesRead);

            // 清空 buffer
            memset(buffer, 0, BUFFER_SIZE);
        }

        if (totalBytesReceived == fileSize) {
            std::cout << "文件接收成功：" << fileName << std::endl;
            std::cout << totalBytesReceived << "   " << fileSize << std::endl;
        }
        else {
            std::cerr << "错误：文件接收失败：" << fileName << std::endl;
            std::cout << totalBytesReceived << "   " << fileSize << std::endl;
        }

        //std::cout << "文件接收成功：" << fileName << std::endl;
        file.close();

        // 对接收的文件进行解码与解密
        aes.decodeAndDecrypt(aes.key, fileName + ".enc", fileName);

        // 删除加密文件
        std::string deleteCommand = "del " + fileName + ".enc";
        system(deleteCommand.c_str());

    }
}

// 获取当前目录下的文件列表
std::vector<std::string> Client::getFilesInCurrentDirectory() {
    std::vector<std::string> files;
    std::string forbid_name1 = "Client.exe";
    std::string forbid_name2 = "clientLog.txt";
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        if (entry.is_regular_file()) {
            std::string fileName = entry.path().filename().string();
            if (fileName.compare(forbid_name1) != 0 and fileName.compare(forbid_name2) != 0) {
                files.push_back(fileName);
            }
        }
    }
    return files;
}

// 初始化发送文件
void Client::InitSendFile()
{
    // 获取当前目录下的文件列表
	files = getFilesInCurrentDirectory();

    // 打印文件列表
    std::cout << "------------------文件列表-----------------------------" << std::endl;
    for (auto file : files)
    {
        std::cout << file << std::endl;
    }
    std::cout << "-------------------------------------------------------" << std::endl;

    // 发送文件数量
    int numFiles = files.size();
    send(clientSocket, (char*)&numFiles, sizeof(int), 0);

    // 等待确认
    recvConfirm();
    cout << "目录下文件数量：" << files.size() << endl;
	// 发送文件
    for (const auto& file : files) {
		sendOneFile(file);
	}
    cout << "目录下文件数量：" << files.size() << endl;
}

void Client::sendOneFile(std::string fileName)
{
    char buffer[BUFFER_SIZE] = { 0 };
    int bytesRead;

    // 发送文件名
    strncpy_s(buffer, fileName.c_str(), BUFFER_SIZE - 1);  // 留出一个位置给终止符
    buffer[BUFFER_SIZE - 1] = '\0';  // 手动添加终止符
    send(clientSocket, buffer, strlen(buffer), 0);

    // 等待确认
    recvConfirm();

    // 清空 buffer
    memset(buffer, 0, BUFFER_SIZE);

    // 将文件进行加密与编码
    aes.encryptAndEncode(aes.key, fileName, fileName + ".enc");


    // 发送文件大小
    struct stat statbuf;
    stat((fileName + ".enc").c_str(), &statbuf);
    std::string fileSize = std::to_string(statbuf.st_size);

    strncpy_s(buffer, fileSize.c_str(), fileSize.size());
    send(clientSocket, buffer, strlen(buffer), 0);

    std::cout << fileSize << std::endl;
    std::cout << buffer << std::endl;

    // 等待确认
    recvConfirm();

    // 清空 buffer
    memset(buffer, 0, BUFFER_SIZE);

    // 发送文件内容
    std::ifstream file(fileName + ".enc", std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "错误：无法打开文件。" << std::endl;
        return;
    }

    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);

        int bytesSent = send(clientSocket, buffer, file.gcount(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "错误：无法发送文件内容。" << std::endl;
            break;
        }
    }

    file.close();

    // 删除加密文件
    std::string deleteCommand = "del " + fileName + ".enc";
    system(deleteCommand.c_str());

    // 等待确认
    recvConfirm();

}
void Client::recvOneFile()
{
    // 文件名
    char buffer[BUFFER_SIZE] = { 0 };
    int bytesRead;

    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead == SOCKET_ERROR) {
        std::cerr << "错误：无法接收文件名。" << std::endl;
        return;
    }
    else if (bytesRead == 0) {
        std::cout << "服务器已断开连接。" << std::endl;
        return;
    }

    std::string fileName = buffer;
    std::cout << "将接收文件：" << fileName << std::endl;

    // 发送确认
    sendConfirm();

    // 清空 buffer
    memset(buffer, 0, BUFFER_SIZE);

    // 文件大小
    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead == SOCKET_ERROR) {
        std::cerr << "错误：无法接收大小。" << std::endl;
        return;
    }
    else if (bytesRead == 0) {
        std::cout << "服务器已断开连接。" << std::endl;
        return;
    }
    // 将接收到的数据转换为整数
    int fileSize = std::stoi(std::string(buffer, bytesRead));
    int circle_num = fileSize / BUFFER_SIZE;
    int last_num = fileSize % BUFFER_SIZE;

    if (last_num != 0)
    {
        circle_num++;
    }
    std::cout << circle_num << std::endl;

    // 清空 buffer
    memset(buffer, 0, BUFFER_SIZE);

    // 发送确认
    sendConfirm();

    // 接收文件内容
    std::ofstream file(fileName + ".enc", std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "错误：无法打开文件。" << std::endl;
        return;
    }

    int totalBytesReceived = 0;
    while (totalBytesReceived < fileSize) {
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "错误：无法接收文件内容。" << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "文件接收完毕：" << fileName + ".enc" << std::endl;
            break;
        }

        totalBytesReceived += bytesRead;
        // 写入文件
        file.write(buffer, bytesRead);

        // 清空 buffer
        memset(buffer, 0, BUFFER_SIZE);
    }

    if (totalBytesReceived == fileSize) {
        std::cout << "文件接收成功：" << fileName << std::endl;
        std::cout << totalBytesReceived << "   " << fileSize << std::endl;
    }
    else {
        std::cerr << "错误：文件接收失败：" << fileName << std::endl;
        std::cout << totalBytesReceived << "   " << fileSize << std::endl;
    }

    file.close();

    // 对接收的文件进行解码与解密
    aes.decodeAndDecrypt(aes.key, fileName + ".enc", fileName);

    // 删除加密文件
    std::string deleteCommand = "del " + fileName + ".enc";
    system(deleteCommand.c_str());

    // 发送确认
    sendConfirm();
}

// 接收文件线程
void Client::recvFileThread() {
    // 无限循环
    while (true) {
        // 接收数据
        int numFiles;
        recv(clientSocket, (char*)&numFiles, sizeof(int), 0);
        // 接收文件
        for (int i = 0; i < numFiles; i++) {
            recvOneFile();
            }
	}

}

// 发送确认信息
void Client::sendConfirm()
{
    sync++;
    send(clientSocket, (char*)&sync, sizeof(int), 0);
}

// 接收确认信息
void Client::recvConfirm()
{
    int sync1;
    recv(clientSocket, (char*)&sync1, sizeof(int), 0);
    std::cout << "sync: " << sync1 << std::endl;
    sync = sync1;
}

// 获取当前时间的字符串
std::string Client::getCurrentTimeAsString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %X");
    return ss.str();
}
