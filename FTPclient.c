/********************************************
**  PROGRAM: FTPclient.c                   **
**  AUTHOR: kenny                          **
**  WRITE DATE:       12/31/1998           **
**  LAST MODIFY DATE: 12/31/1998           **
**  COMMENT: TCP�ļ���������������       **
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
LONG_32    _lPid=0L;                       /* ���̺�  */
char    _sIp[15];                       /* IP��ַ  */
unsigned short	_nPort=41129;
int     _nsd = -1;
int     _nWaitTime = TCP_TIMEOUT;
char	_sRemoteFileName[80];
char	_sLocalFileName[80];
int     _nWorkFlag = SEND_FILE;
int     _nDebug = 0;
FILE    *fp;


/*
 * ConnectRemoteHost() -- TCP����
 *   in  para -- Ip: IP��ַ, Port: �˿ں�
 *   out para -- ��
 *   return   -- -1: ʧ��, >0: �ɹ�SOCKET��
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
 *   in  para -- ��
 *   out para -- ��
 *   return   -- -1: ʧ��, 0: �ɹ� 
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
 *   in  para -- ��
 *   out para -- ��
 *   return   -- -1: ʧ��, 0: �ɹ� 
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
        //PRINT(".");
    }

    PRINT("\n");
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

/*
 * ProcTimeout() -- ��ʱ����
 *   in  para -- ��
 *   out para -- ��
 *   return   -- 
 */
static void ProcTimeout()
{
    CloseLink();

    signal(SIGALRM, SIG_IGN);
    alarm(0);
}

/*
 * main() -- ������
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
        // �򿪱����ļ�
        if(_nWorkFlag == SEND_FILE)
        {
            if ((fp=fopen(_sLocalFileName, "r")) == NULL)
            {
                PRINT("%s �ļ�[%s] ������ʧ��!!\n", _sPrgName, _sLocalFileName); 
                exit(1);
            }
        }
        else
        {
            if ((fp=fopen(_sLocalFileName, "w")) == NULL)
            {
                PRINT("%s �ļ�[%s] д����ʧ��!!\n", _sPrgName, _sLocalFileName); 
                exit(1);
            }
        }

        // TCP����
        PRINT("%s ��������Զ������ IP=[%s], PORT=[%d] ...  \n", _sPrgName, _sIp, _nPort);
        if ((_nsd=ConnectRemoteHost(_sIp, _nPort)) < 0)
        {
            PRINT("%s ����Զ������ʧ��!\n", _sPrgName);
            //CloseLink();
            fclose(fp);
            exit(1);
        }

        // ������Ϣ:������+Զ���ļ���
        PRINT("%s ���ڷ����ļ��� [%s->%s] ...  \n", _sPrgName, _sLocalFileName, _sRemoteFileName);
        if (SndFileNameToRemote() < 0)
        {
            PRINT("%s �����ļ���ʧ��!\n", _sPrgName);
            CloseLink();
            fclose(fp);
            //exit(1);
            continue;
        }

        // ���շ���
        PRINT("%s ���ڽ�����Ӧ ...  \n", _sPrgName);
        if (RcvResponseFromRemote() < 0)
        {
            PRINT("%s Զ��������Ӧʧ��!\n", _sPrgName);
            CloseLink();
            fclose(fp);
            //exit(1);
            continue;
        }

        // ���ļ�ʱ:�����ļ�.., ���ʹ��ͽ�����ʶ
        if (_nWorkFlag == SEND_FILE)
        {
            PRINT("%s ���ڷ����ļ� [%s] ...  \n", _sPrgName, _sLocalFileName);
            if ((len=SndFileToRemote()) < 0)
            {
                PRINT("%s �����ļ�ʧ��!\n", _sPrgName);
                CloseLink();
                fclose(fp);
                //exit(1);
                continue;
            }
            PRINT("%s �����ļ��ɹ�! ����=[%d]�ֽ�. \n", _sPrgName, len);
        }

        // ���ļ�ʱ:�����ļ�.., ���յ����ͽ�����ʶ
        if (_nWorkFlag == RECV_FILE)
        {
            PRINT("%s ���ڽ����ļ� [%s] ...  \n", _sPrgName, _sLocalFileName);
            if ((len=RcvFileFromRemote()) < 0)
            {
                PRINT("%s �����ļ�ʧ��!\n", _sPrgName);
                CloseLink();
                fclose(fp);
                //exit(1);
                continue;
            }
            PRINT("%s �����ļ��ɹ�! ����=[%d]�ֽ�. \n", _sPrgName, len);
        }
        CloseLink();
        fclose(fp);

        break;
    }

    return 0;
}
