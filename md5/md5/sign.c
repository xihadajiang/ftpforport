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
		fprintf(stderr, "\n\n%18s********************文件签名工具********************\n\n", " ");
		fprintf(stderr, "%20s 1. 在文件末尾生成文件的文件签名\n", " ");
		fprintf(stderr, "%20s 2. 验证文件末尾签名的合法性\n", " ");
		fprintf(stderr, "%20s 3. 在数字签名文件中生成文件的签名\n", " ");
		fprintf(stderr, "%20s 4. 在数字签名文件中验证文件签名的合法性\n", " ");
		fprintf(stderr, "%20s 5. 将特定目录下所有文件在文件末尾加上数字签名\n", " ");
		fprintf(stderr, "%20s 6. 将特定目录下所有文件在数字签名文件中加上数字签名\n", " ");
		fprintf(stderr, "%20s 0. 退出该程序", " ");
		fprintf(stderr, "\n\n%18s****************************************************\n\n", " ");
		fprintf(stderr, "%20s    请选择:", " ");
		scanf("%d", &iSelect);
		switch (iSelect)
		{
			case 0:
				exit(0);
			case 1:	
				fprintf(stderr, "%20s    请输入文件名(多个文件请用 ; 隔开)\n", " ");
				fprintf(stderr, "%20s    文件名:", " ");
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
				fprintf(stderr, "%20s    成功, 按任意键继续!", " ");
				getchar();
				getchar();
				break;
			case 2:	
				fprintf(stderr, "%20s    请输入文件名(多个文件请用 ; 隔开)\n", " ");
				fprintf(stderr, "%20s    文件名:", " ");
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
				fprintf(stderr, "%20s    成功, 按任意键继续!", " ");
				getchar();
				getchar();
				break;
			case 3:	
				fprintf(stderr, "%20s    请输入文件名(多个文件请用 ; 隔开)\n", " ");
				fprintf(stderr, "%20s    文件名:", " ");
				memset(pFileName, 0, sizeof(pFileName));
				scanf("%s", pFileName);

				fprintf(stderr, "%20s    请输入数字签名文件名\n", " ");
				fprintf(stderr, "%20s    文件名:", " ");
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
				fprintf(stderr, "%20s    成功, 按任意键继续!", " ");
				getchar();
				getchar();
				break;
			case 4:	
				fprintf(stderr, "%20s    请输入文件名(多个文件请用 ; 隔开)\n", " ");
				fprintf(stderr, "%20s    文件名:", " ");
				memset(pFileName, 0, sizeof(pFileName));
				scanf("%s", pFileName);

				fprintf(stderr, "%20s    请输入数字签名文件名\n", " ");
				fprintf(stderr, "%20s    文件名:", " ");
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
				fprintf(stderr, "%20s    成功, 按任意键继续!", " ");
				getchar();
				getchar();
				break;
			case 5:
				fprintf(stderr, "%20s    请输入目录绝对路径名(多个目录请用 ; 隔开)\n", " ");
				fprintf(stderr, "%20s    目录路径:", " ");
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
				fprintf(stderr, "%20s    成功, 按任意键继续!", " ");
				getchar();
				getchar();
				break;
			case 6:
				fprintf(stderr, "%20s    请输入目录绝对路径名(多个目录请用 ; 隔开)\n", " ");
				fprintf(stderr, "%20s    目录路径:", " ");
				memset(pFileName, 0, sizeof(pFileName));
				scanf("%s", pFileName);

				fprintf(stderr, "%20s    请输入数字签名文件名\n", " ");
				fprintf(stderr, "%20s    文件名:", " ");
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
				fprintf(stderr, "%20s    成功, 按任意键继续!", " ");
				getchar();
				getchar();
				break;
			default:
				break;
		}
	}
}
