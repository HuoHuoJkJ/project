/*
 * 程序名：crtsurfdata.cpp  本程序用于生成全国气象站点观测的分钟数据
 * 作者：丁永涛
 */
#include "_public.h"

// 全国气象站点参数结构体
struct st_stcode
{
    char provname[31];  // 省
    char obtid[11];     // 站号
    char obtname[31];   // 站名
    double lat;         // 纬度
    double lon;         // 经度
    double height;      // 海拔高度
};
// 存放全国气象站点参数的容器
vector<struct st_stcode> vstcode;
// 把站点参数文件中的数据加载到vstcode容器中
bool LoadSTCode(const char *inifile);

// 全国气象站点分钟观测结构体
struct st_surfdata
{
    char obtid[11];     // 站点代码
    char ddatetime[21]; // 数据时间：格式yyyymmddhh24miss
    int  t;             // 气温：单位 0.1摄氏度
    int  p;             // 气压：0.1百帕
    int  u;             // 相对湿度 0-100之间的值
    int  wd;            // 风向 0-360之间
    int  wf;            // 风速：单位 0.1m/s
    int  r;             // 降雨量：0.1mm
    int  vis;           // 能见度：0.1米
};
// 存放站点分钟观测的容器
vector<struct st_surfdata> vsurfdata;
// 模拟生成全国气象站点分钟观测数据，并存入vsurfdata容器中
void CrtSurfData();

CPActive PActive;
CFile File;

// 日志类
CLogFile logfile;
char strddatetime[21];

// 把容器vsurfdata中的全国气象站点分钟观测数据写入文件。
bool CtrSurfFile(const char *outpath, const char *datafmt);

void EXIT(int sig);

int main(int argc, const char *argv[])
{
    // inifile outpath logfile 气象站点参数 气象站点观测数据参数 日志文件
    if ((argc != 5) && (argc != 6))
    {
        printf("Use:./crtsurfdata inifile outpath logfile datafmt [datatime]\n");
        printf("Example:/project/idc1/bin/crtsurfdata /project/idc1/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,csv\n");
        printf("        /project/idc1/bin/crtsurfdata /project/idc1/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,csv 20010911123000\n");
        printf("        /project/tools1/bin/procctl 60 /project/idc1/bin/crtsurfdata /project/idc1/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,csv 20010911123000\n");

        printf("inifile  全国气象站点参数文件名。\n");
        printf("outpath  全国气象站点数据文件存放的目录。\n");
        printf("logfile  本程序运行的日志文件名。\n");
        printf("datafmt  生成数据文件的格式，支持xml、json和csv三种格式，中间用半角逗号分隔。\n");
        printf("datetime 是一个可选参数，表示生成指定时间的数据和文件名\n\n");

        return -1;
    }

    // 关闭全部的信号和输入输出
    // 设置信号在shell中可以被kill正常终止，但请不要用kill -9来终止
    // 这段代码不能放在打开日志文件后面，这样做会是日志文件的io也被关闭
    CloseIOAndSignal(true); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    if (logfile.Open(argv[3], "a+", false) == false)
    {
        printf("logfile.Open(%s) failed.\n", argv[3]);
        return -1;
    }

    logfile.Write("crtsurfdata 开始运行\n");

    PActive.AddPInfo(20, "crtsurfdata");

    // 把站点参数文件中的数据加载到vstcode容器中
    if (LoadSTCode(argv[1]) == false)
        return -1;

    // 获取当前时间，当作观测时间
    memset(&strddatetime, 0, sizeof(strddatetime));
    if (argc == 5) LocalTime(strddatetime, "yyyymmddhh24miss");
    if (argc == 6) STRCPY(strddatetime, strlen(argv[5])+1, argv[5]);

    // 模拟生成全国气象站点分钟观测数据，并存入vsurfdata容器中
    CrtSurfData();

    // 把容器vsurfdata中的全国气象站点分钟观测数据写入文件。
    if (strstr(argv[4], "xml") != 0)    CtrSurfFile(argv[2], "xml");
    if (strstr(argv[4], "json") != 0)   CtrSurfFile(argv[2], "json");
    if (strstr(argv[4], "csv") != 0)    CtrSurfFile(argv[2], "csv");

    logfile.Write("crtsurfdata 运行结束\n");


    return 0;
}

// 把站点参数文件中的数据加载到vstcode容器中
bool LoadSTCode(const char *inifile)
{
    CCmdStr CmdStr;

    // 打开站点参数文件
    if (File.Open(inifile, "r") == false)
    {
        logfile.Write("File.Open(%s) failed.\n", inifile);
        return false;
    }

    char strBuffer[301];
    struct st_stcode stcode;

    while (true)
    {
        // 从站点参数文件中读取一行，如果已经读取完成，跳出循环。
        if (File.Fgets(strBuffer, 300, true) == false)
            break;
        // logfile.Write("=%s=\n", strBuffer); 只是用来测试strBuffer是否读取到了每一行的数据

        // 把读取到的一行拆分。
        CmdStr.SplitToCmd(strBuffer, ",", true);
        if (CmdStr.CmdCount() != 6)
            continue;

        // 把站点参数的每个数据项保存到站点参数结构体中。
        CmdStr.GetValue(0, stcode.provname, 30);
        CmdStr.GetValue(1, stcode.obtid, 10);
        CmdStr.GetValue(2, stcode.obtname, 30);
        CmdStr.GetValue(3, &stcode.lat);
        CmdStr.GetValue(4, &stcode.lon);
        CmdStr.GetValue(5, &stcode.height);

        // 把站点参数结构体放入站点参数容器。
        vstcode.push_back(stcode);
    }

    return true;
}

