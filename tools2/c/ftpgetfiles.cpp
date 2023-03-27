#include "_ftp.h"

struct st_arg
{
    char host[31];
    int  mode;
    char username[31];
    char password[31];
    char localpath[301];
    char remotepath[301];
    char matchname[31];
    char listfilename[301];
}starg;

struct st_fileinfo
{
    char filename[301];
    char mtime[21];
};

CLogFile logfile;
Cftp     ftp;

vector<struct st_fileinfo> vfileinfo;

void _help(void);
void EXIT(int sig);
bool _xmltoarg(const char *xmlbuffer);
bool _ftpgetfiles();
bool LoadListFile();

int main(int argc, char *argv[])
{
    // 生成程序的帮助文档
    if (argc != 3) { _help(); return -1; }

    // 关闭进程的IO和信号，仅保留2和15信号
    // CloseIOAndSignal(true);
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    if ( logfile.Open(argv[1]) == false )
    { printf("打开日志文件%s失败！\n", argv[1]); return -1; }

    if ( _xmltoarg(argv[2]) == false )
    { logfile.Write("解析xmlbuffer失败！\n"); return -1; }

    if ( ftp.login(starg.host, starg.username, starg.password, starg.mode) == false )
    { logfile.Write("登录ftp服务器%s失败！\n", starg.host); return -1; }
    logfile.Write("登录FTP服务器%s成功！\n", starg.host);

    if ( _ftpgetfiles() == false )
    { logfile.WriteEx("下载文件失败！\n"); return -1; }

    return 0;
}

void _help(void)
{
    printf("Use:ftpgetfiles logfile xmlbuffer\n");
    printf("Sample:/project/tools2/bin/ftpgetfiles /home/lighthouse/ftpgetfiles2_surfdata.log \"<host>127.0.0.1:21</host><mode>1</mode><username>lighthouse</username><password>dyt.9525</password><localpath>/home/lighthouse/client</localpath><remotepath>/home/lighthouse/server</remotepath><matchname>*.h,*.cpp</matchname><listfilename>/home/lighthouse/ftpgetfiles2_surfdata.list</listfilename>\"\n\n");

    printf("logfile                                                   客户端:日志文件\n");
    printf("xmlbuffer:\n");
    printf("  <host>127.0.0.1:21</host>                               服务器:IP和端口\n");
    printf("  <mode>1</mode>                                          传输模式。1为被动传输，2为主动传输\n");
    printf("  <username>lighthouse</username>                         服务器:用户名\n");
    printf("  <password>dyt.9525</password>                           服务器:密码\n");
    printf("  <localpath>~/client</localpath>                         客户端:下载到本地的目录的位置\n");
    printf("  <remotepath>~/server</remotepath>                       服务器:需要下载的服务器的文件的目录位置\n");
    printf("  <matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname>        服务器文件名的匹配规则，想下载什么类型的文件\n");
    printf("  <listfilename>~/listfilename.list</listfilename>        客户端:列出服务器目录下的所有文件名到客户端的文件中\n");
}

bool _xmltoarg(const char *xmlbuffer)
{
    memset(&starg, 0, sizeof(starg));

    GetXMLBuffer(xmlbuffer, "host", starg.host, 30);
    if (strlen(starg.host) == 0)
    { logfile.Write("starg.host为空!\n"); return false; }

    GetXMLBuffer(xmlbuffer, "mode", &starg.mode);
    if (starg.mode != 2) starg.mode = 1;
    
    GetXMLBuffer(xmlbuffer, "username", starg.username, 30);
    if (strlen(starg.username) == 0)
    { logfile.Write("starg.username为空!\n"); return false; }
    
    GetXMLBuffer(xmlbuffer, "password", starg.password, 30);
    if (strlen(starg.password) == 0)
    { logfile.Write("starg.password为空!\n"); return false; }

    GetXMLBuffer(xmlbuffer, "localpath", starg.localpath, 300);
    if (strlen(starg.localpath) == 0)
    { logfile.Write("starg.localpath为空!\n"); return false; }

    GetXMLBuffer(xmlbuffer, "remotepath", starg.remotepath, 300);
    if (strlen(starg.remotepath) == 0)
    { logfile.Write("starg.remotepath为空!\n"); return false; }

    GetXMLBuffer(xmlbuffer, "matchname", starg.matchname, 30);
    if (strlen(starg.matchname) == 0)
    { logfile.Write("starg.matchname为空!\n"); return false; }

    GetXMLBuffer(xmlbuffer, "listfilename", starg.listfilename, 300);
    if (strlen(starg.listfilename) == 0)
    { logfile.Write("starg.listfilename为空!\n"); return false; }

    return true;
}

void EXIT(int sig)
{
    printf("接收到%s信号，进程退出\n", sig);
    exit(0);
}

bool _ftpgetfiles()
{
    // 进入remotepath目录
    if ( ftp.chdir(starg.remotepath) == false )
    { logfile.Write("进入到目录%s失败！ ", starg.remotepath); return false; }

    // 使用nlist函数列出remotepath目录下的所有文件名到listfilename中
    if ( ftp.nlist(".", starg.listfilename) == false )
    { logfile.Write("获取目录%s中的文件内容失败！ ", starg.remotepath); return false; }
    
    // 将文件内容添加到容器vlistfile中
    if ( LoadListFile() == false )
    { logfile.Write("添加到容器失败！\n"); return false; }
    
    char strlocalpath[301];
    char stremotepath[301];
    for (int ii = 0; ii < vfileinfo.size(); ii++)
    {
        SNPRINTF(strlocalpath, sizeof(strlocalpath), 300, "%s/%s", starg.localpath, vfileinfo[ii].filename);
        SNPRINTF(stremotepath, sizeof(stremotepath), 300, "%s/%s", starg.remotepath, vfileinfo[ii].filename);

        if (ftp.get(stremotepath, strlocalpath) == false )
        { logfile.Write("下载文件%s失败！\n", vfileinfo[ii].filename); return false; }
        logfile.Write("下载文件%s成功！\n", vfileinfo[ii].filename);
    }

    return true;
}

bool LoadListFile()
{
    vfileinfo.clear();
    CFile File;
    
    if ( File.Open(starg.listfilename, "r") == false )
    { logfile.Write("读取listfilename文件失败\n"); return false; }

    struct st_fileinfo stfileinfo;

    while (true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));
        
        if ( File.Fgets(stfileinfo.filename, 300, true) == false ) break;
        
        if ( MatchStr(stfileinfo.filename, starg.matchname) == false ) continue;

        vfileinfo.push_back(stfileinfo);
    }
    
    // for (int ii = 0; ii < vfileinfo.size(); ii++)
        // logfile.Write("%s\n", vfileinfo[ii].filename);

    return true;
}