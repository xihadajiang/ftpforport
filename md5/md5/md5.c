#include <stdio.h>
#include <stdlib.h>
#include "md5.h"

#define BUFSIZE	1024*16
#define MAXDOWNFILENUM	1024

extern errno;
//去掉右边的空格.
char *TrimRight1(char *str)
{
    int i, len;

	len = strlen(str);
	for (i = len - 1; i >= 0; i--)
	{
		if ((str[i] == ' ') || (str[i] == '\n') || (str[i] == '\t'))
			str[i] = '\0';
		else
			break;
	}
	return (str);
}

//去掉左边的空格.
char *TrimLeft1(char *str)
{
	int i, len;

	len = strlen(str);
	for (i=0; i < len; i++)
	{
		if ((str[i] != ' ') && (str[i] != '\t'))
			break;
	}

	if (i>0)
		strcpy(str, str + i);

	return (str);
}
//去掉左右两边的空格.
char *TrimAll1(char *str)
{
    TrimRight1(str);
	TrimLeft1(str);
	return (str);
}


char *StrTok1(char *sourcestr,char *splitstr)
{
	static char srcstr[8192];
	static char retstr[8192];
	int i,l;

	if(sourcestr!=NULL)
		strcpy(srcstr,sourcestr);
	l=strlen(srcstr);
		srcstr[l]=splitstr[0];
	for (i=0;i<l;i++)
	{
		if (srcstr[i]==splitstr[0])
		{
			srcstr[i]='\0';
			break;
		}
	}
	srcstr[l]='\0';
	strcpy(retstr,srcstr);
	if (i == l)
		srcstr[i+1]='\0';
	strcpy(srcstr,srcstr+i+1);
	return retstr;
}


int WriteMac(char *pFileName, char *pMacFileName, char *pFileMac)
{
	FILE *fp;
	struct FileMac
	{
		char *pFileName[128];	
		char *pFileMac[40];
	};
	struct FileMac struFileMac[MAXDOWNFILENUM];
	char pTemp[1024];
	int j, i, iFindFlag;

	if (!strcmp(pMacFileName, NULL))
		return 0;

	fp=fopen(pMacFileName,"r");
	j = 0;
	if (fp == NULL)
	{
		strcpy(struFileMac[j].pFileName, pFileName);
		strcpy(struFileMac[j].pFileMac, pFileMac);
		j ++;
	}
	else
	{
		while(!feof(fp))
		{
			memset(pTemp, 0, sizeof(pTemp));
			fgets(pTemp, 1023, fp);	
			strcpy(struFileMac[j].pFileName, (char*)TrimAll1(StrTok1(pTemp, "^")));
			strcpy(struFileMac[j].pFileMac, (char*)TrimAll1(StrTok1(NULL, "^")));
			if (strlen(struFileMac[j].pFileName) > 0)
			j++;
		}

		iFindFlag = 0;
		for(i=0; i<j; i++)
		{
			if (!strcmp(pFileName, struFileMac[i].pFileName))
			{
				iFindFlag = 1;
				memset(struFileMac[i].pFileMac, 0, sizeof(struFileMac[i].pFileMac));
				strcpy(struFileMac[i].pFileMac, pFileMac);
				break;
			}
		}
		if (iFindFlag == 0)
		{
			strcpy(struFileMac[j].pFileName, pFileName);
			strcpy(struFileMac[j].pFileMac, pFileMac);
			j ++;
		}
	}
	fclose(fp);

	//更新本地的已下载的最新文件列表
	fp = fopen(pMacFileName, "w");
	if (fp == NULL)
		return -1;
    for (i=0; i<j; i++)
    {
		fprintf(fp, "%s^%s^\n", struFileMac[i].pFileName, struFileMac[i].pFileMac);
	}
	fflush(fp);
	fclose(fp);

	return 0;
}

