#include "_ftp.h"

Cftp ftp;

int main(int arg, char *argv[])
{
    if ( ftp.login("127.0.0.1:21", "lighthouse", "dyt.9525") == false )
    {
        printf("ftp.login(127.0.0.1:21) failed!\n");
        return -1;
    }
    printf("ftp.login(127.0.0.1:21) successed!\n");

    if ( ftp.mtime("/home/lighthouse/ftplib.h") == false )
    {
        printf("ftp.mtime(/home/lighthouse/ftplib.h) failed!\n");
        return -1;
    }
    printf("ftp.mtime(/home/lighthouse/ftplib.h) successed! m_mtime = %s\n", ftp.m_mtime);

    if ( ftp.size("/home/lighthouse/ftplib.h") == false )
    {
        printf("ftp.size(/home/lighthouse/ftplib.h) failed!\n");
        return -1;
    }
    printf("ftp.size(/home/lighthouse/ftplib.h) successed! m_size = %d\n", ftp.m_size);

    if ( ftp.nlist("/home/lighthouse", "/tmp/aaa/bbb.lst") == false )
    {
        printf("ftp.nlist(/home/lighthouse, /tmp/aaa/bbb.lst) failed!\n");
        return -1;
    }
    printf("ftp.nlist(/home/lighthouse, /tmp/aaa/bbb.lst) successed!\n");
    
    if ( ftp.get("/home/lighthouse/_ftp.h", "/tmp/demo/_ftp.g.bak", true) == false )
    {
        printf("ftp.get() failed!\n");
        return -1;
    }
    printf("ftp.get() successed!\n");
    
    if ( ftp.put("/project/idc1/c/crtsurfdata.cpp", "/home/lighthouse/aaa/crtsurfdata.cpp.bak", true) == false )
    {
        printf("ftp.put() failed!\n");
        return -1;
    }
    printf("ftp.put() successed!\n");

    ftp.logout();

    return 0;
}