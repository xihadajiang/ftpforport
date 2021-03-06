/********************************************
**  PROGRAM: FTPserver.c                   **
**  AUTHOR: kenny                          **
**  WRITE DATE:       12/31/1998           **
**  LAST MODIFY DATE: 12/31/1998           **
**  COMMENT: TCP文件服务响应处理程序       **
*********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <setjmp.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "tcpt.h"

#define  TCP_TIMEOUT       30 
#define  STAT_OK           999
#define  STAT_FAIL         -1
#define  SEND_FILENAME     1
#define  SEND_FILE         2
#define  RECV_FILENAME     3
#define  RECV_FILE         4
#define ERROR_MSG(msg...) \
    do \
    { \
        fprintf(stderr, msg);\
        fprintf(stderr, "\n");\
        fflush(stderr);\
        exit(-1); \
    } while(0)

//#define  PRINT             if (_nDebug) printf
#define  PRINT              logmsg

static void StopRun();
static void sig_chld();
static void ProcTimeout();
static LONG_32 SndFileToRemote();
static LONG_32 RcvFileFromRemote();
void pid_check(const char *pid_file);
extern int InstallLink(unsigned short port);
extern int BuildOptions(int argc, char **argv);
extern int GetRequestFromRemote(char *P_Buf, int P_Len);
extern int PutResponseToRemote(char *P_Buf, int P_Len);
extern int CloseLink();
extern int DoProcess();

const char *PID_FILE = "/tmp/ftp_server.pid";

struct {
    LONG_32 trflag;
    LONG_32 datalen;
    char data[1024];
}msgbuf;

char    _sPrgName[]="FTPserver=>";
//LONG_32    _lPid=0L;                       /* 进程号  */
unsigned short	_nPort=41129;
int     _nsd = -1;
int     _nWaitTime = TCP_TIMEOUT;
char	_sRemoteFileName[256];
char	_sLocalFileName[256];
int     _nWorkFlag = SEND_FILE;
int     _nDebug = 0;
FILE    *fp;

/*
 * main() -- 主程序
 */
