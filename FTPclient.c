/********************************************
**  PROGRAM: FTPclient.c                   **
**  AUTHOR: kenny                          **
**  WRITE DATE:       12/31/1998           **
**  LAST MODIFY DATE: 12/31/1998           **
**  COMMENT: TCP文件服务请求处理程序       **
*********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <setjmp.h>
#include <fcntl.h>
#include <unistd.h>
#include "tcpt.h"

#define  TCP_TIMEOUT       30 
#define  STAT_OK           999
#define  STAT_FAIL         -1
#define  SEND_FILENAME     1
#define  SEND_FILE         2
#define  RECV_FILENAME     3
#define  RECV_FILE         4

//#define  PRINT             if (_nDebug) printf
#define  PRINT              logmsg

static void StopRun();
static void ProcTimeout();
static LONG_32 SndFileToRemote();
static LONG_32 RcvFileFromRemote();

struct {
    LONG_32 trflag;
    LONG_32 datalen;
    char data[1024];
}msgbuf;

char    _sPrgName[]="FTPclient=>";
LONG_32    _lPid=0L;                       /* 进程号  */
char    _sIp[15];                       /* IP地址  */
unsigned short	_nPort=41129;
int     _nsd = -1;
int     _nWaitTime = TCP_TIMEOUT;
char	_sRemoteFileName[80];
char	_sLocalFileName[80];
int     _nWorkFlag = SEND_FILE;
int     _nDebug = 0;
FILE    *fp;


/*
 * ConnectRemoteHost() -- TCP联接
 *   in  para -- Ip: IP地址, Port: 端口号
 *   out para -- 无
 *   return   -- -1: 失败, >0: 成功SOCKET号
 */
int ConnectRemoteHost(char *Ip, unsigned short Port)
{
    int i=0, sd=-1;

    do 
    {
        signal(SIGALRM, ProcTimeout);
        alarm(2);

        sd = ConnectRemote(Ip, Port);

        signal(SIGALRM, SIG_IGN);
        alarm(0);
    } while (sd<0 && i++<3);

    return sd;
}

/*
 * SndFileNameToRemote() --
 *   in  para -- 无
 *   out para -- 无
 *   return   -- -1: 失败, 0: 成功 
 */
int SndFileNameToRemote()
{
    memset(&msgbuf, 0, sizeof(msgbuf));

    if (_nWorkFlag == SEND_FILE)
        msgbuf.trflag = htonl(SEND_FILENAME);
    else
        msgbuf.trflag = htonl(RECV_FILENAME);
    strcpy(msgbuf.data, _sRemoteFileName);
    msgbuf.datalen = htonl(strlen(msgbuf.data));

    if (PutStdMessage(_nsd, (char *)&msgbuf, sizeof(msgbuf)) 
        != sizeof(msgbuf)) 
        return -1;
    return 0;
}


/*
   RcvResponseFromRemote() --
 *   in  para -- 无
 *   out para -- 无
 *   return   -- -1: 失败, 0: 成功 
 */
int RcvResponseFromRemote()
{
    int len;

    signal(SIGALRM, ProcTimeout);
    alarm(_nWaitTime);
  
    memset(&msgbuf, 0, sizeof(msgbuf));
    len = GetStdMessage(_nsd, (char *)&msgbuf, MMAX);
    if (len < 0)
    {
        signal(SIGALRM, SIG_IGN);
        alarm(0);
        return -1;
    }
    signal(SIGALRM, SIG_IGN);
    alarm(0);

    if (atoi(msgbuf.data) == STAT_OK)
        return 0; 

    return -2;
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

        PRINT(".");
        if (endflag == 1) 
            break;
        else
            tlen = tlen + len;

        usleep(500);
    }

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
        //PRINT(".");
    }

    PRINT("\n");
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

    memset(_sIp, 0, sizeof(_sIp));
    memset(_sRemoteFileName, 0, sizeof(_sRemoteFileName));
    memset(_sLocalFileName, 0, sizeof(_sLocalFileName));

    while ((ch=getopt(argc, argv, "h:p:f:F:rsd")) != -1)
    {
        switch ((char)ch)
        {
            case 'h':
                strcpy(_sIp, optarg);
                ret ++;
                break;
			case 'p':
				sscanf(optarg, "%hd", &_nPort);
				break;
            case 'f':
                strcpy(_sRemoteFileName, optarg);
                ret ++;
                break;
            case 'F':
                strcpy(_sLocalFileName, optarg);
                break;
            case 's':
                _nWorkFlag = SEND_FILE;
                break;
            case 'r':
                _nWorkFlag = RECV_FILE;
                break;
            case 'd':
                _nDebug = 1;
                break;
               
	    case '?':
		printf("Unknown option %s ignored\n", argv[optind]);
		break;
	}
}


    if (ret != 2)
        return -1;

    if (strlen(_sLocalFileName) <= 0)
        strcpy(_sLocalFileName, _sRemoteFileName);

    return 0;
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

