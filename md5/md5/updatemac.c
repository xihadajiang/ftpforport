#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "md5.h"

main(int argc, char *argv[])
{
	char cmd[1024];
	char pTemp[1024];
	char pFN[1024];
	char pFileName[4096];
	char pMacFileName[4096];
	FILE *fp;
	char pPwd[1024];
	int i, iSelect, iRet;
	
	for(i=0; i<MAXSIG; i++)
		signal(i, SIG_IGN);

	sprintf(pFileName, "%s/etc;%s/frm;%s/dat", getenv("HOME"), getenv("HOME"), getenv("HOME"));
	sprintf(pMacFileName, "%s/etc/file.mac", getenv("HOME"));

	memset(pTemp, 0, sizeof(pTemp));		
	strcpy(pTemp, (char *)StrTok(pFileName, ";"));
	TrimAll(pTemp);

	while(strlen(pTemp) != 0)
	{
		sprintf(cmd, "l %s > u876yr", pTemp);
		system(cmd);

		fp = fopen("u876yr", "r");
		sprintf(pPwd, "%s/", pTemp);
		while(!feof(fp))
		{
			memset(pTemp, 0, sizeof(pTemp));
			fgets(pTemp, 1023, fp);	
			if (memcmp(pTemp, "-", 1))
			{
				continue;	
			}
			memset(pFN, 0, sizeof(pFN));
			TrimAll1(pTemp);
			sscanf(pTemp, "%*s %*s %*s %*s %*s %*s %*s %*s %s", pFN);
			sprintf(pTemp, "%s%s", pPwd, pFN);
			if (strcmp(pPwd, pTemp) == 0)
				break;
			SignFileRemote(pTemp, pMacFileName);
			fprintf(stderr, "[%s]\n", pTemp);
		}
		fclose(fp);
		sprintf(cmd, "rm u876yr");
		system(cmd);

		memset(pTemp, 0, sizeof(pTemp));
		strcpy(pTemp, (char *)StrTok(NULL, ";"));
		TrimAll(pTemp);
	}
	fprintf(stderr, "参数文件MAC更新成功!\n");
	return 0;
}
