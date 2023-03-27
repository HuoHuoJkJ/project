#include "_ftp.h"

struct st_arg
{
    char host[31];          // 远程服务器的IP和端口。
    int  mode;              // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    char username[31];      // 远程服务器的用户名。
    char password[31];      // 远程服务器的密码。
    char localpath[301];    // 本地文件存放的目录。
    char remotepath[301];   // 远程服务器存放文件的目录。
    char matchname[101];    // 待上传文件匹配的规则。不匹配的文件不会被上传，本字段尽可能设置精确，不建议用*匹配全部文件。
    int  ptype;             // 文件上传成功后，客户器文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。
    char localpathbak[301]; // 文件上传成功后，客户器文件的备份目录，此参数只有当ptype=3时才有效。
    char okfilename[301];   // 已上传成功文件名清单，此参数只有当ptype=1时才有效。
    int  timeout;           // 进程心跳超时时间
    char pname[51];         // 进程名，建议用"ftpputfiles_后缀"的方式
} starg;

struct st_fileinfo
{
    char filename[301];     // list文件中的一个文件名
    char mtime[21];         // 文件时间
};

vector<struct st_fileinfo> vfileinfo1; // 已上传成功的文件名的容器，从okfilename中加载。
vector<struct st_fileinfo> vfileinfo2; // 存放上传前列出客户端文件名的容器，从nlist文件中加载。
vector<struct st_fileinfo> vfileinfo3; // 本次不需要上传的文件名的容器
vector<struct st_fileinfo> vfileinfo4; // 本次需要上传的文件名的容器

CLogFile logfile;
Cftp ftp;
CPActive PActive;

void _help();
bool _xmltoarg(char *strxmlbuffer);
bool _ftpputfiles();
bool LoadLocalFile();
// 加载okfilename文件中的内容到容器vlistfile1中。
bool LoadOKFile();
// 比较vlistfile2和vlistfile1，得到vlistfile3和vlistfile4。
bool CompVector();
// 把容器vlistfile3中的内容写入到okfilename文件，覆盖之前的旧okfilename文件。
bool WriteToOKFile();
// 将上传成功的文件追加到okfilename中。
bool AppendToOKFile(struct st_fileinfo *stfileinfo);
void EXIT(int sig);

int main(int argc, char *argv[])
{
    // 写帮助文档
    if (argc != 3) { _help(); return -1; }

    // 处理程序的信号和IO
    CloseIOAndSignal(true);
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);
    // 在整个程序编写完成且运行稳定后，关闭IO和信号。为了方便调试，暂时不启用CloseIOAndSignal();

    // 日志文件
    if ( logfile.Open(argv[1]) == false )
    { printf("logfile.Open(%s) failed!\n", argv[1]); return -1; }

    // 解析xml字符串，获得参数
    if ( _xmltoarg(argv[2]) == false )
    { logfile.Write("解析xml失败！\n"); return -1; }
    
    PActive.AddPInfo(starg.timeout, starg.pname);

    // 登录ftp服务器
    if ( ftp.login(starg.host, starg.username, starg.password, starg.mode) == false )
    {
        logfile.Write("ftp.login(%s, %s, %s, %d) failed!\n", starg.host, starg.username, starg.password, starg.mode);
        return -1;
    }
    // logfile.Write("ftp.login() successed\n");
    
   if ( _ftpputfiles() == false )
    { logfile.Write("上传文件失败！\n"); return -1; }

    return 0;
}

