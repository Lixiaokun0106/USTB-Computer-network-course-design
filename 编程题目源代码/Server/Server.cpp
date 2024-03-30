#include "server.h"

Server::Server() {
    WSADATA wsaData;
    // 创建日志文件
    log.open("serverLog.txt", ios::app);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "错误：无法初始化Winsock。" << std::endl;
        exit(EXIT_FAILURE);
    }
    //初始化成功
    else { std::cout << "Winsock初始化成功。" << std::endl; }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "错误：无法创建套接字。" << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    //创建套接字成功
	else { std::cout << "套接字创建成功。" << std::endl; }


    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "错误：无法绑定套接字。" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    //绑定套接字成功
    else { std::cout << "套接字绑定成功。" << std::endl; }


    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "错误：无法监听套接字。" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    //监听套接字成功
	else { std::cout << "正在监听套接字。" << std::endl; }

    log << getCurrentTimeAsString() << "：服务器初始化完毕。" <<std::endl;
    log.close();
}

void Server::acceptConnections() {
    SOCKET clientSocket;
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    while (true) {
        clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "错误：无法接受连接。" << std::endl;
            continue;
        }

        std::cout << "已接受来自 " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " 的连接。" << std::endl;

        log.open("serverLog.txt", ios::app);
        log << getCurrentTimeAsString() << "：已接受来自 " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " 的连接。" << std::endl;
        log.close();

        //添加该用户进入用户列表
        clientList.push_back(clientSocket);
        //添加该用户进入用户文件列表
        userFilesMap[clientSocket] = std::vector<std::string>();

        //初始化服务端文件(并不是真正的初始化，而是根据不同的用户对userFileMap进行维护
        InitFileList(clientSocket);
        log.open("serverLog.txt", ios::app);
        log << getCurrentTimeAsString() << "：初始化服务端文件列表。"<<std::endl;
        log.close();


        //更新所有用户的文件列表
        refreshAllFiles();

        //创建子线程处理该用户
        std::thread clientThread(&Server::handleClient, this, clientSocket);
        clientThread.detach();

    }
}

// 第一次与客户建立连接时同步文件列表和加密后的文件
void Server::InitFileList(SOCKET clientSocket)
{
    // 接收客户的文件数量
    int numFiles;
    recv(clientSocket, (char*)&numFiles, sizeof(int), 0);

    // 发送确认
    sendConfirm(clientSocket);
    
    // 清空用户文件列表
    userFilesMap[clientSocket].clear();

    // 循环接收文件
    for (int i = 0; i < numFiles; i++)
    {
        RecvFile(clientSocket);
    }

}

void Server::handleClient(SOCKET clientSocket) {
    //在无限循环中接受来自client的文件数据，并转发给其他client
    while (true)
    {
        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            // 客户端断开连接，从客户端列表中移除并结束循环
            clientList.erase(std::remove(clientList.begin(), clientList.end(), clientSocket), clientList.end());
            // 客户端断开连接，从用户文件列表中移除
            userFilesMap.erase(clientSocket);
            break;
        }
        // 发送确认
        sendConfirm(clientSocket);

        // 将接收到的数据转换为字符串
        std::string receivedMessage(buffer, bytesReceived);

        // 如果收到的消息是"send"，则进行发送操作
        if (receivedMessage == "send") {
            // 进行相应操作
            // 接收文件名
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
            // 记录文件名
            std::string fileName = buffer;
            int temp = 0;
            
            log.open("serverLog.txt", ios::app);
            log << getCurrentTimeAsString() << "："<<clientSocket<<"进行send操作，发送文件："<<fileName <<"。" << std::endl;
            log.close();

            //如果文件名已经在该用户文件列表中，则temp改为1
            for (auto& file : userFilesMap[clientSocket])
            {
                if (file == fileName)
                {
					temp = 1;
					break;
				}
			}

            // 发送确认
            sendConfirm(clientSocket);

            // 接收文件
            RecvFile(clientSocket);

            // 如果该文件确实是新文件，则不从文件列表中删除，否则删除最后一个文件
            if (temp == 1)
            {
                userFilesMap[clientSocket].pop_back();
            }
            //刷新所有用户文件列表
            refreshAllFiles();

            txtToShowFileList();

        }
        // 如果接收到的消息是"flush"，则刷新用户界面，更新其文件列表
        else if (receivedMessage == "flush")
        {
            // 刷新用户文件列表
            InitFileList(clientSocket);

            // 更新所有用户文件列表
            refreshAllFiles();

            // 统计需要向当前用户发送的文件数量
            int numFilesToSend = 0;
            for (auto& fileName : allFiles)
            {
				numFilesToSend++;
			}
            cout<<numFilesToSend<<endl;
            // 将需要向当前用户发送的文件数量发送给当前用户
            send(clientSocket, (char*)&numFilesToSend, sizeof(int), 0);
            cout<<"已发送文件数量，等待确认...."<<endl;
            // 等待确认
            recvConfirm(clientSocket);
            
            // 清空当前用户的文件列表
            //userFilesMap[clientSocket].clear();

            // 向当前用户发送allFiles
            for (auto& fileName : allFiles)
            {
                // 将该文件添加进入当前用户的文件列表
				//userFilesMap[clientSocket].push_back(fileName);

				// 发送该文件给当前用户
				SendFile(clientSocket, fileName);
            }
            log.open("serverLog.txt", ios::app);
            log << getCurrentTimeAsString() << "：" << clientSocket << "进行flush操作，发送其全部文件并更新文件列表。" << std::endl;
            log.close();

            // 刷新所有用户文件列表
            refreshAllFiles();

            txtToShowFileList();
        }
        // 如果是"exit",则断开连接
        else if (receivedMessage == "exit")
        {
            txtToShowFileList();
            break;
        }
        // 如果是"show"，不做处理
        else if (receivedMessage == "show")
        {
            txtToShowFileList();
            // 记录日志
            log.open("serverLog.txt", ios::app);
            log << getCurrentTimeAsString() << "：" << clientSocket << "进行show操作，显示文件列表。" << std::endl;
            log.close();

            continue;
        }
    }

    // 客户端断开连接时，从客户端列表中移除
    clientList.erase(std::remove(clientList.begin(), clientList.end(), clientSocket), clientList.end());
    // 客户端断开连接，从用户文件列表中移除
    userFilesMap.erase(clientSocket);

    // 关闭套接字
    closesocket(clientSocket);
    log.open("serverLog.txt", ios::app);
    log << getCurrentTimeAsString() << "：" << clientSocket << "与服务器断开连接。" << std::endl;
    log.close();

}

