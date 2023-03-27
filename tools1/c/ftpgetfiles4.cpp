#include "_ftp.h"

struct st_arg
{
    char host[31];          // 远程服务器的IP和端口。
    int  mode;              // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    char username[31];      // 远程服务器的用户名。
    char password[31];      // 远程服务器的密码。
    char localpath[301];    // 本地文件存放的目录。
    char remotepath[301];   // 远程服务器存放文件的目录。
    char matchname[101];    // 待下载文件匹配的规则。不匹配的文件不会被下载，本字段尽可能设置精确，不建议用*匹配全部文件。
    char listfilename[301]; // 下载前列出服务器文件名的文件
    int  ptype;             // 文件下载成功后，远程服务器文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。
    char remotepathbak[301];// 文件下载成功后，服务器文件的备份目录，此参数只有当ptype=3时才有效。
    char okfilename[301];   // 已下载成功文件名清单，此参数只有当ptype=1时才有效。
} starg;

struct st_fileinfo
{
    char filename[301];     // list文件中的一个文件名
    char mtime[21];         // 文件时间
};

vector<struct st_fileinfo> vfileinfo1; // 已下载成功的文件名的容器，从okfilename中加载。
vector<struct st_fileinfo> vfileinfo2; // 存放下载前列出服务器文件名的容器，从nlist文件中加载。
vector<struct st_fileinfo> vfileinfo3; // 本次不需要下载的文件名的容器
vector<struct st_fileinfo> vfileinfo4; // 本次需要下载的文件名的容器

CLogFile logfile;
Cftp ftp;

// 加载okfilename文件中的内容到容器vlistfile1中。
bool LoadOKFile();
// 比较vlistfile2和vlistfile1，得到vlistfile3和vlistfile4。
bool CompVector();
// 把容器vlistfile3中的内容写入到okfilename文件，覆盖之前的旧okfilename文件。
bool WriteToOKFile();
// 将下载成功的文件追加到okfilename中。
bool AppendToOKFile(struct st_fileinfo *stfileinfo);
void _help();
void EXIT(int sig);
bool _xmltoarg(char *strxmlbuffer);
bool _ftpgetfiles();
bool LoadListFile();

int main(int argc, char *argv[])
{
    // 写帮助文档
    if (argc != 3) { _help(); return -1; }
    
    // 处理程序的信号和IO
    // CloseIOAndSignal(true);
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);
    // 在整个程序编写完成且运行稳定后，关闭IO和信号。为了方便调试，暂时不启用CloseIOAndSignal();

    // 日志文件
    if ( logfile.Open(argv[1]) == false )
    { printf("logfile.Open(%s) failed!\n", argv[1]); return -1; }

    // 解析xml字符串，获得参数
    if ( _xmltoarg(argv[2]) == false )
    { logfile.Write("解析xml失败！\n"); return -1; }

    // 登录ftp服务器
    if ( ftp.login(starg.host, starg.username, starg.password, starg.mode) == false )
    {
        logfile.Write("ftp.login(%s, %s, %s, %d) failed!\n", starg.host, starg.username, starg.password, starg.mode);
        return -1;
    }
    logfile.Write("ftp.login() successed\n");
    
   if ( _ftpgetfiles() == false )
    { logfile.Write("下载文件失败！\n"); return -1; }

    return 0;
}

