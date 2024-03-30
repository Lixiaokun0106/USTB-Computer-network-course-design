#pragma once
#ifndef CLIENT_H
#define CLIENT_H
#define _WINSOCK_DEPRECATED_NO_WARNINGS
/*
文档说明：
	本文件定义了Client类，该类用于实现客户端功能。
	客户端可以连接服务器，发送文件，接收文件，发送单个文件，接收单个文件，获取当前目录下的所有文件，初始化发送文件，接收文件线程，发送确认信息，接收确认信息，获取当前时间的字符串表示。
	本文件包含了以下函数：
	1. Client()：构造函数，用于初始化客户端对象。
	2. connectToServer()：连接服务器。
	3. sendFile(std::string filePath)：发送文件。
	4. recvFile()：接收文件。
	5. sendOneFile(std::string filePath)：发送单个文件。
	6. recvOneFile()：接收单个文件。
	7. getFilesInCurrentDirectory()：获取当前目录下的所有文件。
	8. InitSendFile()：初始化发送文件。
	9. recvFileThread()：接收文件线程。
	10. sendConfirm()：发送确认信息。
	11. recvConfirm()：接收确认信息。
	12. getCurrentTimeAsString()：获取当前时间的字符串表示。
	本文件包含了以下变量：
	1. clientSocket：客户端套接字。
	2. aes：AES加解密对象。
	3. files：当前目录下的所有文件列表。
	4. log：日志文件。
	5. sync：同步标志。
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
    Client();// 构造函数，用于初始化客户端对象
    void connectToServer();// 连接服务器
    void sendFile(std::string filePath);// 发送文件
    void recvFile();// 接收文件
    void sendOneFile(std::string filePath);// 发送单个文件
    void recvOneFile();// 接收单个文件
    vector<string> getFilesInCurrentDirectory();// 获取当前目录下的所有文件
    void InitSendFile();// 初始化发送文件
    void recvFileThread();// 接收文件线程
    void sendConfirm();// 发送确认信息
    void recvConfirm();// 接收确认信息
    std::string getCurrentTimeAsString();// 获取当前时间的字符串表示

private:
    SOCKET clientSocket;// 客户端套接字
    AES aes;// AES加解密对象
    vector<string> files;// 当前目录下的所有文件列表
    ofstream log;// 日志文件
    int sync = 0;// 同步标志
};

#endif /* CLIENT_H */
