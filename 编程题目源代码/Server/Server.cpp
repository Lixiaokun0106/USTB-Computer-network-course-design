#include "server.h"

Server::Server() {
    WSADATA wsaData;
    // ������־�ļ�
    log.open("serverLog.txt", ios::app);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "�����޷���ʼ��Winsock��" << std::endl;
        exit(EXIT_FAILURE);
    }
    //��ʼ���ɹ�
    else { std::cout << "Winsock��ʼ���ɹ���" << std::endl; }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "�����޷������׽��֡�" << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    //�����׽��ֳɹ�
	else { std::cout << "�׽��ִ����ɹ���" << std::endl; }


    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "�����޷����׽��֡�" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    //���׽��ֳɹ�
    else { std::cout << "�׽��ְ󶨳ɹ���" << std::endl; }


    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "�����޷������׽��֡�" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    //�����׽��ֳɹ�
	else { std::cout << "���ڼ����׽��֡�" << std::endl; }

    log << getCurrentTimeAsString() << "����������ʼ����ϡ�" <<std::endl;
    log.close();
}

void Server::acceptConnections() {
    SOCKET clientSocket;
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    while (true) {
        clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "�����޷��������ӡ�" << std::endl;
            continue;
        }

        std::cout << "�ѽ������� " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " �����ӡ�" << std::endl;

        log.open("serverLog.txt", ios::app);
        log << getCurrentTimeAsString() << "���ѽ������� " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " �����ӡ�" << std::endl;
        log.close();

        //��Ӹ��û������û��б�
        clientList.push_back(clientSocket);
        //��Ӹ��û������û��ļ��б�
        userFilesMap[clientSocket] = std::vector<std::string>();

        //��ʼ��������ļ�(�����������ĳ�ʼ�������Ǹ��ݲ�ͬ���û���userFileMap����ά��
        InitFileList(clientSocket);
        log.open("serverLog.txt", ios::app);
        log << getCurrentTimeAsString() << "����ʼ��������ļ��б�"<<std::endl;
        log.close();


        //���������û����ļ��б�
        refreshAllFiles();

        //�������̴߳�����û�
        std::thread clientThread(&Server::handleClient, this, clientSocket);
        clientThread.detach();

    }
}

// ��һ����ͻ���������ʱͬ���ļ��б�ͼ��ܺ���ļ�
void Server::InitFileList(SOCKET clientSocket)
{
    // ���տͻ����ļ�����
    int numFiles;
    recv(clientSocket, (char*)&numFiles, sizeof(int), 0);

    // ����ȷ��
    sendConfirm(clientSocket);
    
    // ����û��ļ��б�
    userFilesMap[clientSocket].clear();

    // ѭ�������ļ�
    for (int i = 0; i < numFiles; i++)
    {
        RecvFile(clientSocket);
    }

}

