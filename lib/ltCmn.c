/*===========================================================================*
 * File     : ltCmn.c
 * Release  : 1.02
 * Purpose  : Contains the read and write routines for sending packets to
 *            and from a socket.
 * Contents : ltConnect
 *            ltInitKey
 *            ltEncrypt
 *            ltWrite
 *            ltRead
 *            ltDisconnect
 *
 * Copyright (c) Land Titles Office 1994,1995,1996,1997
 *===========================================================================*/

#include <stdio.h>

#ifdef VAXC

#include <netdb.h>
#include <types.h>
#include <socket.h>
#include <in.h>
#include <inet.h>
#include <ucx$inetdef.h>
#define  SYS_CLOSE close

#elif defined(_WIN32)

#include <winsock2.h>
#define  SYS_CLOSE   closesocket

#else

#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define  SYS_CLOSE close

#endif

#ifdef LT_RESET
#include <setjmp.h>
#endif

#include "ltoapi.h"
#include "ltint.h"

#ifdef LT_NO_BLOCK
#include "ltsrv.h"
#endif

/*===========================================================================
 * Function    : ltConnect
 * Purpose     : opens a connection for reading and writing
 * Globals  in : 
 *         out : 
 * Errors      :
 * Used by     : ltOpen
 * Description : This functions opens up a stream socket connection and uses
 *               gethostbyname(host) to obtain all the host information. This
 *               information is then copied to the structure 'server' of type
 *               sockaddr_in. Then a connection is made to that socket. If
 *               successful, sends and receives are then possible through that
 *               socket or connection
 *===========================================================================*/

short
ltConnect(char *host, short port, SOCKET *connection)
{
    SOCKET                    sock_1 = 0;
    static struct sockaddr_in sock2_name;
    static struct hostent    *hostentptr=0;
    static struct hostent     hostentstruct;
    int                       retval = 0;

    memset(&sock2_name, 0, sizeof(sock2_name));
    memset(&hostentstruct, 0, sizeof(hostentstruct));

    if((sock_1 = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
#ifdef LT_LOG_FILE
        ltWriteErrnoLog("Error calling function socket");
#endif
        return LT_ERR_BAD_OPEN;
    }

    if((hostentptr = gethostbyname(host)) == NULL)
    {
#ifdef LT_LOG_FILE
        ltWrite2Log("gethostbyname - unknown host: ", host);
#endif
        SYS_CLOSE(sock_1);
        return LT_ERR_UNKNOWN_HOST;
    }

    hostentstruct = *hostentptr;

    sock2_name.sin_family = hostentstruct.h_addrtype;
    sock2_name.sin_port   = htons(port);
    sock2_name.sin_addr   = * ((struct in_addr *) hostentstruct.h_addr);

    retval = connect(sock_1, (struct sockaddr *)&sock2_name,
                     sizeof(sock2_name));
    if(retval)
    {
#ifdef LT_LOG_FILE
        ltWriteErrnoLog("Error from function connect");
#endif
        SYS_CLOSE(sock_1);
        return LT_ERR_ON_CONNECT;
    }
    *connection = sock_1;
    return 0;
}

#ifdef LT_ENCRYPT

/*===========================================================================
 * Function    : ltInitKey
 * Purpose     : initialises key stream
 * Globals  in : 
 *         out : 
 * Errors      : 
 * Used by     : 
 * Description :
 *===========================================================================*/

void
ltInitKey(LT_ENC_REG *c, unsigned char *key)
{
    c->r1 = key[0] << 11 | key[1] << 3 | key[2] >> 5;                /* 19 */
    c->r2 = key[2] << 17 | key[3] << 9 | key[4] << 1 | key[5] >> 7;  /* 22 */
    c->r3 = key[5] << 15 | key[6] << 8 | key[7];                     /* 23 */
}

/*===========================================================================
 * Function    : ltEncrypt
 * Purpose     : encrypts a buffer
 * Globals  in : 
 *         out : 
 * Errors      : 
 * Used by     : 
 * Description :
 *===========================================================================*/

#define LT_CLOCK_RN0(rn, s1, s2, s3, s4, s5, s6)                            \
                if(rn & s1)                                                 \
                {                                                           \
                    rn = (rn << 1) & s6 | 0x1 & ( (rn >> s2) ^ (rn >> s3)   \
                                                ^ (rn >> s4) ^ (rn >> s5)); \
                }

#define LT_CLOCK_RN1(rn, s1, s2, s3, s4, s5, s6)                            \
                if((rn & s1) == 0)                                          \
                {                                                           \
                    rn = (rn << 1) & s6 | 0x1 & ( (rn >> s2) ^ (rn >> s3)   \
                                                ^ (rn >> s4) ^ (rn >> s5)); \
                }

#ifndef LT_NO_BLOCK
static
#endif
       void
ltEncrypt(LT_ENC_REG *c, long nBuffer, char *buffer)
{
    unsigned long r1 = c->r1;
    unsigned long r2 = c->r2;
    unsigned long r3 = c->r3;
    long  i;
    short j;
    char  t;

    for(i = 0; i < nBuffer; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(((r1 >> 9) & 0x1) + ((r2 >> 11) & 0x1) + ((r3 >> 11) & 0x1) > 1)
            {
                LT_CLOCK_RN0(r1, 0x200L, 18, 17, 16, 13, 0x7ffff)
                LT_CLOCK_RN0(r2, 0x800L, 21, 20, 16, 12, 0x3fffff)
                LT_CLOCK_RN0(r3, 0x800L, 22, 21, 18, 17, 0x7fffff)
            }
            else
            {
                LT_CLOCK_RN1(r1, 0x200L, 18, 17, 16, 13, 0x7ffff)
                LT_CLOCK_RN1(r2, 0x800L, 21, 20, 16, 12, 0x3fffff)
                LT_CLOCK_RN1(r3, 0x800L, 22, 21, 18, 17, 0x7fffff)
            }

            t = t << 1 | (char)((r1 ^ r2 ^ r3) & 1);
        }

        buffer[i] ^= t;
    }
    c->r1 = r1;
    c->r2 = r2;
    c->r3 = r3;
}

