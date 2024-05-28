#include "client.h"

Client::Client() {
    WSADATA wsaData;
    // ������־�ļ�
    log.open("clientLog.txt", ios::app);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "�����޷���ʼ��Winsock��" << std::endl;
        exit(EXIT_FAILURE);
    }
    else{std::cout << "Winsock ��ʼ���ɹ���" << std::endl;}

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "�����޷������׽��֡�" << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else{std::cout << "�׽��ִ����ɹ���" << std::endl;}

    log << getCurrentTimeAsString() << "���ͻ��˳�ʼ����ϡ�"<<std::endl;
    log.close();
}

void Client::connectToServer() {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    serverAddr.sin_port = htons(PORT);

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "�����޷����ӵ���������" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    std::cout << "�����ӵ���������" << std::endl;
    log.open("clientLog.txt", ios::app);
    log << getCurrentTimeAsString() << "�������ӵ���������" << std::endl;
    log.close();

    // ��ʼ�������ļ�
    cout<< "��ʼ�������ļ�..." << endl;
    InitSendFile();
    cout << "��ʼ�������ļ���ɡ�" << endl;
    
    log.open("clientLog.txt", ios::app);
    log << getCurrentTimeAsString() << "����ʼ�������ļ���ɡ�" << std::endl;
    log.close();


    //����ѭ�������û�����
    while (true)
    {
        std::string command;
		std::cout << "���������";
		std::cin >> command;

		if (command == "exit") {
            log.open("clientLog.txt", ios::app);
            log << getCurrentTimeAsString() << "���˳��ͻ��ˡ�" << std::endl;
            log.close();

			break;
		}
		else if (command == "send") {
            // ����"send"�ֶΣ���֪����˽������Ĳ���
            std::string message = "send";
            send(clientSocket, message.c_str(), message.size(), 0);

            // �ȴ�ȷ��
            recvConfirm();

            // �����ļ���
            std::string fileName;
            std::cout << "�������ļ�����";
            std::cin >> fileName;

            // �����ļ���
            send(clientSocket, fileName.c_str(), fileName.size(), 0);

            // �ȴ�ȷ��
            recvConfirm();

            // �����ļ����͸������
            sendOneFile(fileName);

            log.open("clientLog.txt", ios::app);
            log << getCurrentTimeAsString() << "������send�����������ļ���" << fileName<<"��" << std::endl;
            log.close();

        }
        else if (command == "show") {
            //�鿴��ǰĿ¼�µ��ļ��б�
            std::cout << "----------------------------------��ǰĿ¼----------------------------------" << std::endl;
            system("dir");
            std::cout << "----------------------------------------------------------------------------" << std::endl;
            
            log.open("clientLog.txt", ios::app);
            log << getCurrentTimeAsString() << "������show�������鿴�����ļ��б�" << std::endl;
            log.close();
        }
        else if (command == "flush")
        {
            // Ҫ���2������1�����ݵ�ǰĿ¼�µ��ļ����������ļ�������ˣ�2�����շ���˵����ļ�
            // ���͡�flush���ֶΣ���֪����˽������Ĳ���
            std::string message = "flush";
            send(clientSocket, message.c_str(), message.size(), 0);

            // �ȴ�ȷ��
            recvConfirm();

            // �����б�������Ӧ�ļ�
            InitSendFile();
            
            // �������ǽ����йصĲ���
            // �յ����������ļ�������
            int numFiles;
            cout<< "׼�������ļ�����..." << endl;
            recv(clientSocket, (char*)&numFiles, sizeof(int), 0);
            std::cout << numFiles << endl;
            // ����ȷ��
            sendConfirm();

            // ѭ�������ļ�
            for (int i = 0; i < numFiles; i++)
            {
                recvOneFile();
            }
            log.open("clientLog.txt", ios::app);
            log << getCurrentTimeAsString() << "������flush����������ȫ���ļ�������˲������ļ��б�" << std::endl;
            log.close();

        }
		else {
			std::cerr << "������Ч�����" << std::endl;
		}
	}
	closesocket(clientSocket);
	WSACleanup();
	std::cout << "�ѶϿ����ӡ�" << std::endl;
}