void Server::handleClient(SOCKET clientSocket) {
    //������ѭ���н�������client���ļ����ݣ���ת��������client
    while (true)
    {
        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            // �ͻ��˶Ͽ����ӣ��ӿͻ����б����Ƴ�������ѭ��
            clientList.erase(std::remove(clientList.begin(), clientList.end(), clientSocket), clientList.end());
            // �ͻ��˶Ͽ����ӣ����û��ļ��б����Ƴ�
            userFilesMap.erase(clientSocket);
            break;
        }
        // ����ȷ��
        sendConfirm(clientSocket);

        // �����յ�������ת��Ϊ�ַ���
        std::string receivedMessage(buffer, bytesReceived);

        // ����յ�����Ϣ��"send"������з��Ͳ���
        if (receivedMessage == "send") {
            // ������Ӧ����
            // �����ļ���
            char buffer[BUFFER_SIZE] = { 0 };
            int bytesRead;
            bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytesRead == SOCKET_ERROR) {
				std::cerr << "�����޷������ļ�����" << std::endl;
				return;
			}
            else if (bytesRead == 0) {
				std::cout << "�������ѶϿ����ӡ�" << std::endl;
				return;
			}
            // ��¼�ļ���
            std::string fileName = buffer;
            int temp = 0;
            
            log.open("serverLog.txt", ios::app);
            log << getCurrentTimeAsString() << "��"<<clientSocket<<"����send�����������ļ���"<<fileName <<"��" << std::endl;
            log.close();

            //����ļ����Ѿ��ڸ��û��ļ��б��У���temp��Ϊ1
            for (auto& file : userFilesMap[clientSocket])
            {
                if (file == fileName)
                {
					temp = 1;
					break;
				}
			}

            // ����ȷ��
            sendConfirm(clientSocket);

            // �����ļ�
            RecvFile(clientSocket);

            // ������ļ�ȷʵ�����ļ����򲻴��ļ��б���ɾ��������ɾ�����һ���ļ�
            if (temp == 1)
            {
                userFilesMap[clientSocket].pop_back();
            }
            //ˢ�������û��ļ��б�
            refreshAllFiles();

            txtToShowFileList();

        }
        // ������յ�����Ϣ��"flush"����ˢ���û����棬�������ļ��б�
        else if (receivedMessage == "flush")
        {
            // ˢ���û��ļ��б�
            InitFileList(clientSocket);

            // ���������û��ļ��б�
            refreshAllFiles();

            // ͳ����Ҫ��ǰ�û����͵��ļ�����
            int numFilesToSend = 0;
            for (auto& fileName : allFiles)
            {
				numFilesToSend++;
			}
            cout<<numFilesToSend<<endl;
            // ����Ҫ��ǰ�û����͵��ļ��������͸���ǰ�û�
            send(clientSocket, (char*)&numFilesToSend, sizeof(int), 0);
            cout<<"�ѷ����ļ��������ȴ�ȷ��...."<<endl;
            // �ȴ�ȷ��
            recvConfirm(clientSocket);
            
            // ��յ�ǰ�û����ļ��б�
            //userFilesMap[clientSocket].clear();

            // ��ǰ�û�����allFiles
            for (auto& fileName : allFiles)
            {
                // �����ļ���ӽ��뵱ǰ�û����ļ��б�
				//userFilesMap[clientSocket].push_back(fileName);

				// ���͸��ļ�����ǰ�û�
				SendFile(clientSocket, fileName);
            }
            log.open("serverLog.txt", ios::app);
            log << getCurrentTimeAsString() << "��" << clientSocket << "����flush������������ȫ���ļ��������ļ��б�" << std::endl;
            log.close();

            // ˢ�������û��ļ��б�
            refreshAllFiles();

            txtToShowFileList();
        }
        // �����"exit",��Ͽ�����
        else if (receivedMessage == "exit")
        {
            txtToShowFileList();
            break;
        }
        // �����"show"����������
        else if (receivedMessage == "show")
        {
            txtToShowFileList();
            // ��¼��־
            log.open("serverLog.txt", ios::app);
            log << getCurrentTimeAsString() << "��" << clientSocket << "����show��������ʾ�ļ��б�" << std::endl;
            log.close();

            continue;
        }
    }

    // �ͻ��˶Ͽ�����ʱ���ӿͻ����б����Ƴ�
    clientList.erase(std::remove(clientList.begin(), clientList.end(), clientSocket), clientList.end());
    // �ͻ��˶Ͽ����ӣ����û��ļ��б����Ƴ�
    userFilesMap.erase(clientSocket);

    // �ر��׽���
    closesocket(clientSocket);
    log.open("serverLog.txt", ios::app);
    log << getCurrentTimeAsString() << "��" << clientSocket << "��������Ͽ����ӡ�" << std::endl;
    log.close();

}

// ��ȡ��ǰĿ¼�µ��ļ��б�
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

    // �����ļ���
    strncpy_s(buffer, fileName.c_str(), BUFFER_SIZE - 1);  // ����һ��λ�ø���ֹ��
    buffer[BUFFER_SIZE - 1] = '\0';  // �ֶ������ֹ��

    send(clientSocket, buffer, strlen(buffer), 0);

    // ��� buffer
    memset(buffer, 0, BUFFER_SIZE);

    // �ȴ�ȷ��
    recvConfirm(clientSocket);

    // �����ļ���С
    struct stat statbuf;
    stat((fileName).c_str(), &statbuf);
    std::string fileSize = std::to_string(statbuf.st_size);
    std::cout<<fileSize<<std::endl;
    strncpy_s(buffer, fileSize.c_str(), fileSize.size());
    send(clientSocket, buffer, strlen(buffer), 0);

    // ��� buffer
    memset(buffer, 0, BUFFER_SIZE);

    // �ȴ�ȷ��
    recvConfirm(clientSocket);

    // �����ļ�����
    std::ifstream file(fileName, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "�����޷����ļ���" << std::endl;
        return;
    }

    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);

        int bytesSent = send(clientSocket, buffer, file.gcount(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "�����޷������ļ����ݡ�" << std::endl;
            break;
        }
    }

    file.close();

    // �ȴ�ȷ��
    recvConfirm(clientSocket);
}


