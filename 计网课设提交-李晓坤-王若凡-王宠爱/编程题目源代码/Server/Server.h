#pragma once
#ifndef SERVER_H
#define SERVER_H
#define _WINSOCK_DEPRECATED_NO_WARNINGS
/*
�ĵ�˵����
    1. ���ĵ������˷������Server�������˷���˵����й��ܡ�
    2. ����˵Ĺ��ܰ�����
	a. ��ʼ������˶���
	b. ����˿�ʼ��������
	c. ��ȡ��ǰĿ¼�µ������ļ�
	d. ��ʼ���û��ļ��б�
	e. �����ļ�
	f. �����ļ�
	g. ˢ�������û����ļ��б�
	h. ����ȷ����Ϣ
	i. ����ȷ����Ϣ
	j. ��ȡ��ǰʱ��
	k. ���ļ��б�д���ļ�
    3. ����˵���Ҫ���ݳ�Ա������
    a. ����˼����׽���
    b. �������������ӵ��û��б�
    c. �û��ļ��б�
    d. �����û����ļ��Ĳ���
    e. ��־�ļ�
    f. ͬ����־
    4. ����˵���Ҫ������Ա������
    a. ���캯��
    b. ����ͻ�������
    c. ���ļ��б�д���ļ�
    d. ��ȡ��ǰʱ��
    e. ����ȷ����Ϣ
    f. ����ȷ����Ϣ
    g. ˢ�������û����ļ��б�
    h. �����ļ�
    i. �����ļ�
    j. ��ʼ���û��ļ��б�
    k. ��ȡ��ǰĿ¼�µ������ļ�
    l. ����˿�ʼ��������
    5. ����˵���Ҫ���ݳ�Ա������
    a. ����˼����׽���
    b. �������������ӵ��û��б�
    c. �û��ļ��б�
    d. �����û����ļ��Ĳ���
    e. ��־�ļ�
    f. ͬ����־
    6. ����˵���Ҫ������Ա������
    a. ���캯��
    b. ����ͻ�������
    c. ���ļ��б�д���ļ�
    d. ��ȡ��ǰʱ��
    e. ����ȷ����Ϣ
    f. ����ȷ����Ϣ
    g. ˢ�������û����ļ��б�
    h. �����ļ�
    i. �����ļ�
    j. ��ʼ���û��ļ��б�
    k. ��ȡ��ǰĿ¼�µ������ļ�
*/
#include <iostream>
#include <fstream>
#include <string>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <ctime>
#include <chrono>
#include <sstream>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 64974
#define BUFFER_SIZE 1024
using namespace std;

class Server {
public:
    Server();// ���캯������ʼ������˶���
    void acceptConnections();// ����˿�ʼ��������
    vector<string> getFilesInCurrentDirectory();// ��ȡ��ǰĿ¼�µ������ļ�
    void InitFileList(SOCKET clientSocket);// ��ʼ���û��ļ��б�
    void SendFile(SOCKET clientSocket, string fileName);// �����ļ�
    void RecvFile(SOCKET clientSocket);// �����ļ�
    void refreshAllFiles();// ˢ�������û����ļ��б�
    void sendConfirm(SOCKET clientSocket);// ����ȷ����Ϣ
    void recvConfirm(SOCKET clientSocket);// ����ȷ����Ϣ
    std::string getCurrentTimeAsString();// ��ȡ��ǰʱ��
    void txtToShowFileList();// ���ļ��б�д���ļ�
private:
    SOCKET listenSocket;// ����˼����׽���
    void handleClient(SOCKET clientSocket);// ����ͻ�������
    // �������������ӵ��û��б�
    std::vector <SOCKET> clientList;
    // �û��ļ��б�
    std::unordered_map<SOCKET, std::vector<std::string>> userFilesMap;
    // �����û����ļ��Ĳ���
    std::vector<string> allFiles;
    ofstream log;// ��־�ļ�
    int sync = 0;// ͬ����־
};

#endif /* SERVER_H */