void Client::sendFile(std::string fileName) {
    char buffer[BUFFER_SIZE] = {0};
    int bytesRead;

    // �����ļ���
    strncpy_s(buffer, fileName.c_str(), BUFFER_SIZE - 1);  // ����һ��λ�ø���ֹ��
    buffer[BUFFER_SIZE - 1] = '\0';  // �ֶ������ֹ��

    send(clientSocket, buffer, strlen(buffer), 0);

    // ��� buffer
    memset(buffer, 0, BUFFER_SIZE);

    // ���ļ����м��������
    aes.encryptAndEncode(aes.key, fileName, fileName + ".enc");


    // �����ļ���С
    struct stat statbuf;
    stat((fileName+".enc").c_str(), &statbuf);
    std::string fileSize = std::to_string(statbuf.st_size);
    
    strncpy_s(buffer, fileSize.c_str(), fileSize.size());
    send(clientSocket, buffer, strlen(buffer), 0);
    
    std::cout << fileSize << std::endl;
    std::cout << buffer << std::endl;

    // ��� buffer
    memset(buffer, 0, BUFFER_SIZE);

    // �����ļ�����
    std::ifstream file(fileName+".enc", std::ios::in | std::ios::out | std::ios::binary);
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

    // ɾ�������ļ�
    std::string deleteCommand = "del " + fileName + ".enc";
    system(deleteCommand.c_str());

}

void Client::recvFile() {

    while (true) {
        // �ļ���
        char buffer[BUFFER_SIZE] = { 0 };
        int bytesRead;

        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "�����޷������ļ�����" << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "�������ѶϿ����ӡ�" << std::endl;
            break;
        }

        std::string fileName = buffer;
        std::cout << "�������ļ���" << fileName << std::endl;

        // ��� buffer
        memset(buffer, 0, BUFFER_SIZE);

        // �ļ���С
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "�����޷����մ�С��" << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "�������ѶϿ����ӡ�" << std::endl;
            break;
        }
        // �����յ�������ת��Ϊ����
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


        // �����ļ�����
        std::ofstream file(fileName + ".enc", std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "�����޷����ļ���" << std::endl;
            return;
        }

        int totalBytesReceived = 0;
        while (totalBytesReceived < fileSize) {
            bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytesRead == SOCKET_ERROR) {
                std::cerr << "�����޷������ļ����ݡ�" << std::endl;
                break;
            }
            else if (bytesRead == 0) {
                std::cout << "�ļ�������ϣ�" << fileName + ".enc" << std::endl;
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
            std::cout << totalBytesReceived << "   " << fileSize << std::endl;
        }
        else {
            std::cerr << "�����ļ�����ʧ�ܣ�" << fileName << std::endl;
            std::cout << totalBytesReceived << "   " << fileSize << std::endl;
        }

        //std::cout << "�ļ����ճɹ���" << fileName << std::endl;
        file.close();

        // �Խ��յ��ļ����н��������
        aes.decodeAndDecrypt(aes.key, fileName + ".enc", fileName);

        // ɾ�������ļ�
        std::string deleteCommand = "del " + fileName + ".enc";
        system(deleteCommand.c_str());

    }
}

// ��ȡ��ǰĿ¼�µ��ļ��б�
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

// ��ʼ�������ļ�
void Client::InitSendFile()
{
    // ��ȡ��ǰĿ¼�µ��ļ��б�
	files = getFilesInCurrentDirectory();

    // ��ӡ�ļ��б�
    std::cout << "------------------�ļ��б�-----------------------------" << std::endl;
    for (auto file : files)
    {
        std::cout << file << std::endl;
    }
    std::cout << "-------------------------------------------------------" << std::endl;

    // �����ļ�����
    int numFiles = files.size();
    send(clientSocket, (char*)&numFiles, sizeof(int), 0);

    // �ȴ�ȷ��
    recvConfirm();
    cout << "Ŀ¼���ļ�������" << files.size() << endl;
	// �����ļ�
    for (const auto& file : files) {
		sendOneFile(file);
	}
    cout << "Ŀ¼���ļ�������" << files.size() << endl;
}

