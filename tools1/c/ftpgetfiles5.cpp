/* 
 * 程序名：ftpgetfiles5.cpp 在学习到第四章基于TCP协议的上传下载文件后，温习FTP协议的下载步骤
 * 作者：丁永涛
 */
#include "_ftp.h"

struct st_arg
{
    char host[21];          // FTP登录的服务器的ip地址和使用的命令端口
    int  mode;              // 采用被动(1)/主动(2)模式使用FTP服务器  默认为1，被动模式
    char username[31];      // 登录FTP服务器的用户名
    char password[31];      // 登录FTP服务器的密码
    char remotepath[301];   // 服务器待下载的目录
    char localpath[301];    // 客户端将要下载到的目录
    char matchname[31];     // 待下载的文件的文件名匹配规则
    char nlistfile[301];    // 保存remotepath目录下的文件名的文件    
    int  ptype;             // 设置下载方式  1-仅下载新增和修改文件  2-删除remote目录文件  3-备份remote目录文件
    char remotepathbak[301];// 备份目录。当ptype=3时，进行remotepath目录中已下载的文件的备份
    char okfilename[301];   // 保存localpath目录下的文件信息
    bool checktime;         // 检查文件时间
} starg;

struct st_fileinfo
{
    char filename[301];
    char mtime[21];
};

vector<struct st_fileinfo> vstfileinfo1;  // 保存loaclpath目录中文件的信息
vector<struct st_fileinfo> vstfileinfo2;  // 保存remotepath目录中文件的信息
vector<struct st_fileinfo> vstfileinfo3;  // 不需要进行下载的文件信息存放点
vector<struct st_fileinfo> vstfileinfo4;  // 待下载的文件信息存放点

CLogFile logfile;   // 日志类
Cftp     ftp;       // ftp类

///////////////////运行在main中//////////////////
// 程序运行的帮助文档
void _help();
// 将argv[2]的xml解析到starg中 
bool _xmltoarg(char *xmlbuffer);
// 下载文件
bool _ftpgetfiles();
// 接受信号2/15后进程退出
void EXIT(int sig);
////////////////////////////////////////////////

///////////////_ftpgetfiles子函数////////////////
bool LoadtoNLFile();    // 加载nlistfile文件中的文件内容到vstfileinfo2中
bool LoadtoOKFile();    // 加载okfilename文件中的内容到vstfileinfo1中
bool ComplVector();     // 将vs1与vs2进行对比，得出vs3和vs4
bool WriteOKFile();     // 把容器vlistfile3中的内容写入到okfilename文件，覆盖之前的旧okfilename文件。
bool AppendtoOKFile(struct st_fileinfo *stfileinfo); // 将已经下载的文件写入okfilename中
////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    // 创建帮助文档
    if (argc != 3) { _help(); return -1;}

    // 关闭进程IO和信号，只保留2和15信号
    CloseIOAndSignal(true); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    // 创建日志文件
    if (logfile.Open(argv[1]) == false)
    { printf("logfile.Open(%s) failed!\n", argv[1]); return -1; }

    if (_xmltoarg(argv[2]) == false)
    { logfile.Write("解析xml内容出错！\n"); return -1; }

    if (ftp.login(starg.host, starg.username, starg.password, starg.mode) == false)
    { logfile.Write("登录FTP服务器(%s)失败！\n", starg.host); return -1; }
    logfile.Write("登录FTP服务器(%s)成功！\n", starg.host);

    if (_ftpgetfiles() == false)
    { logfile.Write("下载文件失败！\n"); return -1; }
    
    return 0;
}

