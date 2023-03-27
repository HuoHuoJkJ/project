#include "_public.h"

#define MAXNUMP_ 1000   // 最大进程数量
#define SHMKEYP_ 0x5095 // 共享内存的key
#define SEMKEYP_ 0x5095 // 信号量的key

struct st_pinfo
{
    int     pid;        // 进程id
    char    pname[51];  // 进程名称
    int     timeout;    // 进程超时时间
    time_t  atime;      // 最后一次心跳时间
};

int main(int argc, char *argv[])
{
    if (argc < 2) { printf("Use:./book1 procname\n"); return -1; }
    
    // 创建/获取共享内存。大小为n*sizeof(st_pinfo)  n 为系统可运行的进程的最大数量(一般为1000)
    int m_shmid = 0;
    if ( (m_shmid = shmget(SHMKEYP_, MAXNUMP_*sizeof(struct st_pinfo), 0640|IPC_CREAT)) == -1 )
    { printf("shmget(%x) failed!\n", SHMKEYP_); return 0; }
    
    CSEM m_sem;
    if ( (m_sem.init(SEMKEYP_)) == false )
    { printf("sem.init(%x) failed!\n", SEMKEYP_); return 0; }

    // 将共享内存的地址链接到当前进程的地址空间中，方便将本进程的数据写入到共享内存中
    struct st_pinfo *m_shm;
    m_shm = (struct st_pinfo *)shmat(m_shmid, 0, 0);

    // 将进程数据写入到共享内存
    struct st_pinfo stpinfo;
    memset(&stpinfo, 0, sizeof(stpinfo));
    stpinfo.pid = getpid();
    STRNCPY(stpinfo.pname, sizeof(stpinfo.pname), argv[1], 50);
    stpinfo.timeout = 30;
    stpinfo.atime = time(0);
    
    /* 
     * 在以下代码中存在隐患。如果之前的一个进程异常退出，并且没有执行把进程信息从共享内存中移除的代码。
     * 那么共享内存中就会残留该进程的pid等信息。假如现在有一个进程，它的进程编号恰巧与异常退出的进程的编号相同
     * 那么当守护进程读取共享内存来检查进程心跳时，会误杀死当前进程
     * 因此还需要对共享内存中是否存在与本进程拥有相同pid进行判断，如果有说明，一定是之前进程残留在共享内存当中的
     */
   
    // 在共享内存中找到空余的位置存入进程信息
    int m_pos = -1;

    for (int ii = 0; ii < MAXNUMP_; ii++)
        if ( (m_shm[ii].pid == stpinfo.pid) )
        {
            m_pos = ii;
            break;
        }
    
    m_sem.P();

    if (m_pos == -1)
    {
        for (int ii = 0; ii < MAXNUMP_; ii++)
        {
            if( (m_shm[ii].pid == 0) )
            {
                m_pos = ii;
                break;
            }
        }
    }

    if (m_pos == -1)
    {
        m_sem.V();
        printf("共享内存已用完\n");
        return -1;
    }
    
    memcpy(m_shm+m_pos, &stpinfo, sizeof(struct st_pinfo));

    m_sem.V();
 
    while (true)
    {
        // 更新共享内存中本进程最近的心跳时间
        m_shm[m_pos].atime = time(0); 
        
        sleep(10);
    }
    
    // 程序执行结束后，把进程信息从共享内存中移除，以免共享内存存留进程残留信息
    memset(m_shm+m_pos, 0, sizeof(struct st_pinfo));
    
    // 把共享内存与本进程分离

    return 0;
}