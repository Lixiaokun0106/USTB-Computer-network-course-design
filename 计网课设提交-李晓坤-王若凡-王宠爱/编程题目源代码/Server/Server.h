#pragma once
#ifndef SERVER_H
#define SERVER_H
#define _WINSOCK_DEPRECATED_NO_WARNINGS
/*
文档说明：
    1. 本文档定义了服务端类Server，包含了服务端的所有功能。
    2. 服务端的功能包括：
	a. 初始化服务端对象
	b. 服务端开始监听连接
	c. 获取当前目录下的所有文件
	d. 初始化用户文件列表
	e. 发送文件
	f. 接收文件
	g. 刷新所有用户的文件列表
	h. 发送确认信息
	i. 接收确认信息
	j. 获取当前时间
	k. 将文件列表写入文件
    3. 服务端的主要数据成员包括：
    a. 服务端监听套接字
    b. 服务器建立连接的用户列表
    c. 用户文件列表
    d. 所有用户的文件的并集
    e. 日志文件
    f. 同步标志
    4. 服务端的主要函数成员包括：
    a. 构造函数
    b. 处理客户端连接
    c. 将文件列表写入文件
    d. 获取当前时间
    e. 接收确认信息
    f. 发送确认信息
    g. 刷新所有用户的文件列表
    h. 接收文件
    i. 发送文件
    j. 初始化用户文件列表
    k. 获取当前目录下的所有文件
    l. 服务端开始监听连接
    5. 服务端的主要数据成员包括：
    a. 服务端监听套接字
    b. 服务器建立连接的用户列表
    c. 用户文件列表
    d. 所有用户的文件的并集
    e. 日志文件
    f. 同步标志
    6. 服务端的主要函数成员包括：
    a. 构造函数
    b. 处理客户端连接
    c. 将文件列表写入文件
    d. 获取当前时间
    e. 接收确认信息
    f. 发送确认信息
    g. 刷新所有用户的文件列表
    h. 接收文件
    i. 发送文件
    j. 初始化用户文件列表
    k. 获取当前目录下的所有文件
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
    Server();// 构造函数，初始化服务端对象
    void acceptConnections();// 服务端开始监听连接
    vector<string> getFilesInCurrentDirectory();// 获取当前目录下的所有文件
    void InitFileList(SOCKET clientSocket);// 初始化用户文件列表
    void SendFile(SOCKET clientSocket, string fileName);// 发送文件
    void RecvFile(SOCKET clientSocket);// 接收文件
    void refreshAllFiles();// 刷新所有用户的文件列表
    void sendConfirm(SOCKET clientSocket);// 发送确认信息
    void recvConfirm(SOCKET clientSocket);// 接收确认信息
    std::string getCurrentTimeAsString();// 获取当前时间
    void txtToShowFileList();// 将文件列表写入文件
private:
    SOCKET listenSocket;// 服务端监听套接字
    void handleClient(SOCKET clientSocket);// 处理客户端连接
    // 服务器建立连接的用户列表
    std::vector <SOCKET> clientList;
    // 用户文件列表
    std::unordered_map<SOCKET, std::vector<std::string>> userFilesMap;
    // 所有用户的文件的并集
    std::vector<string> allFiles;
    ofstream log;// 日志文件
    int sync = 0;// 同步标志
};

#endif /* SERVER_H */
