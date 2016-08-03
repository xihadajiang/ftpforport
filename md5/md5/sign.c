#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "md5.h"

mainasdf(int argc, char *argv[])
{
	char cmd[20];
	char pTemp[1024];
	char pFileName[100];
	FILE *fp;
	char pPwd[100];

	sprintf(cmd, "l %s > u876yr", argv[1]);
	system(cmd);
	//memset(pFileName, 0, sizeof(pFileName));
	//sprintf(pFileName, "%s", argv[1]);
	//SignFileLocal(pFileName);

	fp = fopen("u876yr", "r");
	sprintf(pPwd, "%s/", argv[1]);
	while(!feof(fp))
	{
		memset(pTemp, 0, sizeof(pTemp));
		fgets(pTemp, 1023, fp);	
		if (memcmp(pTemp, "-", 1))
		{
			continue;	
		}
		memset(pFileName, 0, sizeof(pFileName));
		TrimAll1(pTemp);
		sscanf(pTemp, "%*s %*s %*s %*s %*s %*s %*s %*s %s", pFileName);
		sprintf(pTemp, "%s%s", pPwd, pFileName);
		strcpy(pFileName, pTemp);
		if (strcmp(pPwd, pFileName) == 0)
			break;
		fprintf(stderr, "[%s]\n", pFileName);
		SignFileRemote(pFileName, argv[2]);
		//SignFileLocal(pFileName);
		VerifyFileRemote(pFileName, argv[2]);
	}
	fclose(fp);
	sprintf(cmd, "rm u876yr");
	system(cmd);
	return 0;
}

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
	while(1)
	{
		system("clear");	
		fprintf(stderr, "\n\n%18s********************�ļ�ǩ������********************\n\n", " ");
		fprintf(stderr, "%20s 1. ���ļ�ĩβ�����ļ����ļ�ǩ��\n", " ");
		fprintf(stderr, "%20s 2. ��֤�ļ�ĩβǩ���ĺϷ���\n", " ");
		fprintf(stderr, "%20s 3. ������ǩ���ļ��������ļ���ǩ��\n", " ");
		fprintf(stderr, "%20s 4. ������ǩ���ļ�����֤�ļ�ǩ���ĺϷ���\n", " ");
		fprintf(stderr, "%20s 5. ���ض�Ŀ¼�������ļ����ļ�ĩβ��������ǩ��\n", " ");
		fprintf(stderr, "%20s 6. ���ض�Ŀ¼�������ļ�������ǩ���ļ��м�������ǩ��\n", " ");
		fprintf(stderr, "%20s 0. �˳��ó���", " ");
		fprintf(stderr, "\n\n%18s****************************************************\n\n", " ");
		fprintf(stderr, "%20s    ��ѡ��:", " ");
		scanf("%d", &iSelect);
		switch (iSelect)
		{
			case 0:
				exit(0);
			case 1:	
				fprintf(stderr, "%20s    �������ļ���(����ļ����� ; ����)\n", " ");
				fprintf(stderr, "%20s    �ļ���:", " ");
				memset(pFileName, 0, sizeof(pFileName));
				scanf("%s", pFileName);

				memset(pTemp, 0, sizeof(pTemp));		
				strcpy(pTemp, (char *)StrTok(pFileName, ";"));
				TrimAll(pTemp);
				while(strlen(pTemp) != 0)
				{
					SignFileLocal(pTemp);
					memset(pTemp, 0, sizeof(pTemp));
					strcpy(pTemp, (char *)StrTok(NULL, ";"));
					TrimAll(pTemp);
				}
				fprintf(stderr, "%20s    �ɹ�, �����������!", " ");
				getchar();
				getchar();
				break;
			case 2:	
				fprintf(stderr, "%20s    �������ļ���(����ļ����� ; ����)\n", " ");
				fprintf(stderr, "%20s    �ļ���:", " ");
				memset(pFileName, 0, sizeof(pFileName));
				scanf("%s", pFileName);

				memset(pTemp, 0, sizeof(pTemp));		
				strcpy(pTemp, (char *)StrTok(pFileName, ";"));
				TrimAll(pTemp);
				while(strlen(pTemp) != 0)
				{
					iRet = VerifyFileLocal(pTemp);
					fprintf(stderr, "%20s    [%s][%d]\n", " ", pTemp, iRet);
					memset(pTemp, 0, sizeof(pTemp));
					strcpy(pTemp, (char *)StrTok(NULL, ";"));
					TrimAll(pTemp);
				}
				fprintf(stderr, "%20s    �ɹ�, �����������!", " ");
				getchar();
				getchar();
				break;
			case 3:	
				fprintf(stderr, "%20s    �������ļ���(����ļ����� ; ����)\n", " ");
				fprintf(stderr, "%20s    �ļ���:", " ");
				memset(pFileName, 0, sizeof(pFileName));
				scanf("%s", pFileName);

				fprintf(stderr, "%20s    ����������ǩ���ļ���\n", " ");
				fprintf(stderr, "%20s    �ļ���:", " ");
				memset(pMacFileName, 0, sizeof(pMacFileName));
				scanf("%s", pMacFileName);

				memset(pTemp, 0, sizeof(pTemp));		
				strcpy(pTemp, (char *)StrTok(pFileName, ";"));
				TrimAll(pTemp);
				while(strlen(pTemp) != 0)
				{
					SignFileRemote(pTemp, pMacFileName);
					memset(pTemp, 0, sizeof(pTemp));
					strcpy(pTemp, (char *)StrTok(NULL, ";"));
					TrimAll(pTemp);
				}
				fprintf(stderr, "%20s    �ɹ�, �����������!", " ");
				getchar();
				getchar();
				break;
			case 4:	
				fprintf(stderr, "%20s    �������ļ���(����ļ����� ; ����)\n", " ");
				fprintf(stderr, "%20s    �ļ���:", " ");
				memset(pFileName, 0, sizeof(pFileName));
				scanf("%s", pFileName);

				fprintf(stderr, "%20s    ����������ǩ���ļ���\n", " ");
				fprintf(stderr, "%20s    �ļ���:", " ");
				memset(pMacFileName, 0, sizeof(pMacFileName));
				scanf("%s", pMacFileName);

				memset(pTemp, 0, sizeof(pTemp));		
				strcpy(pTemp, (char *)StrTok(pFileName, ";"));
				TrimAll(pTemp);
				while(strlen(pTemp) != 0)
				{
					iRet = VerifyFileRemote(pTemp, pMacFileName);
					fprintf(stderr, "%20s    [%s][%d]\n", " ", pTemp, iRet);
					memset(pTemp, 0, sizeof(pTemp));
					strcpy(pTemp, (char *)StrTok(NULL, ";"));
					TrimAll(pTemp);
				}
				fprintf(stderr, "%20s    �ɹ�, �����������!", " ");
				getchar();
				getchar();
				break;
			case 5:
				fprintf(stderr, "%20s    ������Ŀ¼����·����(���Ŀ¼���� ; ����)\n", " ");
				fprintf(stderr, "%20s    Ŀ¼·��:", " ");
				memset(pFileName, 0, sizeof(pFileName));
				scanf("%s", pFileName);

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
						SignFileLocal(pTemp);
						fprintf(stderr, "%20s    [%s]\n", " ", pTemp);
					}
					fclose(fp);
					sprintf(cmd, "rm u876yr");
					system(cmd);

					memset(pTemp, 0, sizeof(pTemp));
					strcpy(pTemp, (char *)StrTok(NULL, ";"));
					TrimAll(pTemp);
				}
				fprintf(stderr, "%20s    �ɹ�, �����������!", " ");
				getchar();
				getchar();
				break;
			case 6:
				fprintf(stderr, "%20s    ������Ŀ¼����·����(���Ŀ¼���� ; ����)\n", " ");
				fprintf(stderr, "%20s    Ŀ¼·��:", " ");
				memset(pFileName, 0, sizeof(pFileName));
				scanf("%s", pFileName);

				fprintf(stderr, "%20s    ����������ǩ���ļ���\n", " ");
				fprintf(stderr, "%20s    �ļ���:", " ");
				memset(pMacFileName, 0, sizeof(pMacFileName));
				scanf("%s", pMacFileName);

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
						fprintf(stderr, "%20s    [%s]\n", " ", pTemp);
					}
					fclose(fp);
					sprintf(cmd, "rm u876yr");
					system(cmd);

					memset(pTemp, 0, sizeof(pTemp));
					strcpy(pTemp, (char *)StrTok(NULL, ";"));
					TrimAll(pTemp);
				}
				fprintf(stderr, "%20s    �ɹ�, �����������!", " ");
				getchar();
				getchar();
				break;
			default:
				break;
		}
	}
}