// 程序运行的帮助文档
void _help()
{
    printf("Use:ftpgetfiles5 logfile xmlbuffer\n");
    printf("Example:/project/tools1/bin/ftpgetfiles5 /log/tools1/ftpgetfiles5_surfdata.log \"<host>10.0.8.4:21</host><mode>1</mode><username>lighthouse</username><password>dyt.9525</password><remotepath>/log/surfdata</remotepath><localpath>/log/surfdatabak</localpath><matchname>*.XML,*.JSON,*.CSV</matchname><nlistfile>/log/tools1/nlistfile.list</nlistfile><ptype>1</ptype><remotepathbak>/log/surfdataremotebak</remotepathbak><okfilename>/log/okfilename.list</okfilename><checktime>true</checktime>\"\n\n");

    printf("logfile     日志目录存放地点\n");
    printf("xmlbuffer   xml数据\n");
    printf("  <host>10.0.8.4:21</host>                              FTP登录的服务器的ip地址和使用的命令端口\n");
    printf("  <mode>1</mode>                                        采用被动(1)/主动(2)模式使用FTP服务器  默认为1，被动模式\n");
    printf("  <username>lighthouse</username>                       登录FTP服务器的用户名\n");
    printf("  <password>dyt.9525</password>                         登录FTP服务器的密码\n");
    printf("  <remotepath>/log/surfdata</remotepath>                服务器待下载的目录\n");
    printf("  <localpath>/log/surfdatabak</localpath>               客户端将要下载到的目录\n");
    printf("  <matchname>*.XML,*.JSON,*.CSV</matchname>             待下载的文件的文件名匹配规则\n");
    printf("  <nlistfile>/log/tools1/nlistfile.list</nlistfile>     保存remotepath目录下的文件名的文件\n");
    printf("  <ptype>2</ptype>                                      设置下载方式  1-仅下载新增和修改文件  2-删除remote目录文件  3-备份remote目录文件\n");
    printf("  <remotepathbak>/log/surfdataremotebak</remotepathbak> 备份目录。当ptype=3时，进行remotepath目录中已下载的文件的备份\n");
    printf("  <okfilename>/log/okfilename.list</okfilename>         保存localpath目录下的文件信息\n");
    printf("  <checktime>true</checktime>                           检查文件时间\n");
}

// 将argv[2]的xml解析到starg中 
bool _xmltoarg(char *xmlbuffer)
{
    memset(&starg, 0, sizeof(starg));

    GetXMLBuffer(xmlbuffer, "host", starg.host, 20);                // FTP登录的服务器的ip地址和使用的命令端口
    if (strlen(starg.host) == 0)
    { logfile.Write("host值为空！\n"); return false; }

    GetXMLBuffer(xmlbuffer, "mode", &starg.mode);                   // 采用被动(1)/主动(2)模式使用FTP服务器  默认为1，被动模式
    if (starg.mode != 2) starg.mode = 1;

    GetXMLBuffer(xmlbuffer, "username", starg.username, 30);        // 登录FTP服务器的用户名
    if (strlen(starg.username) == 0)
    { logfile.Write("username值为空！\n"); return false; }

    GetXMLBuffer(xmlbuffer, "password", starg.password, 30);        // 登录FTP服务器的密码
    if (strlen(starg.password) == 0)
    { logfile.Write("password值为空！\n"); return false; }

    GetXMLBuffer(xmlbuffer, "remotepath", starg.remotepath, 300);   // 服务器待下载的目录
    if (strlen(starg.remotepath) == 0)
    { logfile.Write("remotepath值为空！\n"); return false; }

    GetXMLBuffer(xmlbuffer, "localpath", starg.localpath, 300);     // 客户端将要下载到的目录
    if (strlen(starg.localpath) == 0)
    { logfile.Write("localpath值为空！\n"); return false; }

    GetXMLBuffer(xmlbuffer, "matchname", starg.matchname, 30);      // 待下载的文件的文件名匹配规则
    if (strlen(starg.matchname) == 0)
    { logfile.Write("matchname值为空！\n"); return false; }
    
    GetXMLBuffer(xmlbuffer, "nlistfile", starg.nlistfile, 30);      // 保存remotepath目录下的文件名的文件
    if (strlen(starg.nlistfile) == 0)
    { logfile.Write("nlistfile值为空！\n"); return false; }
    
    GetXMLBuffer(xmlbuffer, "ptype", &starg.ptype);                 // 设置下载方式  1-仅下载新增和修改文件  2-删除remote目录文件  3-备份remote目录文件
    if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3) )
    { logfile.Write("ptype的值为%d，不符合规范！\n"); return false; }

    GetXMLBuffer(xmlbuffer, "remotepathbak", starg.remotepathbak, 300); // 备份目录。当ptype=3时，进行remotepath目录中已下载的文件的备份
    if (strlen(starg.remotepathbak) == 0)
    { logfile.Write("remotepathbak值为空！\n"); return false; }

    GetXMLBuffer(xmlbuffer, "okfilename", starg.okfilename, 300);   // 保存localpath目录下的文件信息
    if (strlen(starg.okfilename) == 0)
    { logfile.Write("okfilename值为空！\n"); return false; }

    GetXMLBuffer(xmlbuffer, "checktime", &starg.checktime);   // 检查文件时间
    if (starg.checktime != false) starg.checktime = true;

    return true;
}

