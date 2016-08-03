/********************************************
**  PROGRAM: FTPserver.c                   **
**  AUTHOR: kenny                          **
**  WRITE DATE:       12/31/1998           **
**  LAST MODIFY DATE: 12/31/1998           **
**  COMMENT: TCP�ļ�������Ӧ��������       **
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
//LONG_32    _lPid=0L;                       /* ���̺�  */
unsigned short	_nPort=41129;
int     _nsd = -1;
int     _nWaitTime = TCP_TIMEOUT;
char	_sRemoteFileName[256];
char	_sLocalFileName[256];
int     _nWorkFlag = SEND_FILE;
int     _nDebug = 0;
FILE    *fp;

/*
 * main() -- ������
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

    // ����Ƿ����ظ�����
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
 * DoProcess() -- ����������
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

    // ȡ����
    PRINT( "%s ���ڽ���Զ����������, PORT=[%d] ...  \n", _sPrgName, _nPort);
    memset(&msgbuf, 0, sizeof(msgbuf));
    if ((len=GetRequestFromRemote((char *)&msgbuf, MMAX)) < 0)
    {
        PRINT( "%s ����Զ����������ʧ��!\n", _sPrgName);
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

    // �򿪱����ļ�
    if(_nWorkFlag == RECV_FILE)
    {
        if ((fp=fopen(_sLocalFileName, "r")) == NULL)
        {
            PRINT( "%s �ļ�[%s] ������ʧ��!!\n", _sPrgName, _sLocalFileName); 
            ret = -1;
        }
    }
    else
    {
        if ((fp=fopen(_sLocalFileName, "w")) == NULL)
        {
            PRINT( "%s �ļ�[%s] д����ʧ��!!\n", _sPrgName, _sLocalFileName); 
            ret = -1;
        }
    }

    // ������Ӧ��Ϣ
    if (_nWorkFlag  == SEND_FILE)
    {
        if (ret == -1)
            strcpy(temp, "�����������ļ�:");
        else
            strcpy(temp, "���������ļ�:");
    }
    else
    {
        if (ret == -1)
            strcpy(temp, "�������´��ļ�:");
        else
            strcpy(temp, "�����´��ļ�:");
    }
    PRINT( "%s ���ڷ�����Ӧ [%s%s] ...  \n", _sPrgName, temp, _sLocalFileName);
    if (ret == -1)
        sprintf(msgbuf.data, "%d", STAT_FAIL);
    else
        sprintf(msgbuf.data, "%d", STAT_OK);
    msgbuf.datalen = htonl(strlen(msgbuf.data));

    if (PutResponseToRemote((char *)&msgbuf, sizeof(msgbuf)) < 0)
    {
        PRINT( "%s ������Ӧʧ��!\n", _sPrgName);
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

    // �´��ļ�ʱ:�����ļ�.., ���͵����ͽ�����ʶ
    if (_nWorkFlag == RECV_FILE)
    {
        PRINT( "%s �����´��ļ� [%s] ...  \n", _sPrgName, _sLocalFileName);
        if ((len=SndFileToRemote()) < 0)
        {
            PRINT( "%s �����´�ʧ��!\n", _sPrgName);
            if (fp != NULL)
                fclose(fp);
            return -1;
        }
        PRINT( "%s �´��ļ��ɹ�! ����=[%ld]�ֽ�. \n", _sPrgName, len);
    }

    // �����ļ�ʱ:�����ļ�.., ���մ��ͽ�����ʶ
    if (_nWorkFlag == SEND_FILE)
    {
        PRINT( "%s ���ڽ����ļ� [%s] ...  \n", _sPrgName, _sLocalFileName);
        if ((len=RcvFileFromRemote()) < 0)
        {
            PRINT( "%s �����ļ�ʧ��!\n", _sPrgName);
            if (fp != NULL)
                fclose(fp);
            return -1;
        }
        PRINT( "%s �����ļ��ɹ�! ����=[%ld]�ֽ�. \n", _sPrgName, len);
    }

    if (fp != NULL)
        fclose(fp);
    return 0;
}

/*
 * InstallLink() -- �ȴ�TCP����
 *   in  para -- port: �˿ں�
 *   out para -- ��
 *   return   -- -1: ʧ��, >0: �ɹ�SOCKET��
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
 *   out para -- ��
 *   return   -- -1: ʧ��, >0: �ɹ�����
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
 *   out para -- ��
 *   return   -- -1: ʧ��, 0: �ɹ�
 */
int PutResponseToRemote(char *P_Buf, int P_Len) 
{
    if (PutStdMessage(_nsd, P_Buf, P_Len) != P_Len)
        return -1;
    return 0;
}


/*
   SndFileToRemote() -- 
 *   in  para -- ��
 *   out para -- ��
 *   return   -- -1: ʧ��, >=0: �ɹ� 
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
 *   in  para -- ��
 *   out para -- ��
 *   return   -- -1: ʧ��, >=0: �ɹ� 
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
 * BuildOptions() -- �����������ȷ��ϵͳ����
 *   para   -- argc: �����������
 *             argv: �������ֵ
 *   return --0: �ɹ�, -1: ʧ��
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
 * StopRun() -- �رս���
 *   in  para -- ��
 *   out para -- ��
 *   return   -- ��
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
 * CloseLink() -- �ر�TCP����
 *   in  para -- ��
 *   out para -- ��
 *   return   -- -1: ʧ��, 0: �ɹ�
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
 * ProcTimeout() -- ��ʱ����
 *   in  para -- ��
 *   out para -- ��
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
            fprintf(stderr, "�����Ѿ�����. ��鿴 PID �ļ�:%s.", pid_file);
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