// 获取当前目录下的文件列表
std::vector<std::string> Server::getFilesInCurrentDirectory() {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        if (entry.is_regular_file()) {
            std::string fileName = entry.path().filename().string();
            if (fileName != "Server.exe" and fileName != "serverLog.txt") {
                files.push_back(fileName);
            }
        }
    }
    return files;
}

void Server::SendFile(SOCKET clientSocket, string fileName)
{
    char buffer[BUFFER_SIZE] = { 0 };
    int bytesRead;

    // 发送文件名
    strncpy_s(buffer, fileName.c_str(), BUFFER_SIZE - 1);  // 留出一个位置给终止符
    buffer[BUFFER_SIZE - 1] = '\0';  // 手动添加终止符

    send(clientSocket, buffer, strlen(buffer), 0);

    // 清空 buffer
    memset(buffer, 0, BUFFER_SIZE);

    // 等待确认
    recvConfirm(clientSocket);

    // 发送文件大小
    struct stat statbuf;
    stat((fileName).c_str(), &statbuf);
    std::string fileSize = std::to_string(statbuf.st_size);
    std::cout<<fileSize<<std::endl;
    strncpy_s(buffer, fileSize.c_str(), fileSize.size());
    send(clientSocket, buffer, strlen(buffer), 0);

    // 清空 buffer
    memset(buffer, 0, BUFFER_SIZE);

    // 等待确认
    recvConfirm(clientSocket);

    // 发送文件内容
    std::ifstream file(fileName, std::ios::in | std::ios::out | std::ios::binary);
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

    // 等待确认
    recvConfirm(clientSocket);
}


void Server::RecvFile(SOCKET clientSocket)
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
    sendConfirm(clientSocket);

    // 添加文件名到用户文件列表
    userFilesMap[clientSocket].push_back(fileName);

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
    sendConfirm(clientSocket);

    // 接收文件内容
    std::ofstream file(fileName, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "错误：无法打开文件。" << std::endl;
        return;
    }

    // 接收文件内容
    int totalBytesReceived = 0;
    while (totalBytesReceived < fileSize) {
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "错误：无法接收文件内容。" << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "文件接收完毕：" << fileName << std::endl;
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
        std::cout << totalBytesReceived<<"   "<<fileSize<<std::endl;
    }
    else {
        std::cerr << "错误：文件接收失败：" << fileName << std::endl;
        std::cout << totalBytesReceived << "   " << fileSize << std::endl;
    }
    file.close();

    // 发送确认
    sendConfirm(clientSocket);
}
// 根据用户文件列表刷新所有用户的文件列表
void Server::refreshAllFiles()
{
    // 清空 allFiles
	allFiles.clear();

	// 遍历所有用户的文件列表，将文件名添加到 allFiles
    for (auto& userFiles : userFilesMap)
    {
        for (auto& fileName : userFiles.second)
        {
            // 如果 allFiles 中没有该文件名，则添加
            if (std::find(allFiles.begin(), allFiles.end(), fileName) == allFiles.end())
            {
				allFiles.push_back(fileName);
			}
		}
	}

    //将每个用户的文件列表清空，然后调整至与allFiles一致
    for (auto& userFiles : userFilesMap)
    {
        //清空该用户的文件列表
        userFiles.second.clear();

        for (auto& fileName : allFiles)
        {
            //将allFiles中的文件名添加进入该用户的文件列表
            userFiles.second.push_back(fileName);
        }
    }
}

// 发送确认信息
void Server::sendConfirm(SOCKET clientSocket)
{
    sync++;
    send(clientSocket, (char*)&sync, sizeof(int), 0);

}

// 接收确认信息
void Server::recvConfirm(SOCKET clientSocket)
{
    // 接收sync，用于同步
    int sync1;
    recv(clientSocket, (char*)&sync1, sizeof(int), 0);
    std::cout << "sync: " << sync1 << std::endl;
    sync = sync1;
}

// 获取当前时间的字符串
std::string Server::getCurrentTimeAsString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %X");
    return ss.str();
}

// 将每个用户的文件列表和所有用户文件列表写入txt文件
void Server::txtToShowFileList()
{
	ofstream file("fileListRecord.txt", ios::app);
    file << "----------------------------------------------" << std::endl;
	file << "所有用户文件列表：" << endl;
    for (auto& fileName : allFiles)
    {
		file << fileName << endl;
	}
	file << endl;
	file << "每个用户的文件列表：" << endl;
    for (auto& userFiles : userFilesMap)
    {
		file << "用户" << userFiles.first << "的文件列表：" << endl;
        for (auto& fileName : userFiles.second)
        {
			file << fileName << endl;
		}
		file << endl;
	}
    file << "----------------------------------------------" << std::endl;
	file.close();
}
