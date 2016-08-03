/***************************************************************************/
/*                                                                         */
/* Name : tcpt.h                                                           */
/* Purpose : Simple TCP/IP socket based send and receive primitives        */
/*                                                                         */
/* Date : Oct 25, 95                                                       */
/*                                                                         */
/* Last Modification :                                                     */
/*                                                                         */
/***************************************************************************/

#ifndef _TCPT_H
#define _TCPT_H

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* Macros */
#define	MMAX		2048
#define	USHORT_LEN	(sizeof(short))

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef STX
#define STX                     0x02
#define ETX                     0x03
#endif

typedef int LONG_32;

#ifndef INADDR_NONE
#define	INADDR_NONE	((LONG_32) 0xffffffff)
#endif

/* Function to read/write n bytes from network connection */
int	readn(int sd, char *ptr, int nbyte);
int	writen(int sd, char *ptr, int nbyte);

/* Function to initiate a connection and waiting for a connection */
/* Hostname is cached so that if hostname specified is the same as the 
   previous one no gethostbyname etc are called again */
int ConnectRemote(char *hostname, unsigned short s_remote_port);
int ListenRemote(unsigned short s_own_port);

/* Function to get/put one application level message from socket */
int GetMessage(int sd, char *ptr, int max);
int PutMessage(int sd, char *ptr, int nbytes);
int PutStdMessage(int sd, char *ptr, int nbytes);
int GetStdMessage(int sd, char *ptr, int nbytes);
int AcceptRemote(int sd);
int ListenRemote(unsigned short s_own_port);

/* Function to log message into log file during daemon mode */
void logmsg(char *fmt, ...);

#endif