// 下载文件
bool _ftpgetfiles()
{
    // 进入remotepath目录
    if (ftp.chdir(starg.remotepath) == false)
    { logfile.Write("ftp.chdir(%s) failed!\n", starg.remotepath); return false; }
    
    // 将目录下的内容加载到nlistfile文件中
    if (ftp.nlist(".", starg.nlistfile) == false)
    { logfile.Write("ftp.nlist(%s) failed!\n", starg.nlistfile); return false;}
    
    // 将nlistfile文件中的内容加载到容器vstfileinfo2中，并进行matchname的过滤
    if (LoadtoNLFile() == false)
    { logfile.Write("LoadtoNLFile() failed!\n"); return false; }
    
    if (starg.ptype == 1)
    {
        // 加载okfilename文件中的内容到vstfileinfo21中
        LoadtoOKFile();
        
        // 将vs1与vs2进行对比，得出vs3和vs4
        ComplVector();
        
        // 把容器vlistfile3中的内容写入到okfilename文件，覆盖之前的旧okfilename文件。
        WriteOKFile();
        
        // 由于ptype==2 ==3时，都是使用vst2作为存储待下载文件的存储容器。为了使程序更加灵活，我们需要将vst4转换为vst2
        vstfileinfo2.clear(); vstfileinfo2.swap(vstfileinfo4);
    }

    // 遍历容器，下载文件
    char strremotefile[301], strlocalfile[301];
    for (int ii = 0; ii < vstfileinfo2.size(); ii++)
    {
        memset(strremotefile, 0, sizeof(strremotefile));
        memset(strlocalfile, 0, sizeof(strlocalfile));
        // 拼接出绝对路径
        SNPRINTF(strremotefile, sizeof(strremotefile), 300, "%s/%s", starg.remotepath, vstfileinfo2[ii].filename);
        SNPRINTF(strlocalfile, sizeof(strlocalfile), 300, "%s/%s", starg.localpath, vstfileinfo2[ii].filename);

        logfile.Write("get %s ... ", strremotefile);

        if (ftp.get(strremotefile, strlocalfile) == false)
        { logfile.Write("failed!\n"); return false; }
        logfile.Write("successed!\n");
        
        if (starg.ptype == 1)
            AppendtoOKFile(&vstfileinfo2[ii]);
        
        if (starg.ptype == 2)
        {
            if (ftp.ftpdelete(strremotefile) == false)
            { logfile.Write("ftp.ftpdelete(%s) failed!\n", strremotefile); return false; }
        }
        
        if (starg.ptype == 3)
        {
            char strremotefilebak[301];
            SNPRINTF(strremotefilebak, sizeof(strremotefilebak), 300, "%s/%s", starg.remotepathbak, vstfileinfo2[ii].filename);
            if (ftp.ftprename(strremotefile, strremotefilebak) == false)
            { logfile.Write("ftp.ftprename(%s,%s) failed!\n", strremotefile, strremotefilebak); return false; }
        }
    }

    return true;
}