#endif /* LT_ENCRYPT */

/*===========================================================================
 * Function    : ltWrite
 * Purpose     : writes nBuffer bytes to the socket id
 * Globals  in : 
 *         out :
 * Errors      : LT_ERR_ON_SEND
 * Used by     : ltSendMessage
 * Description : if the send buffer is greater than 1024 bytes then the
 *               packet is sent separately in chunks of 1024
 *===========================================================================*/

short
ltWrite(
#ifdef LT_ENCRYPT
        short session,
#endif
                       SOCKET connection, LT_POS nBuffer, char *buffer)
{
    char   *localBuff;
    LT_POS  buffSize;
#ifdef LT_ENCRYPT
    char    decryptBuf[LT_MAX_PACKET];
#endif

    localBuff = buffer;
    buffSize = nBuffer;

    while(buffSize > 0)
    {
        int packetLen = buffSize >= LT_MAX_PACKET ? LT_MAX_PACKET : buffSize;

#ifdef LT_ENCRYPT
        memcpy(decryptBuf, localBuff, packetLen);
#ifdef LT_NO_BLOCK
        if(session >= 0)
#endif
            ltEncrypt(&ltSessions[session].sendReg, packetLen, decryptBuf);
#endif
        if(send(connection,
#ifdef LT_ENCRYPT
                            decryptBuf,
#else
                            localBuff,
#endif
                                       packetLen, 0) != packetLen)
        {
#ifdef LT_LOG_FILE
            ltWriteErrnoLog("Error calling function send");
#endif
            return LT_ERR_ON_SEND;
        }
        if(buffSize >= LT_MAX_PACKET)
            localBuff += LT_MAX_PACKET;
        else
            break;

        buffSize -= LT_MAX_PACKET;
    }
    return 0;
}

/*===========================================================================
 * Function    : ltRead
 * Purpose     : reads nBuffer bytes into buffer
 * Globals  in : 
 *         out :
 * Errors      : LT_ERR_ON_RECEIVE
 * Used by     : receiveMessage().
 * Description : this function repeatedly reads from the connection until
 *               it has read all nBuffer bytes
 *===========================================================================*/

short
ltRead(
#ifdef LT_ENCRYPT
       short session,
#endif
                      SOCKET connection, LT_POS nBuffer, char *buffer)
{
    int    rval;
    LT_POS readLen;
    LT_POS buffIn = 0;

    do
    {
        readLen = nBuffer - buffIn;
        if((rval = recv(connection, buffer + buffIn, (int)readLen, 0)) < 0)
        {
#ifdef LT_LOG_FILE
            ltWriteErrnoLog("Error calling function recv");
#endif
            return LT_ERR_ON_RECEIVE;
        }
        if(rval == 0)
        {
#ifdef LT_LOG_FILE
            ltWriteLog("ltRead: connection was closed");
#endif
            return LT_ERR_ON_RECEIVE;
        }
        buffIn += rval;
    } while(rval != readLen);
#ifdef LT_ENCRYPT
#ifdef LT_NO_BLOCK
    if(session >= 0)
#endif
        ltEncrypt(&ltSessions[session].recvReg, nBuffer, buffer);
#endif
    return 0;
}

/*===========================================================================
 * Function    : ltDisconnect
 * Purpose     : closes a socket id.
 * Globals  in : 
 *         out :
 * Errors      : 
 * Used by     : 
 *===========================================================================*/

short
ltDisconnect(SOCKET connection)
{
    SYS_CLOSE(connection);
    return 0;
}

/*===========================================================================
 * Function    : ltDisconnectSession
 * Purpose     : closes a socket id. associated with a session
 * Globals  in : ltSessions
 *         out :
 * Errors      : 
 * Used by     : 
 *===========================================================================*/

short
ltDisconnectSession(short session)
{
    SYS_CLOSE(ltSessions[session].connection);
    return 0;
}