void Server::RecvFile(SOCKET clientSocket)
{
    // �ļ���
    char buffer[BUFFER_SIZE] = { 0 };
    int bytesRead;

    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead == SOCKET_ERROR) {
        std::cerr << "�����޷������ļ�����" << std::endl;
        return;
    }
    else if (bytesRead == 0) {
        std::cout << "�������ѶϿ����ӡ�" << std::endl;
        return;
    }

    std::string fileName = buffer;
    std::cout << "�������ļ���" << fileName << std::endl;

    // ����ȷ��
    sendConfirm(clientSocket);

    // ����ļ������û��ļ��б�
    userFilesMap[clientSocket].push_back(fileName);

    // ��� buffer
    memset(buffer, 0, BUFFER_SIZE);

    // �ļ���С
    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead == SOCKET_ERROR) {
        std::cerr << "�����޷����մ�С��" << std::endl;
        return;
    }
    else if (bytesRead == 0) {
        std::cout << "�������ѶϿ����ӡ�" << std::endl;
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

    // ��� buffer
    memset(buffer, 0, BUFFER_SIZE);

    // ����ȷ��
    sendConfirm(clientSocket);

    // �����ļ�����
    std::ofstream file(fileName, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "�����޷����ļ���" << std::endl;
        return;
    }

    // �����ļ�����
    int totalBytesReceived = 0;
    while (totalBytesReceived < fileSize) {
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "�����޷������ļ����ݡ�" << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "�ļ�������ϣ�" << fileName << std::endl;
            break;
        }

        totalBytesReceived += bytesRead;
        // д���ļ�
        file.write(buffer, bytesRead);

        // ��� buffer
        memset(buffer, 0, BUFFER_SIZE);
    }

    if (totalBytesReceived == fileSize) {
        std::cout << "�ļ����ճɹ���" << fileName << std::endl;
        std::cout << totalBytesReceived<<"   "<<fileSize<<std::endl;
    }
    else {
        std::cerr << "�����ļ�����ʧ�ܣ�" << fileName << std::endl;
        std::cout << totalBytesReceived << "   " << fileSize << std::endl;
    }
    file.close();

    // ����ȷ��
    sendConfirm(clientSocket);
}
// �����û��ļ��б�ˢ�������û����ļ��б�
void Server::refreshAllFiles()
{
    // ��� allFiles
	allFiles.clear();

	// ���������û����ļ��б����ļ�����ӵ� allFiles
    for (auto& userFiles : userFilesMap)
    {
        for (auto& fileName : userFiles.second)
        {
            // ��� allFiles ��û�и��ļ����������
            if (std::find(allFiles.begin(), allFiles.end(), fileName) == allFiles.end())
            {
				allFiles.push_back(fileName);
			}
		}
	}

    //��ÿ���û����ļ��б���գ�Ȼ���������allFilesһ��
    for (auto& userFiles : userFilesMap)
    {
        //��ո��û����ļ��б�
        userFiles.second.clear();

        for (auto& fileName : allFiles)
        {
            //��allFiles�е��ļ�����ӽ�����û����ļ��б�
            userFiles.second.push_back(fileName);
        }
    }
}

// ����ȷ����Ϣ
void Server::sendConfirm(SOCKET clientSocket)
{
    sync++;
    send(clientSocket, (char*)&sync, sizeof(int), 0);

}

// ����ȷ����Ϣ
void Server::recvConfirm(SOCKET clientSocket)
{
    // ����sync������ͬ��
    int sync1;
    recv(clientSocket, (char*)&sync1, sizeof(int), 0);
    std::cout << "sync: " << sync1 << std::endl;
    sync = sync1;
}

// ��ȡ��ǰʱ����ַ���
std::string Server::getCurrentTimeAsString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %X");
    return ss.str();
}

// ��ÿ���û����ļ��б�������û��ļ��б�д��txt�ļ�
void Server::txtToShowFileList()
{
	ofstream file("fileListRecord.txt", ios::app);
    file << "----------------------------------------------" << std::endl;
	file << "�����û��ļ��б�" << endl;
    for (auto& fileName : allFiles)
    {
		file << fileName << endl;
	}
	file << endl;
	file << "ÿ���û����ļ��б�" << endl;
    for (auto& userFiles : userFilesMap)
    {
		file << "�û�" << userFiles.first << "���ļ��б�" << endl;
        for (auto& fileName : userFiles.second)
        {
			file << fileName << endl;
		}
		file << endl;
	}
    file << "----------------------------------------------" << std::endl;
	file.close();
}