void _help()
{
    printf("\n");
    printf("Use: ftpgetfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools1/bin/procctl 30 /project/tools1/bin/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log \"<host>127.0.0.1:21</host><mode>1</mode><username>lighthouse</username><password>dyt.9525</password><localpath>/idcdata/surfdata</localpath><remotepath>/tmp/idc/surfdata</remotepath><matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname><listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename><ptype>1</ptype><remotepathbak>/tmp/idc/surfdatabak</remotepathbak><okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename>\"\n\n\n");

    printf("本程序是通用的功能模块，用于把远程ftp服务器的文件下载到本地目录。\n");
    printf("logfilename 是本程序的运行日志文件。\n");
    printf("xmlbuffer   为文件下载的参数，如下：\n");
    printf("    <host></host>                   远程服务器的IP和端口。\n");
    printf("    <mode></mode>                   传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("    <username></username>           远程服务器的用户名。\n");
    printf("    <password></password>           远程服务器的密码。\n");
    printf("    <localpath></localpath>         本地文件存放的目录。\n");
    printf("    <remotepath></remotepath>       远程服务器存放文件的目录。\n");
    printf("    <matchname></matchname>         待下载文件匹配的规则。不匹配的文件不会被下载，本字段尽可能设置精确，不建议用*匹配全部文件。\n");
    printf("    <listfilename></listfilename>   下载前列出文件名的文件。\n");
    printf("    <ptype></ptype>                 文件下载成功后，远程服务器文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("    <remotepathbak></remotepathbak> 文件下载成功后，服务器文件的备份目录，此参数只有当ptype=3时才有效。\n");
    printf("    <okfilename></okfilename>       已下载成功文件名清单，此参数只有当ptype=1时才有效。\n\n\n");
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

    GetXMLBuffer(strxmlbuffer, "matchname", starg.matchname, 100);              // 待下载文件匹配的规则
    if (strlen(starg.matchname) == 0)
    { logfile.Write("matchname值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "listfilename", starg.listfilename, 100);        // 下载前列出服务器文件名的文件
    if (strlen(starg.listfilename) == 0)
    { logfile.Write("listfilename值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "ptype", &starg.ptype);                          // 文件下载成功后，远程服务器文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。
    if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3) )
    { logfile.Write("ptype值不正确！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "remotepathbak", starg.remotepathbak, 300);      // 文件下载成功后，服务器文件的备份目录，此参数只有当ptype=3时才有效。
    if ( (strlen(starg.remotepathbak) == 0) && (starg.ptype == 3) )
    { logfile.Write("remotepathbak值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "okfilename", starg.okfilename, 300);            // 已下载成功文件名清单，此参数只有当ptype=1时才有效。      
    if ( (strlen(starg.okfilename) == 0) && (starg.ptype == 1) )
    { logfile.Write("okfilename值为空！\n"); return false; }

    return true;
}

// 下载文件的功能函数
bool _ftpgetfiles()
{
    // 进入ftp服务器存放文件的目录
    if ( ftp.chdir(starg.remotepath) == false )
    {
        logfile.Write("ftp.chdir(%s) failed!\n", starg.remotepath); return false;
    }
    
    // 调用ftp.nlist()方法列出服务器目录中的文件，结果存放到本地文件中。
    if ( ftp.nlist(".", starg.listfilename) == false )
    {
        logfile.Write("ftp.nlist(%s) failed!\n", starg.remotepath); return false;
    }
    
    // 把ftp.nlist()方法获取到的list文件加载到容器vfilelist中。
    if ( LoadListFile() == false )
    {
        logfile.Write("将获取到的list文件写入vfilelist失败！\n"); return false;
    }

    if (starg.ptype == 1)
    {
        // 加载okfilename文件中的内容到容器vlistfile1中。
        LoadOKFile();

        // 比较vlistfile2和vlistfile1，得到vlistfile3和vlistfile4。
        CompVector();

        // 把容器vlistfile3中的内容写入到okfilename文件，覆盖之前的旧okfilename文件。
        WriteToOKFile();

        // 把vlistfile4中的内容复制到vlistfile2中。这是为了兼容下方的代码。在我们得到的结果中v4是待下载的文件，而下方代码中v2才是待下载的文件。
        vfileinfo2.clear(); vfileinfo2.swap(vfileinfo4);
    }
    
    // 遍历容器vfilelist
    char strremotefilename[301], strlocalfilename[301];
    for (int ii = 0; ii < vfileinfo2.size(); ii++)
    {
        // 调用ftp.get()方法从服务器下载文件。

        // 将绝对路径的文件名拼接起来 
        SNPRINTF(strremotefilename, sizeof(strremotefilename), 300, "%s/%s", starg.remotepath, vfileinfo2[ii].filename);
        SNPRINTF(strlocalfilename, sizeof(strlocalfilename), 300, "%s/%s", starg.localpath, vfileinfo2[ii].filename);

        logfile.Write("get %s ... ", strremotefilename);
        
        if ( ftp.get(strremotefilename, strlocalfilename) == false )
        {
            logfile.WriteEx("failed!\n");
        }
        logfile.WriteEx("successed!\n");
        if (starg.ptype == 1)
            AppendToOKFile(&vfileinfo2[ii]);

        if (starg.ptype == 2)
        {
            if ( ftp.ftpdelete(strremotefilename) == false )
            {
                logfile.Write("ftp.ftpdelete(%s) failed!\n", strremotefilename);
                return false;
            }
        }

        if (starg.ptype == 3)
        {
            char strremotefilenamebak[301];
            SNPRINTF(strremotefilenamebak, sizeof(strremotefilenamebak), 300, "%s/%s", starg.remotepathbak, vfileinfo2[ii].filename);
            if ( ftp.ftprename(strremotefilename, strremotefilenamebak) == false )
            {
                logfile.Write("ftprename(%s, %s) failed!\n", strremotefilename, strremotefilenamebak);
                return false;
            }
        }
    }
    
    return true;
}

bool LoadListFile()
{
    vfileinfo2.clear();
    
    CFile File;
    
    if (File.Open(starg.listfilename, "r") == false)
    {
        logfile.Write("File.Open(%s) failed!\n", starg.listfilename); return false;
    }
    
    struct st_fileinfo stfileinfo;

    while (true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));

        if ( File.Fgets(stfileinfo.filename, 300, true) == false ) break;

        if ( MatchStr(stfileinfo.filename, starg.matchname) == false ) continue;

        vfileinfo2.push_back(stfileinfo);
    }
    // for (int ii = 0; ii < vfileinfo2.size(); ii++)
        // logfile.Write("filename=%s=\n", vfileinfo2[ii].filename);

    return true;
}

// 加载okfilename文件中的内容到容器vlistfile1中。
bool LoadOKFile()
{
    vfileinfo1.clear();
    CFile File;
    
    // 如果是第一次运行，okfilename本身就不存在，并不是错误，所以返回true
    if ( File.Open(starg.okfilename, "r") == false ) return true;

    struct st_fileinfo stfileinfo;

    while(true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));
        
        if ( File.Fgets(stfileinfo.filename, 300, true) == false ) break;
        
        vfileinfo1.push_back(stfileinfo);
    }

    return true;
}

// 比较vlistfile2和vlistfile1，得到vlistfile3和vlistfile4。
bool CompVector()
{
    vfileinfo3.clear(); vfileinfo4.clear();
    
    int ii, jj;
    // 从v2中找文件，第一层for循环不能是遍历v1。v1存放的是客户端下的文件，而我们需要下载的文件应当是服务器上的。
    // 如果遍历v1的话，得到的值可能就是客户端上有的但是服务器上没有的文件
    for (ii = 0; ii < vfileinfo2.size(); ii++)
    {
        for (jj = 0; jj < vfileinfo1.size(); jj++)
        {
            if (strcmp(vfileinfo1[jj].filename, vfileinfo2[ii].filename) == 0)
            {
                vfileinfo3.push_back(vfileinfo2[ii]);
                break;
            }
        }
        // 如果从在v2中，没有找到与v1对应的文件，说明该文件需要被下载，存到v4中
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
        File.Fprintf("%s\n", vfileinfo3[ii].filename);

    return true;
}

// 将下载成功的文件追加到okfilename中。
bool AppendToOKFile(struct st_fileinfo *stfileinfo)
{
    CFile File;
    if ( File.Open(starg.okfilename, "a") == false )
    {
        logfile.Write("File.Open(%s, %s) failed!\n", starg.okfilename, "\"a\""); return false;
    }
    File.Fprintf("%s\n", stfileinfo->filename);

    return true;
}