// 加载nlistfile文件中的文件内容到vstfileinfo2中
bool LoadtoNLFile()
{
    vstfileinfo2.clear();
    CFile file;

    if (file.Open(starg.nlistfile, "r+") == false)
    { logfile.Write("file.Open(%s) failed!\n", starg.nlistfile); return false; }

    struct st_fileinfo stfileinfo;

    while (true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));

        if (file.Fgets(stfileinfo.filename, 300, true) == false) break;

        if (MatchStr(stfileinfo.filename, starg.matchname) == false) continue;
        
        if ((starg.ptype == 1) && (starg.checktime == true))
        {
            if (ftp.mtime(stfileinfo.filename) == false)
            { logfile.Write("ftp.mtime(%s) failed!\n", stfileinfo.filename); return false; }
            strcpy(stfileinfo.mtime, ftp.m_mtime);
        }

        vstfileinfo2.push_back(stfileinfo);
    }

    return true;
}

// 加载okfilename文件中的内容到vstfileinfo1中
bool LoadtoOKFile()
{
    vstfileinfo1.clear();
    CFile file;
    
    // 打开okfilename文件，如果没有读取到，说明是第一次读取。
    if (file.Open(starg.okfilename, "r") == false) return true;
    
    char strxmlbuffer[301];
    
    struct st_fileinfo stfileinfo;

    while (true)
    {
        memset(&stfileinfo, 0, sizeof(struct st_fileinfo));

        if (file.Fgets(strxmlbuffer, 300, true) == false) break;
        
        GetXMLBuffer(strxmlbuffer, "filename", stfileinfo.filename, 300);
        GetXMLBuffer(strxmlbuffer, "mtime", stfileinfo.mtime, 20);

        if (MatchStr(stfileinfo.filename, starg.matchname) == false) continue;

        vstfileinfo1.push_back(stfileinfo);
    }

    return true;
}

// 将vs1与vs2进行对比，得出vs3和vs4
bool ComplVector()
{
    vstfileinfo3.clear(); vstfileinfo4.clear();
    int ii, hh;
    for (ii = 0; ii < vstfileinfo2.size(); ii++)
    {
        for (hh = 0; hh < vstfileinfo1.size(); hh++)
        {
            if (strcmp(vstfileinfo1[hh].filename, vstfileinfo2[ii].filename) == 0 &&
                strcmp(vstfileinfo1[hh].mtime,    vstfileinfo2[ii].mtime)    == 0 )
            { vstfileinfo3.push_back(vstfileinfo2[ii]); break; }
        }
        if (hh == vstfileinfo1.size())
            vstfileinfo4.push_back(vstfileinfo2[ii]);
    }

    return true;
}

// 把容器vlistfile3中的内容写入到okfilename文件，覆盖之前的旧okfilename文件。
bool WriteOKFile()
{
    CFile file;
    if (file.Open(starg.okfilename, "w") == false)
    { logfile.Write("file.Open(%s) failed!\n", starg.okfilename); return false; }
    
    for (int ii = 0; ii < vstfileinfo3.size(); ii++)
        file.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n", vstfileinfo3[ii].filename, vstfileinfo3[ii].mtime);

    return true;
}

// 将已经下载的文件写入okfilename中
bool AppendtoOKFile(struct st_fileinfo *stfileinfo)
{
    CFile file;
    if (file.Open(starg.okfilename, "a") == false)
    {  logfile.Write("File.Open(%s, %s) failed!\n", starg.okfilename, "\"a\""); return false; }
    file.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n", stfileinfo->filename, stfileinfo->mtime);

    return true;
}

// 接受信号2/15后进程退出
void EXIT(int sig)
{
    printf("接收到信号%s，进程退出", sig);
    exit(0);
}