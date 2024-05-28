#pragma once
#ifndef CLIENT_H
#define CLIENT_H
#define _WINSOCK_DEPRECATED_NO_WARNINGS
/*
�ĵ�˵����
	���ļ�������Client�࣬��������ʵ�ֿͻ��˹��ܡ�
	�ͻ��˿������ӷ������������ļ��������ļ������͵����ļ������յ����ļ�����ȡ��ǰĿ¼�µ������ļ�����ʼ�������ļ��������ļ��̣߳�����ȷ����Ϣ������ȷ����Ϣ����ȡ��ǰʱ����ַ�����ʾ��
	���ļ����������º�����
	1. Client()�����캯�������ڳ�ʼ���ͻ��˶���
	2. connectToServer()�����ӷ�������
	3. sendFile(std::string filePath)�������ļ���
	4. recvFile()�������ļ���
	5. sendOneFile(std::string filePath)�����͵����ļ���
	6. recvOneFile()�����յ����ļ���
	7. getFilesInCurrentDirectory()����ȡ��ǰĿ¼�µ������ļ���
	8. InitSendFile()����ʼ�������ļ���
	9. recvFileThread()�������ļ��̡߳�
	10. sendConfirm()������ȷ����Ϣ��
	11. recvConfirm()������ȷ����Ϣ��
	12. getCurrentTimeAsString()����ȡ��ǰʱ����ַ�����ʾ��
	���ļ����������±�����
	1. clientSocket���ͻ����׽��֡�
	2. aes��AES�ӽ��ܶ���
	3. files����ǰĿ¼�µ������ļ��б�
	4. log����־�ļ���
	5. sync��ͬ����־��
*/


#include <iostream>
#include <string>
#include <winsock2.h>
#include <thread>
#include <fstream>
#include <vector>
#include <filesystem>
#include <ctime>
#include <chrono>
#include <sstream>
#include <string.h>
#include "AES.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVER_ADDRESS "192.168.167.115"
#define PORT 64974
#define BUFFER_SIZE 1024

class Client {
public:
    Client();// ���캯�������ڳ�ʼ���ͻ��˶���
    void connectToServer();// ���ӷ�����
    void sendFile(std::string filePath);// �����ļ�
    void recvFile();// �����ļ�
    void sendOneFile(std::string filePath);// ���͵����ļ�
    void recvOneFile();// ���յ����ļ�
    vector<string> getFilesInCurrentDirectory();// ��ȡ��ǰĿ¼�µ������ļ�
    void InitSendFile();// ��ʼ�������ļ�
    void recvFileThread();// �����ļ��߳�
    void sendConfirm();// ����ȷ����Ϣ
    void recvConfirm();// ����ȷ����Ϣ
    std::string getCurrentTimeAsString();// ��ȡ��ǰʱ����ַ�����ʾ

private:
    SOCKET clientSocket;// �ͻ����׽���
    AES aes;// AES�ӽ��ܶ���
    vector<string> files;// ��ǰĿ¼�µ������ļ��б�
    ofstream log;// ��־�ļ�
    int sync = 0;// ͬ����־
};

#endif /* CLIENT_H */
