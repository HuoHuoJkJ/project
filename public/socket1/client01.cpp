/*
 * 创建客户端socket
 * 向服务器发起连接请求
 * 与服务器通信，发送一个报文后等待回复，然后再发下一个报文
 * 不断重复上一步，直到全部的数据被发送完
 * 关闭socket，释放资源
 */
#include "../_public.h"

CTcpClient TcpClient;

bool srv000();      // 保持心跳
bool srv001();      // 登录业务
bool srv002();      // 查询余额

int main(int argc, char *argv[])
{
    if (argc != 3)
    { printf("Using:./client01 ip port\nExample:./client01 127.0.0.1 5005\n\n"); return -1; }
    
    
    if ( TcpClient.ConnectToServer(argv[1], atoi(argv[2])) == false )
    { printf("客户端尝试连接服务端(%s)失败！\n", argv[1]); return -1; }
    
    if ( srv000() == false ) return -1;
    
    sleep(10);

    // 登录业务
    if ( srv001() == false ) return -1;

    // 查询余额
    if ( srv002() == false ) return -1;
    
    for (int ii = 2; ii < 5; ii ++)
    {
        if ( srv000() == false ) break;
    }

    if ( srv002() == false ) return -1;

    return 0;
}

bool srv000()
{
    char buffer[1024];

    SPRINTF(buffer, sizeof(buffer), "<srvcode>0</srvcode>");
    printf("发送:%s\n", buffer);
    if ( TcpClient.Write(buffer) == false ) return false;  // 向服务端发送请求报文

    memset(buffer, 0, sizeof(buffer));
    if ( TcpClient.Read(buffer) == false ) return false;
    printf("接受%s\n", buffer);
    
    return true;
}

// 登录业务
bool srv001()
{
    char buffer[1024];

    SPRINTF(buffer, sizeof(buffer), "<srvcode>1</srvcode><tel>18561860345</tel><password>123456</password>");
    printf("发送:%s\n", buffer);
    if ( TcpClient.Write(buffer) == false ) return false;  // 向服务端发送请求报文

    memset(buffer, 0, sizeof(buffer));
    if ( TcpClient.Read(buffer) == false ) return false;
    printf("接受%s\n", buffer);
    
    int iretcode = -1;
    GetXMLBuffer(buffer, "retcode", &iretcode);
    if ( iretcode != 0 )
    { printf("登录失败！\n"); return false; }
    printf("登录成功！\n");

    return true;
}

// 查询余额
bool srv002()
{
    char buffer[1024];

    SPRINTF(buffer, sizeof(buffer), "<srvcode>2</srvcode><cardid>3123129412312</cardid>");
    printf("发送:%s\n", buffer);
    if ( TcpClient.Write(buffer) == false ) return false;  // 向服务端发送请求报文

    memset(buffer, 0, sizeof(buffer));
    if ( TcpClient.Read(buffer) == false ) return false;
    printf("接受%s\n", buffer);
    
    int iretcode = -1;
    GetXMLBuffer(buffer, "retcode", &iretcode);
    if ( iretcode != 0 )
    { printf("查询余额失败！\n"); return false; }
    double ua = 0;
    GetXMLBuffer(buffer, "ua", &ua);
    printf("余额查询成功(%.2f)\n", ua);


    return true;
}