// 模拟生成全国气象站点分钟观测数据，并存入vsurfdata容器中
void CrtSurfData()
{
    // 播随机数种子
    srand(time(0));

    struct st_surfdata stsurfdata;

    // 遍历气象站点参数的vstcode容器
    for (int ii = 0; ii < vstcode.size(); ii++)
    {
        memset(&stsurfdata, 0, sizeof(struct st_surfdata));

        // 用随机数填充分钟观测数据的结构体
        strncpy(stsurfdata.obtid, vstcode[ii].obtid, 10);
        strncpy(stsurfdata.ddatetime, strddatetime, 14);
        stsurfdata.t=rand()%351;       // 气温：单位，0.1摄氏度
        stsurfdata.p=rand()%265+10000; // 气压：0.1百帕
        stsurfdata.u=rand()%100+1;     // 相对湿度，0-100之间的值。
        stsurfdata.wd=rand()%360;      // 风向，0-360之间的值。
        stsurfdata.wf=rand()%150;      // 风速：单位0.1m/s
        stsurfdata.r=rand()%16;        // 降雨量：0.1mm
        stsurfdata.vis=rand()%5001+100000;  // 能见度：0.1米

        // 把观测数据的结构体放入vsurfdata容器中
        vsurfdata.push_back(stsurfdata);
    }
}

// 把容器vsurfdata中的全国气象站点分钟观测数据写入文件。
bool CtrSurfFile(const char *outpath, const char *datafmt)
{
    // 拼接生成数据的文件名，例如：/tmp/idc/surfdata/SURF_ZH_20230217092200_2254.csv
    char strFileName[301];
    sprintf(strFileName, "%s/SURF_ZH_%s_%d.%s", outpath, strddatetime, getpid(), datafmt);

    // 打开文件
    if (File.OpenForRename(strFileName, "w") == false)
    {
        logfile.Write("File.OpenForRename(%s) failed.\n",strFileName);
        return false;
    }

    if (strcmp(datafmt, "csv") == 0)
        File.Fprintf("站点代码,数据,时间,气温,气压,相对湿度,风向,风速,降雨量,能见度\n");
    if (strcmp(datafmt, "xml") == 0)
        File.Fprintf("<data>\n");
    if (strcmp(datafmt, "json") == 0)
        File.Fprintf("{\n\"data\":[\n");

    for (int ii = 0; ii < vsurfdata.size(); ii++)
    {
        // 写入一条记录
        if (strcmp(datafmt, "csv") == 0)
            File.Fprintf("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n",\
                vsurfdata[ii].obtid, vsurfdata[ii].ddatetime,\
                vsurfdata[ii].t/10.0, vsurfdata[ii].p/10.0, \
                vsurfdata[ii].u, vsurfdata[ii].wd, vsurfdata[ii].wf/10.0,\
                vsurfdata[ii].r/10.0, vsurfdata[ii].vis/10.0);

        if (strcmp(datafmt, "xml") == 0)
            File.Fprintf("<obtid>%s</obtid><ddatetime>%s</ddatetime><t>%.1f</t><p>%.1f</p><u>%d</u><wd>%d</wd><wf>%.1f</wf><r>%.1f</r><vis>%.1f</vis><endl/>\n",\
                vsurfdata[ii].obtid, vsurfdata[ii].ddatetime,\
                vsurfdata[ii].t/10.0, vsurfdata[ii].p/10.0, \
                vsurfdata[ii].u, vsurfdata[ii].wd, vsurfdata[ii].wf/10.0,\
                vsurfdata[ii].r/10.0, vsurfdata[ii].vis/10.0);

        if (strcmp(datafmt, "json") == 0)
        {
            File.Fprintf("{\"obtid\":\"%s\",\"ddatetime\":\"%s\",\"t\":\"%.1f\",\"p\":\"%.1f\",\"u\":\"%d\",\"wd\":\"%d\",\"wf\":\"%.1f\",\"r\":\"%.1f\",\"vis\":\"%.1f\"}",\
                vsurfdata[ii].obtid, vsurfdata[ii].ddatetime,\
                vsurfdata[ii].t/10.0, vsurfdata[ii].p/10.0, \
                vsurfdata[ii].u, vsurfdata[ii].wd, vsurfdata[ii].wf/10.0,\
                vsurfdata[ii].r/10.0, vsurfdata[ii].vis/10.0);
            if (ii < vsurfdata.size() - 1)
                File.Fprintf(",\n");
            else
                File.Fprintf("\n");
        }
    }

    if (strcmp(datafmt, "xml") == 0)
        File.Fprintf("</data>");
    if (strcmp(datafmt, "json") == 0)
        File.Fprintf("]\n}\n");

    // 关闭文件
    File.CloseAndRename();

    // 更改文件的时间属性
    UTime(strFileName, strddatetime);

    logfile.Write("生成数据文件%s成功，数据时间%s，记录数%d。\n", strFileName, strddatetime, vsurfdata.size());

    return true;
}

void EXIT(int sig)
{
    logfile.Write("接收到%d信号，程序即将退出。\n", sig);
    exit(0);
}