/*
 * ProcTimeout() -- 超时处理
 *   in  para -- 无
 *   out para -- 无
 *   return   -- 
 */
static void ProcTimeout()
{
    CloseLink();

    signal(SIGALRM, SIG_IGN);
    alarm(0);
}

/*
 * main() -- 主程序
 */
int main(int argc, char *argv[])
{
    int  i;
    LONG_32 len;
    int iInteval = 1;
    int iMaxRetry = 3;

    memset(_sIp, 0, sizeof(_sIp));

    if (BuildOptions(argc, argv) != 0)
    {
        fprintf(stderr, "\nUsage: %s -h remoteip [-p port(41129)] -r|-s(recv|send) -f remotefilename [-F localfilename] [-d] \n", argv[0]);
        exit(1);
    }

    for (i=0; i<64; i++)
        signal(i, SIG_IGN);
    signal(SIGUSR1, StopRun);

    _lPid = getpid();

    for (i = 0; i < iMaxRetry; sleep(iInteval), i++)
    {
        // 打开本地文件
        if(_nWorkFlag == SEND_FILE)
        {
            if ((fp=fopen(_sLocalFileName, "r")) == NULL)
            {
                PRINT("%s 文件[%s] 读操作失败!!\n", _sPrgName, _sLocalFileName); 
                exit(1);
            }
        }
        else
        {
            if ((fp=fopen(_sLocalFileName, "w")) == NULL)
            {
                PRINT("%s 文件[%s] 写操作失败!!\n", _sPrgName, _sLocalFileName); 
                exit(1);
            }
        }

        // TCP联接
        PRINT("%s 正在联接远端主机 IP=[%s], PORT=[%d] ...  \n", _sPrgName, _sIp, _nPort);
        if ((_nsd=ConnectRemoteHost(_sIp, _nPort)) < 0)
        {
            PRINT("%s 联接远端主机失败!\n", _sPrgName);
            //CloseLink();
            fclose(fp);
            exit(1);
        }

        // 发送信息:操作码+远端文件名
        PRINT("%s 正在发送文件名 [%s->%s] ...  \n", _sPrgName, _sLocalFileName, _sRemoteFileName);
        if (SndFileNameToRemote() < 0)
        {
            PRINT("%s 发送文件名失败!\n", _sPrgName);
            CloseLink();
            fclose(fp);
            //exit(1);
            continue;
        }

        // 接收返回
        PRINT("%s 正在接收响应 ...  \n", _sPrgName);
        if (RcvResponseFromRemote() < 0)
        {
            PRINT("%s 远端主机响应失败!\n", _sPrgName);
            CloseLink();
            fclose(fp);
            //exit(1);
            continue;
        }

        // 发文件时:发送文件.., 发送传送结束标识
        if (_nWorkFlag == SEND_FILE)
        {
            PRINT("%s 正在发送文件 [%s] ...  \n", _sPrgName, _sLocalFileName);
            if ((len=SndFileToRemote()) < 0)
            {
                PRINT("%s 发送文件失败!\n", _sPrgName);
                CloseLink();
                fclose(fp);
                //exit(1);
                continue;
            }
            PRINT("%s 发送文件成功! 长度=[%d]字节. \n", _sPrgName, len);
        }

        // 收文件时:接收文件.., 接收到传送结束标识
        if (_nWorkFlag == RECV_FILE)
        {
            PRINT("%s 正在接收文件 [%s] ...  \n", _sPrgName, _sLocalFileName);
            if ((len=RcvFileFromRemote()) < 0)
            {
                PRINT("%s 接收文件失败!\n", _sPrgName);
                CloseLink();
                fclose(fp);
                //exit(1);
                continue;
            }
            PRINT("%s 接收文件成功! 长度=[%d]字节. \n", _sPrgName, len);
        }
        CloseLink();
        fclose(fp);

        break;
    }

    return 0;
}

