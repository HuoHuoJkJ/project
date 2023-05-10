#include "_public.h"

CLogFile logfile;

int main(int argc, char *argv[])
{
    // 程序的使用说明
    if (argc != 2)
    {
        printf("\n");
        printf("Error! Use:./checkproc logfilename\n");
        printf("Example:/project/tools1/bin/procctl 10 /project/tools1/bin/checkproc /log/idc/checkprox.log\n\n");

        printf("本程序用于检查后台服务程序是否超时，如果以超时，就终止它。\n");
        printf("注意：\n");
        printf("    1） 本程序由procctl启动，运行周期建议为10秒。\n");
        printf("    2） 普通用户无法杀死root用户的进程。为了避免被普通用户误杀，本程序应该用root用户启动。\n\n\n");

        return 0;
    }

    // 打开日志文件
    if ( logfile.Open(argv[1], "a+", false) == false)
    { printf("logfile.Open(%s) failed!", argv[1]); return -1; }

    // 创建/获取共享内存，键值为SHMKEYP，大小为MAXNUMP个st_procinfo结构体的大小。
    int shmid = 0;
    if ( (shmid = shmget((key_t)SHMKEYP, MAXNUMP*sizeof(struct st_procinfo), 0666|IPC_CREAT)) == -1 )
    { logfile.Write("创建/获取共享内存(%x)失败。\n", SHMKEYP); return false; }

    // 将共享内存链接到当前进程的地址空间
    struct st_procinfo *shm = (struct st_procinfo *)shmat(shmid, 0, 0);

    for (int ii = 0; ii < MAXNUMP; ii++)
    {
        // 如果ii位置的pid为0 continue
        if (shm[ii].pid == 0) continue;

        // 如果ii位置存在pid，向该pid发送0信号，判断进程是否存活
        if (int iret = kill(shm[ii].pid, 0) == -1)
        {
            memset(&shm[ii], 0, sizeof(struct st_procinfo));
            continue;
        }

        // 如果ii位置的pid不为0，向该进程发送0的信号，判断进程是否存活
        if (shm[ii].pid != 0)
            logfile.Write("ii=%d, pid=%d, pname=%s, timeout=%d, atime=%d\n",\
                           ii, shm[ii].pid, shm[ii].pname, shm[ii].timeout, shm[ii].atime);

        if ( (time(0) - shm[ii].atime) > shm[ii].timeout )
        {
            int iret = 0;
            logfile.Write("编号为%d pid为%d 进程名为%s 的进程已经超时，尝试正常终止它\n",ii, shm[ii].pid, shm[ii].pname);
            kill(shm[ii].pid, 15);
            for (int jj = 0; jj < 5; jj++)
            {
                sleep(1);
                if (iret = kill(shm[ii].pid, 0) == -1)
                    break;
            }
            if (iret == -1)
            {
                logfile.Write("编号为%d pid为%d 进程名为%s 的进程已经超时，已经正常终止\n",\
                               ii, shm[ii].pid, shm[ii].pname);
            }
            else
            {
                logfile.Write("编号为%d pid为%d 进程名为%s 的进程已经超时，尝试正常终止失败，正在进行强制终止\n",\
                               ii, shm[ii].pid, shm[ii].pname);
                kill(shm[ii].pid, 9);
            }
            memset(&shm[ii], 0, sizeof(struct st_procinfo));
        }
    }

    // 把共享内存从当前进程中分离。
    shmdt(shm);

    return 0;
}