void _help()
{
    printf("\n");
    printf("Use: ftpputfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools1/bin/procctl 30 /project/tools1/bin/ftpputfiles /log/idc/ftpputfiles_surfdata.log \"<host>127.0.0.1:21</host><mode>1</mode><username>lighthouse</username><password>dyt.9525</password><localpath>/tmp/idc/surfdata</localpath><remotepath>/tmp/ftpputest</remotepath><matchname>SURF_ZH*.JSON</matchname><ptype>1</ptype><localpathbak>/tmp/idc/surfdatabak</localpathbak><okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename><timeout>80</timeout><pname>ftpputfiles_surfdata</pname>\"\n\n\n");

    printf("本程序是通用的功能模块，用于把远程ftp服务器的文件上传到本地目录。\n");
    printf("logfilename 是本程序的运行日志文件。\n");
    printf("xmlbuffer   为文件上传的参数，如下：\n");
    printf("    <host></host>                   远程服务器的IP和端口。\n");
    printf("    <mode></mode>                   传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("    <username></username>           远程服务器的用户名。\n");
    printf("    <password></password>           远程服务器的密码。\n");
    printf("    <localpath></localpath>         本地文件存放的目录。\n");
    printf("    <remotepath></remotepath>       远程服务器存放文件的目录。\n");
    printf("    <matchname></matchname>         待上传文件匹配的规则。不匹配的文件不会被上传，本字段尽可能设置精确，不建议用*匹配全部文件。\n");
    printf("    <ptype></ptype>                 文件上传成功后，本地文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("    <localpathbak></localpathbak>   文件上传成功后，服务器文件的备份目录，此参数只有当ptype=3时才有效。\n");
    printf("    <okfilename></okfilename>       已上传成功文件名清单，此参数只有当ptype=1时才有效。\n");
    printf("    <timeout></timeout>             进程心跳超时时间\n");
    printf("    <pname></pname>                 进程名，建议用\"ftpputfiles_后缀\"的方式\n\n\n");
}

void EXIT(int sig)
{
    printf("接收到%s信号，进程退出\n\n", sig);
    exit(0);
}

bool _xmltoarg(char *strxmlbuffer)
{
    memset(&starg, 0, sizeof(starg));
    GetXMLBuffer(strxmlbuffer, "host", starg.host, 30);              // 远程服务器的IP和端口。
    if (strlen(starg.host) == 0)
    { logfile.Write("host值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "mode", &starg.mode);                 // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    if (starg.mode != 2) starg.mode = 1;

    GetXMLBuffer(strxmlbuffer, "username", starg.username, 30);      // 远程服务器的用户名。
    if (strlen(starg.username) == 0)
    { logfile.Write("username值为空！\n"); return false; }
    
    GetXMLBuffer(strxmlbuffer, "password", starg.password, 30);      // 远程服务器的密码
    if (strlen(starg.password) == 0)
    { logfile.Write("password值为空！\n"); return false; }
    
    GetXMLBuffer(strxmlbuffer, "localpath", starg.localpath, 300);   // 本地存放文件的目录
    if (strlen(starg.localpath) == 0)
    { logfile.Write("localpath值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "remotepath", starg.remotepath, 300);            // 远程服务器存放文件的目录
    if (strlen(starg.remotepath) == 0)
    { logfile.Write("remotepath值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "matchname", starg.matchname, 100);              // 待上传文件匹配的规则
    if (strlen(starg.matchname) == 0)
    { logfile.Write("matchname值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "ptype", &starg.ptype);                          // 文件上传成功后，本地文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。
    if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3) )
    { logfile.Write("ptype值不正确！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "localpathbak", starg.localpathbak, 300);      // 文件上传成功后，本地文件的备份目录，此参数只有当ptype=3时才有效。
    if ( (strlen(starg.localpathbak) == 0) && (starg.ptype == 3) )
    { logfile.Write("localpathbak值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "okfilename", starg.okfilename, 300);            // 已上传成功文件名清单，此参数只有当ptype=1时才有效。      
    if ( (strlen(starg.okfilename) == 0) && (starg.ptype == 1) )
    { logfile.Write("okfilename值为空！\n"); return false; }
    
    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);                      // 进程心跳超时时间
    if (starg.timeout == 0)
    { logfile.Write("timeout值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);                       // 进程名
    if (strlen(starg.pname) == 0)
    { logfile.Write("pname值为空！\n"); return false; }

    return true;
}

// 上传文件的功能函数
bool _ftpputfiles()
{
    // 把localpath目录下的文件信息加载到容器v2中
    if ( LoadLocalFile() == false )
    {
        logfile.Write("把localpath目录下的文件信息加载到容器v2中失败！\n"); return false;
    }

    PActive.UptATime();

    if (starg.ptype == 1)
    {
        // 加载okfilename文件中的内容到容器vlistfile1中。
        LoadOKFile();

        // 比较vlistfile2和vlistfile1，得到vlistfile3和vlistfile4。
        CompVector();

        // 把容器vlistfile3中的内容写入到okfilename文件，覆盖之前的旧okfilename文件。
        WriteToOKFile();

        // 把vlistfile4中的内容复制到vlistfile2中。这是为了兼容下方的代码。在我们得到的结果中v4是待上传的文件，而下方代码中v2才是待上传的文件。
        vfileinfo2.clear(); vfileinfo2.swap(vfileinfo4);
    }

    PActive.UptATime();

    // 遍历容器vfilelist
    char strremotefilename[301], strlocalfilename[301];
    for (int ii = 0; ii < vfileinfo2.size(); ii++)
    {
        // 调用ftp.get()方法从服务器上传文件。

        // 将绝对路径的文件名拼接起来 
        SNPRINTF(strremotefilename, sizeof(strremotefilename), 300, "%s/%s", starg.remotepath, vfileinfo2[ii].filename);
        SNPRINTF(strlocalfilename, sizeof(strlocalfilename), 300, "%s/%s", starg.localpath, vfileinfo2[ii].filename);

        logfile.Write("put %s ... ", strlocalfilename);
        
        if ( ftp.put(strlocalfilename, strremotefilename) == false )
        {
            logfile.WriteEx("failed!\n");
            return false;
        }
        logfile.WriteEx("successed!\n");

        PActive.UptATime();

        if (starg.ptype == 1)
            AppendToOKFile(&vfileinfo2[ii]);

        if (starg.ptype == 2)
        {
            if ( REMOVE(strlocalfilename) == false )
            {
                logfile.Write("REMOVE(%s) failed!\n", strlocalfilename);
                return false;
            }
        }

        if (starg.ptype == 3)
        {
            char strlocalfilenamebak[301];
            SNPRINTF(strlocalfilenamebak, sizeof(strlocalfilenamebak), 300, "%s/%s", starg.localpathbak, vfileinfo2[ii].filename);
            if ( RENAME(strlocalfilename, strlocalfilenamebak) == false )
            {
                logfile.Write("RENAME(%s, %s) failed!\n", strlocalfilename, strlocalfilenamebak);
                return false;
            }
        }
    }
    
    return true;
}

bool LoadLocalFile()
{
    vfileinfo2.clear();
    
    CDir dir;
    
    dir.SetDateFMT("yyyymmddhh24miss");
    
    if ( dir.OpenDir(starg.localpath, starg.matchname) == false )
    { logfile.Write("dir.OpenDir(%s, %s) failed!\n", starg.localpath, starg.matchname); return false; }
    
    struct st_fileinfo stfileinfo;

    while (true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));
        
        if ( dir.ReadDir() == false ) break;
        
        strcpy(stfileinfo.filename, dir.m_FileName);    // 文件名，不包含目录名
        strcpy(stfileinfo.mtime, dir.m_ModifyTime);     // 文件时间

        vfileinfo2.push_back(stfileinfo);
    }

    return true;
}

// 加载okfilename文件中的内容到容器vlistfile1中。
bool LoadOKFile()
{
    vfileinfo1.clear();
    CFile File;
    
    // 如果是第一次运行，okfilename本身就不存在，并不是错误，所以返回true
    if ( File.Open(starg.okfilename, "r") == false ) return true;
    
    char strxmlbuffer[301];

    struct st_fileinfo stfileinfo;

    while(true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));
        
        if ( File.Fgets(strxmlbuffer, 300, true) == false ) break;
        
        GetXMLBuffer(strxmlbuffer, "filename", stfileinfo.filename, 300);
        GetXMLBuffer(strxmlbuffer, "mtime", stfileinfo.mtime, 20);

        vfileinfo1.push_back(stfileinfo);
    }

    return true;
}

// 比较vlistfile2和vlistfile1，得到vlistfile3和vlistfile4。
bool CompVector()
{
    vfileinfo3.clear(); vfileinfo4.clear();
    
    int ii, jj;
    // 从v2中找文件，第一层for循环不能是遍历v1。v1存放的是客户端下的文件，而我们需要上传的文件应当是服务器上的。
    // 如果遍历v1的话，得到的值可能就是客户端上有的但是服务器上没有的文件
    for (ii = 0; ii < vfileinfo2.size(); ii++)
    {
        for (jj = 0; jj < vfileinfo1.size(); jj++)
        {
            if ( (strcmp(vfileinfo1[jj].filename, vfileinfo2[ii].filename) == 0) &&
                 (strcmp(vfileinfo1[jj].mtime,    vfileinfo2[ii].mtime   ) == 0) )
            {
                vfileinfo3.push_back(vfileinfo2[ii]);
                break;
            }
        }
        // 如果从在v2中，没有找到与v1对应的文件，说明该文件需要被上传，存到v4中
        if (jj == vfileinfo1.size())    vfileinfo4.push_back(vfileinfo2[ii]);
    }

return true;
}

// 把容器vlistfile3中的内容写入到okfilename文件，覆盖之前的旧okfilename文件。
bool WriteToOKFile()
{
    CFile File;
    if ( File.Open(starg.okfilename, "w") == false )
    {
        logfile.Write("File.Open(%s, %s) failed!\n", starg.okfilename, "\"w\""); return false;
    }
    for (int ii = 0; ii < vfileinfo3.size(); ii++)
        File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n", vfileinfo3[ii].filename, vfileinfo3[ii].mtime);

    return true;
}

// 将上传成功的文件追加到okfilename中。
bool AppendToOKFile(struct st_fileinfo *stfileinfo)
{
    CFile File;
    if ( File.Open(starg.okfilename, "a") == false )
    {
        logfile.Write("File.Open(%s, %s) failed!\n", starg.okfilename, "\"a\""); return false;
    }
    File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n", stfileinfo->filename, stfileinfo->mtime);

    return true;
}