int main(int argc, char *argv[])
{
    int  ret;
    int  listenfd;

    if (BuildOptions(argc, argv) != 0)
    {
        fprintf(stderr, "\nUsage: %s [-p port(41129)] [-d] \n", argv[0]);
        exit(1);
    }

    signal(SIGINT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    if ((ret = fork()) < 0)
        ERROR_MSG("%s: SERVER START FAILED!!!", _sPrgName);
    else if (ret > 0)
        exit(0);

    if (setpgrp() < 0)
        ERROR_MSG("%s: CHANGE GROUP ERROR!!!", _sPrgName);

    // 检查是否有重复进程
    pid_check(PID_FILE);

    signal(SIGHUP, SIG_IGN);
    signal(SIGTERM, StopRun);
    signal(SIGCHLD, sig_chld);

    //_lPid = getpid();

    if ((listenfd = ListenRemote(_nPort)) < 0)
    {
        logmsg("ListenRemote failed.");
        ERROR_MSG("%s: Listen ERROR!!!", _sPrgName);
        //return -1;
    } 

    while (1)
    {
        /*
        if ((_nsd=InstallLink(_nPort)) == -1)
        {
            PRINT("InstallLink failed.\n");
            CloseLink();
            //usleep(1000);
            continue;
        }
        */
        if ((_nsd = AcceptRemote(listenfd)) < 0)
        {
            logmsg("AcceptRemote failed.");
            return -1;
        }

        if ((ret=fork()) == 0)
        {
            close(listenfd);
            DoProcess();
            CloseLink();
            exit(0);
        }
        else if (ret < 0)
        {
            close(_nsd);
            PRINT("%s fork() err", _sPrgName);
        }
        else
        {
            close(_nsd);
        }
    }

    return 0;
}

/*
 * DoProcess() -- 主处理函数
 *   in  para --
 *   out para --
 *   return   --
 */
int DoProcess()
{
    LONG_32 len=0L;
    int  ret=0;
    char temp[50];

    //_lPid = getpid();

    // 取请求
    PRINT( "%s 正在接收远端主机请求, PORT=[%d] ...  \n", _sPrgName, _nPort);
    memset(&msgbuf, 0, sizeof(msgbuf));
    if ((len=GetRequestFromRemote((char *)&msgbuf, MMAX)) < 0)
    {
        PRINT( "%s 接收远端主机请求失败!\n", _sPrgName);
        return -1;
    }

    logmsg("msgdata=[%s]", msgbuf.data);
    if (ntohl(msgbuf.trflag) == SEND_FILENAME)
        _nWorkFlag = SEND_FILE;
    else if (ntohl(msgbuf.trflag) == RECV_FILENAME)
        _nWorkFlag = RECV_FILE;
    else
    {
        logmsg("trflag error: [%d].", ntohl(msgbuf.trflag));
        return -1;
    }
    strcpy(_sLocalFileName, msgbuf.data);

    // 打开本地文件
    if(_nWorkFlag == RECV_FILE)
    {
        if ((fp=fopen(_sLocalFileName, "r")) == NULL)
        {
            PRINT( "%s 文件[%s] 读操作失败!!\n", _sPrgName, _sLocalFileName); 
            ret = -1;
        }
    }
    else
    {
        if ((fp=fopen(_sLocalFileName, "w")) == NULL)
        {
            PRINT( "%s 文件[%s] 写操作失败!!\n", _sPrgName, _sLocalFileName); 
            ret = -1;
        }
    }

    // 发送响应信息
    if (_nWorkFlag  == SEND_FILE)
    {
        if (ret == -1)
            strcpy(temp, "不允许上送文件:");
        else
            strcpy(temp, "允许上送文件:");
    }
    else
    {
        if (ret == -1)
            strcpy(temp, "不允许下传文件:");
        else
            strcpy(temp, "允许下传文件:");
    }
    PRINT( "%s 正在发送响应 [%s%s] ...  \n", _sPrgName, temp, _sLocalFileName);
    if (ret == -1)
        sprintf(msgbuf.data, "%d", STAT_FAIL);
    else
        sprintf(msgbuf.data, "%d", STAT_OK);
    msgbuf.datalen = htonl(strlen(msgbuf.data));

    if (PutResponseToRemote((char *)&msgbuf, sizeof(msgbuf)) < 0)
    {
        PRINT( "%s 发送响应失败!\n", _sPrgName);
        if (fp != NULL)
        {
            fsync(fileno(fp));
            fclose(fp);
        }
        return -1;
    }

    if(ret == -1)
    {
        if (fp != NULL)
            fclose(fp);
        return -1;
    }

    // 下传文件时:发送文件.., 发送到传送结束标识
    if (_nWorkFlag == RECV_FILE)
    {
        PRINT( "%s 正在下传文件 [%s] ...  \n", _sPrgName, _sLocalFileName);
        if ((len=SndFileToRemote()) < 0)
        {
            PRINT( "%s 发送下传失败!\n", _sPrgName);
            if (fp != NULL)
                fclose(fp);
            return -1;
        }
        PRINT( "%s 下传文件成功! 长度=[%ld]字节. \n", _sPrgName, len);
    }

    // 上送文件时:接收文件.., 接收传送结束标识
    if (_nWorkFlag == SEND_FILE)
    {
        PRINT( "%s 正在接收文件 [%s] ...  \n", _sPrgName, _sLocalFileName);
        if ((len=RcvFileFromRemote()) < 0)
        {
            PRINT( "%s 接收文件失败!\n", _sPrgName);
            if (fp != NULL)
                fclose(fp);
            return -1;
        }
        PRINT( "%s 接收文件成功! 长度=[%ld]字节. \n", _sPrgName, len);
    }

    if (fp != NULL)
        fclose(fp);
    return 0;
}

/*
 * InstallLink() -- 等待TCP联接
 *   in  para -- port: 端口号
 *   out para -- 无
 *   return   -- -1: 失败, >0: 成功SOCKET号
 */
int InstallLink(unsigned short port)
{
    int initsd, newsd;

    if ((initsd = ListenRemote(port)) < 0)
    {
        logmsg("ListenRemote failed.");
        return -1;
    } 
    if ((newsd = AcceptRemote(initsd)) < 0)
    {
        logmsg("AcceptRemote failed.");
        return -1;
    }
    close(initsd);
    return newsd;
}

/*
 * GetRequestFromRemote() -- 
 *   in  para -- P_Buf:
 *   out para -- 无
 *   return   -- -1: 失败, >0: 成功长度
 */
int GetRequestFromRemote(char *P_Buf, int P_Len) 
{
    int len;

    //PRINT("Now GetStdMessage...");

    signal(SIGALRM, ProcTimeout);
    alarm(_nWaitTime);

    len = GetStdMessage(_nsd, P_Buf, P_Len);

    signal(SIGALRM, SIG_IGN);
    alarm(0);

    if (len <= 0)
    {
        PRINT("GetStdMessage failed.");
        return -1;
    }


    //PRINT("End GetStdMessage!");

    return len;
}

/*
 * PutResponseToRemote() -- 
 *   in  para -- P_Buf: 
 *               P_Len: 
 *   out para -- 无
 *   return   -- -1: 失败, 0: 成功
 */
int PutResponseToRemote(char *P_Buf, int P_Len) 
{
    if (PutStdMessage(_nsd, P_Buf, P_Len) != P_Len)
        return -1;
    return 0;
}


/*
   SndFileToRemote() -- 
 *   in  para -- 无
 *   out para -- 无
 *   return   -- -1: 失败, >=0: 成功 
 */
LONG_32 SndFileToRemote() 
{
    int endflag=0;
    LONG_32 tlen=0L, len=0L;

    while(1)
    {
        memset(&msgbuf, 0, sizeof(msgbuf));

        msgbuf.trflag = htonl(SEND_FILE);

        len = fread(msgbuf.data, 1, 1024, fp);
        if (len < 0)
            return -1;
        if (len == 0)
        {
            endflag = 1;
            memset(msgbuf.data, 'F', 256);  
            len = 256;
        }
        msgbuf.datalen = htonl(len);

        if (PutStdMessage(_nsd, (char *)&msgbuf, sizeof(msgbuf)) != sizeof(msgbuf))
            return -2;

        //PRINT( ".");
        if (endflag == 1) 
            break;
        else
            tlen = tlen + len;

        usleep(500);
    }

    //PRINT( "\n");
    return tlen;
}


/*
   RcvFileFromRemote() --
 *   in  para -- 无
 *   out para -- 无
 *   return   -- -1: 失败, >=0: 成功 
 */
LONG_32 RcvFileFromRemote()
{
    LONG_32 tlen=0L, len=0L;

    while (1)
    {
        memset(&msgbuf, 0, sizeof(msgbuf));

        signal(SIGALRM, ProcTimeout);
        alarm(_nWaitTime);
        if ((len=GetStdMessage(_nsd, (char *)&msgbuf, MMAX)) <= 0) 
        {
            signal(SIGALRM, SIG_IGN);
            return -1; 
        }

        signal(SIGALRM, SIG_IGN);
        if (memcmp(msgbuf.data, "FFFFFFFFFFFF", 10) == 0) 
            break;
        fwrite(msgbuf.data, 1, ntohl(msgbuf.datalen), fp);

        tlen = tlen + ntohl(msgbuf.datalen);
        //PRINT( ".");
    }

    //PRINT( "\n");
    return tlen;
}

/*
 * BuildOptions() -- 根据命令参数确定系统参数
 *   para   -- argc: 命令参数个数
 *             argv: 命令参数值
 *   return --0: 成功, -1: 失败
 */
int BuildOptions(int argc, char **argv)
{
    int ch, ret=0;
    extern char *optarg;
    extern int  optind;

    while ((ch=getopt(argc, argv, "p:d")) != -1)
    {
        switch ((char)ch)
        {
            case 'p':
                sscanf(optarg, "%hd", &_nPort);
                ret ++;
                break;
            case 'd':
                _nDebug = 1;
                break;

            case '?':
                printf("Unknown option %s ignored\n", argv[optind]);
                break;
        }
    }

    return 0;
}

/*
 * StopRun() -- 关闭进程
 *   in  para -- 无
 *   out para -- 无
 *   return   -- 无
 */
static void StopRun()
{
    CloseLink();
    exit(0);
}

static void sig_chld()
{
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        PRINT("child %d terminated.", pid);
    return;
}


/*
 * CloseLink() -- 关闭TCP联接
 *   in  para -- 无
 *   out para -- 无
 *   return   -- -1: 失败, 0: 成功
 */
int CloseLink()
{
    if (_nsd > 0)
    {
        shutdown(_nsd, 2);
        close(_nsd);
        _nsd = -1;
        usleep(2000);
    }
    return 0;
}

/*
 * ProcTimeout() -- 超时处理
 *   in  para -- 无
 *   out para -- 无
 *   return   -- 
 */
static void ProcTimeout()
{
    logmsg("timeout...");
    CloseLink();

    signal(SIGALRM, SIG_IGN);
    alarm(0);
}

void pid_check(const char *pid_file)
{
    struct flock pid_lock;
    char buf[20];
    int pid_fd;
    int pid_tmp;

    pid_lock.l_type = F_WRLCK;
    pid_lock.l_start = 0;
    pid_lock.l_whence = SEEK_SET;
    pid_lock.l_len = 0;

    if((pid_fd = open(pid_file, O_WRONLY|O_CREAT, 0644)) < 0)
    {
        fprintf(stderr, "Failure to open the pid file(%s).", pid_file);
        exit(-255);
    }

    if(fcntl(pid_fd, F_SETLK, &pid_lock) < 0)
    {
        if((errno == EACCES) || (errno == EAGAIN))
        {
            fprintf(stderr, "程序已经运行. 请查看 PID 文件:%s.", pid_file);
            exit(0);
        }
        else
        {
            fprintf(stderr, "Fcntl error in pid_check 1.");
            exit(-253);
        }
    }

    if(ftruncate(pid_fd, 0) < 0)
    {
        fprintf(stderr, "ftruncate error in pid_check 2.");
        exit(-253);
    }

    sprintf(buf, "%ld", (long)getpid());
    if (write(pid_fd, buf, strlen(buf)) != strlen(buf))
    {
        fprintf(stderr,"write error 1.");
        exit(-253);
    }

    if((pid_tmp = fcntl(pid_fd, F_GETFD, 0)) < 0)
    {
        fprintf(stderr,"fcntl error 2.");
        exit(-253);
    }
    pid_tmp |= FD_CLOEXEC;
    if(fcntl(pid_fd, F_SETFD, pid_tmp) < 0)
    {
        fprintf(stderr,"fcntl error 3.");
        exit(-253);
    }
    return;
}