void Client::sendOneFile(std::string fileName)
{
    char buffer[BUFFER_SIZE] = { 0 };
    int bytesRead;

    // �����ļ���
    strncpy_s(buffer, fileName.c_str(), BUFFER_SIZE - 1);  // ����һ��λ�ø���ֹ��
    buffer[BUFFER_SIZE - 1] = '\0';  // �ֶ������ֹ��
    send(clientSocket, buffer, strlen(buffer), 0);

    // �ȴ�ȷ��
    recvConfirm();

    // ��� buffer
    memset(buffer, 0, BUFFER_SIZE);

    // ���ļ����м��������
    aes.encryptAndEncode(aes.key, fileName, fileName + ".enc");


    // �����ļ���С
    struct stat statbuf;
    stat((fileName + ".enc").c_str(), &statbuf);
    std::string fileSize = std::to_string(statbuf.st_size);

    strncpy_s(buffer, fileSize.c_str(), fileSize.size());
    send(clientSocket, buffer, strlen(buffer), 0);

    std::cout << fileSize << std::endl;
    std::cout << buffer << std::endl;

    // �ȴ�ȷ��
    recvConfirm();

    // ��� buffer
    memset(buffer, 0, BUFFER_SIZE);

    // �����ļ�����
    std::ifstream file(fileName + ".enc", std::ios::in | std::ios::out | std::ios::binary);
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

    // ɾ�������ļ�
    std::string deleteCommand = "del " + fileName + ".enc";
    system(deleteCommand.c_str());

    // �ȴ�ȷ��
    recvConfirm();

}
void Client::recvOneFile()
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
    sendConfirm();

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
    // �����յ�������ת��Ϊ����
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
    sendConfirm();

    // �����ļ�����
    std::ofstream file(fileName + ".enc", std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "�����޷����ļ���" << std::endl;
        return;
    }

    int totalBytesReceived = 0;
    while (totalBytesReceived < fileSize) {
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "�����޷������ļ����ݡ�" << std::endl;
            break;
        }
        else if (bytesRead == 0) {
            std::cout << "�ļ�������ϣ�" << fileName + ".enc" << std::endl;
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
        std::cout << totalBytesReceived << "   " << fileSize << std::endl;
    }
    else {
        std::cerr << "�����ļ�����ʧ�ܣ�" << fileName << std::endl;
        std::cout << totalBytesReceived << "   " << fileSize << std::endl;
    }

    file.close();

    // �Խ��յ��ļ����н��������
    aes.decodeAndDecrypt(aes.key, fileName + ".enc", fileName);

    // ɾ�������ļ�
    std::string deleteCommand = "del " + fileName + ".enc";
    system(deleteCommand.c_str());

    // ����ȷ��
    sendConfirm();
}

// �����ļ��߳�
void Client::recvFileThread() {
    // ����ѭ��
    while (true) {
        // ��������
        int numFiles;
        recv(clientSocket, (char*)&numFiles, sizeof(int), 0);
        // �����ļ�
        for (int i = 0; i < numFiles; i++) {
            recvOneFile();
            }
	}

}

// ����ȷ����Ϣ
void Client::sendConfirm()
{
    sync++;
    send(clientSocket, (char*)&sync, sizeof(int), 0);
}

// ����ȷ����Ϣ
void Client::recvConfirm()
{
    int sync1;
    recv(clientSocket, (char*)&sync1, sizeof(int), 0);
    std::cout << "sync: " << sync1 << std::endl;
    sync = sync1;
}

// ��ȡ��ǰʱ����ַ���
std::string Client::getCurrentTimeAsString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %X");
    return ss.str();
}