int ReadMac(char *pFileName, char *pMacFileName, char *pFileMac)
{
	FILE *fp;
	char pTemp[1024];
	int j, i, iFindFlag;
	char pName[128];
	char pMac[40];

	fp=fopen(pMacFileName,"r");
	if (fp == NULL)
	{
		return -1;
	}
	iFindFlag = 0;
	while(!feof(fp))
	{
		memset(pTemp, 0, sizeof(pTemp));
		fgets(pTemp, 1023, fp);	
		strcpy(pName, (char*)TrimAll1(StrTok1(pTemp, "^")));
		strcpy(pMac, (char*)TrimAll1(StrTok1(NULL, "^")));
		if (!strcmp(pName, pFileName))
		{
			iFindFlag = 1;
			strcpy(pFileMac, pMac);
			break;
		}
	}

	if (iFindFlag == 0)
	{
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

/*对文件进行MD5数字签名, 将签名写在文件的末尾*/
int SignFileLocal(char *pFileName)
{
	FILE *fp;
	int fd;
	int i;
	MD5_CTX c;
	unsigned char md[MD5_DIGEST_LENGTH];
	char md_str[MD5_DIGEST_LENGTH*2+1];
	static unsigned char buf[BUFSIZE];

	memset(md_str, 0, sizeof(md_str));
	fp=fopen(pFileName,"a+");
	if (fp == NULL)
	{
		return -1;
	}

	rewind(fp);

	fd=fileno(fp);
	MD5_Init(&c);
	for (;;)
	{
		i=read(fd,buf,BUFSIZE);
		if (i <= 0) break;
		MD5_Update(&c,buf,(unsigned long)i);
	}
	MD5_Final(&(md[0]),&c);
	for (i=0; i<MD5_DIGEST_LENGTH; i++)
	{
		if ((md[i]>>4) >9)
			md_str[2*i] = (md[i]>>4)-10+'a';
		else
			md_str[2*i] = (md[i]>>4)+'0';
		if ((md[i] & 0x0f) > 9)
			md_str[2*i+1] = (md[i] & 0x0f)-10+'a';
		else
			md_str[2*i+1] = (md[i] & 0x0f)+'0';
	}
	fprintf(fp, "\n%s", md_str);
	fclose(fp);
	return 0;
}

/*对文件进行MD5数字签名, 将签名写在数字签名文件中*/
int SignFileRemote(char *pFileName, char *pMacFileName)
{
	FILE *fp;
	int fd;
	int i;
	MD5_CTX c;
	unsigned char md[MD5_DIGEST_LENGTH];
	char md_str[MD5_DIGEST_LENGTH*2+1];
	static unsigned char buf[BUFSIZE];

	memset(md_str, 0, sizeof(md_str));
	fp=fopen(pFileName,"r");
	if (fp == NULL)
	{
		return -1;
	}

	rewind(fp);

	fd=fileno(fp);
	MD5_Init(&c);
	for (;;)
	{
		i=read(fd,buf,BUFSIZE);
		if (i <= 0) break;
		MD5_Update(&c,buf,(unsigned long)i);
	}
	MD5_Final(&(md[0]),&c);
	for (i=0; i<MD5_DIGEST_LENGTH; i++)
	{
		if ((md[i]>>4) >9)
			md_str[2*i] = (md[i]>>4)-10+'a';
		else
			md_str[2*i] = (md[i]>>4)+'0';
		if ((md[i] & 0x0f) > 9)
			md_str[2*i+1] = (md[i] & 0x0f)-10+'a';
		else
			md_str[2*i+1] = (md[i] & 0x0f)+'0';
	}
	fclose(fp);

	WriteMac(pFileName, pMacFileName, md_str);

	return 0;
}

/*对带数字签名的文件进行MD5验证*/
int VerifyFileLocal(char *pFileName)
{
	FILE *fp;
	int fd;
	int i;
	long j, lFileLen;
	MD5_CTX c;
	unsigned char md[MD5_DIGEST_LENGTH];
	char md_str[MD5_DIGEST_LENGTH*2+1];
	char oldmd_str[MD5_DIGEST_LENGTH*2+1];
	static unsigned char buf[BUFSIZE];

	memset(md_str, 0, sizeof(md_str));
	memset(oldmd_str, 0, sizeof(oldmd_str));
	fp=fopen(pFileName,"a+");
	if (fp == NULL)
	{
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	lFileLen=ftell(fp)-2*MD5_DIGEST_LENGTH-1;
	if (lFileLen<0)
	{
		fclose(fp);
		return -1;
	}

	rewind(fp);

	fd=fileno(fp);
	MD5_Init(&c);
	for (j=0;j<lFileLen;)
	{
		if (lFileLen-j>=BUFSIZE)
			i = BUFSIZE;
		else
			i = lFileLen-j;
		i=read(fd,buf,i);
		MD5_Update(&c,buf,(unsigned long)i);
		j+=i;
	}
	read(fd,oldmd_str,1);
	read(fd,oldmd_str,MD5_DIGEST_LENGTH*2);
	MD5_Final(&(md[0]),&c);
	for (i=0; i<MD5_DIGEST_LENGTH; i++)
	{
		if ((md[i]>>4) >9)
			md_str[2*i] = (md[i]>>4)-10+'a';
		else
			md_str[2*i] = (md[i]>>4)+'0';
		if ((md[i] & 0x0f) > 9)
			md_str[2*i+1] = (md[i] & 0x0f)-10+'a';
		else
			md_str[2*i+1] = (md[i] & 0x0f)+'0';
	}
	if (memcmp(oldmd_str, md_str, MD5_DIGEST_LENGTH*2) != 0)
	{
		fclose(fp);
		return -1;
	}
	else
	{
		/*
		ftruncate(fd, lFileLen);
		*/
		fclose(fp);
		return 0;
	} 
}

/*对带数字签名的文件进行MD5验证, 验证成功后将签名删除掉, 并将MAC写入MAC文件*/
int VerifyFileAndMove(char *pFileName, char *pMacFileName)
{
	FILE *fp;
	int fd;
	int i;
	long j, lFileLen;
	MD5_CTX c;
	unsigned char md[MD5_DIGEST_LENGTH];
	char md_str[MD5_DIGEST_LENGTH*2+1];
	char oldmd_str[MD5_DIGEST_LENGTH*2+1];
	static unsigned char buf[BUFSIZE];

	memset(md_str, 0, sizeof(md_str));
	memset(oldmd_str, 0, sizeof(oldmd_str));
	fp=fopen(pFileName,"a+");
	if (fp == NULL)
	{
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	lFileLen=ftell(fp)-2*MD5_DIGEST_LENGTH-1;
	if (lFileLen<0)
	{
		fclose(fp);
		return -1;
	}

	rewind(fp);

	fd=fileno(fp);
	MD5_Init(&c);
	for (j=0;j<lFileLen;)
	{
		if (lFileLen-j>=BUFSIZE)
			i = BUFSIZE;
		else
			i = lFileLen-j;
		i=read(fd,buf,i);
		MD5_Update(&c,buf,(unsigned long)i);
		j+=i;
	}
	read(fd,oldmd_str,1);
	read(fd,oldmd_str,MD5_DIGEST_LENGTH*2);
	MD5_Final(&(md[0]),&c);
	for (i=0; i<MD5_DIGEST_LENGTH; i++)
	{
		if ((md[i]>>4) >9)
			md_str[2*i] = (md[i]>>4)-10+'a';
		else
			md_str[2*i] = (md[i]>>4)+'0';
		if ((md[i] & 0x0f) > 9)
			md_str[2*i+1] = (md[i] & 0x0f)-10+'a';
		else
			md_str[2*i+1] = (md[i] & 0x0f)+'0';
	}
	if (memcmp(oldmd_str, md_str, MD5_DIGEST_LENGTH*2) != 0)
	{
		fclose(fp);
		return -1;
	}
	else
	{
		ftruncate(fd, lFileLen);
		fclose(fp);
		WriteMac(pFileName, pMacFileName, md_str);
		return 0;
	} 
}

/*用数字签名文件对不带数字签名的文件进行MD5验证*/
int VerifyFileRemote(char *pFileName, char *pMacFileName)
{
	FILE *fp;
	int fd;
	int i;
	long j, lFileLen;
	MD5_CTX c;
	unsigned char md[MD5_DIGEST_LENGTH];
	char md_str[MD5_DIGEST_LENGTH*2+1];
	char oldmd_str[MD5_DIGEST_LENGTH*2+1];
	static unsigned char buf[BUFSIZE];
	memset(md_str, 0, sizeof(md_str));
	memset(oldmd_str, 0, sizeof(oldmd_str));
	ReadMac(pFileName, pMacFileName, oldmd_str);
	fp=fopen(pFileName,"r");
	if (fp == NULL)
	{
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	lFileLen=ftell(fp);
	if (lFileLen<0)
	{
		fclose(fp);
		return -1;
	}

	rewind(fp);

	fd=fileno(fp);
	MD5_Init(&c);
	for (j=0;j<lFileLen;)
	{
		if (lFileLen-j>=BUFSIZE)
			i = BUFSIZE;
		else
			i = lFileLen-j;
		i=read(fd,buf,i);
		MD5_Update(&c,buf,(unsigned long)i);
		j+=i;
	}
	MD5_Final(&(md[0]),&c);
	for (i=0; i<MD5_DIGEST_LENGTH; i++)
	{
		if ((md[i]>>4) >9)
			md_str[2*i] = (md[i]>>4)-10+'a';
		else
			md_str[2*i] = (md[i]>>4)+'0';
		if ((md[i] & 0x0f) > 9)
			md_str[2*i+1] = (md[i] & 0x0f)-10+'a';
		else
			md_str[2*i+1] = (md[i] & 0x0f)+'0';
	}
	if (memcmp(oldmd_str, md_str, MD5_DIGEST_LENGTH*2) != 0)
	{
		fclose(fp);
		return -1;
	}
	else
	{
		fclose(fp);
		return 0;
	} 
}


/*对给定长度lBufLen的数据pBuf计算MAC,返回的MAC存放在pMac上,8个字节长度*/
unsigned char *CreateMac_ICBC(pBuf, lBufLen, pMac)
unsigned char *pBuf;
unsigned long lBufLen;
unsigned char *pMac;
{
	int i;
	MD5_CTX c;
	unsigned char pM[MD5_DIGEST_LENGTH];
	static unsigned char pMm[MD5_DIGEST_LENGTH/2];

	if (pMac == NULL) pMac=pMm;
	MD5_Init(&c);
	MD5_Update(&c,pBuf,lBufLen);
	MD5_Final(pM,&c);
	memset(&c,0,sizeof(c)); /* security consideration */
	for(i=0;i<MD5_DIGEST_LENGTH; i+=2)
		pMac[i/2]=pM[i]^pM[i+1];
	return(pMac);
}

/*对给定长度lBufLen的数据pBuf的pMAC进行验证,返回0表示验证通过,否则为验证失败*/
int VerifyMac(pBuf, lBufLen, pMac)
unsigned char *pBuf;
unsigned long lBufLen;
unsigned char *pMac;
{
	unsigned char pMd[MD5_DIGEST_LENGTH/2];

	CreateMac_ICBC(pBuf, lBufLen, pMd);
	if (memcmp(pMd, pMac, 8) == 0)
		return 0;
	else
		return -1;
}
