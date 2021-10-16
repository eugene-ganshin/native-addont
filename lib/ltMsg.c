/*===========================================================================
 * File     : ltMsg.c
 * Release  : 1.02
 * Purpose  : message handling
 * Contents : ltOpenLogPid
 *            ltOpenLog
 *            ltWriteLog
 *            ltWrite2Log
 *            ltWriteErrnoLog
 *            ltCloseLog
 *            ltReopenLog
 *            ltGetField
 *            ltGetSeqNum
 *            ltPrepareHeader
 *            ltSendMessage
 *            ltReceiveMessage
 *            ltSendInfo
 *
 * Copyright (c) Land Titles Office 1994,1995,1996,1997,2007
 *===========================================================================*/

#ifdef VAXC

#include <types.h>
#include <errno.h>

#elif defined(_WIN32)

#include <winsock2.h>
#define errno       (WSAGetLastError())
extern int getpid();

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#endif

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#ifdef LT_RESET
#include <setjmp.h>
#endif

#include "ltoapi.h"
#include "ltint.h"

#ifdef LT_NO_BLOCK
#include "ltsrv.h"
#endif

/* the following variables are in this file because they are common to
   client and server and so is this file */
short          ltNSessions = 0;
LT_SESSION     ltSessions[LT_MAX_NUM_OF_SESSIONS];

unsigned char  ltBuffer[LT_MAX_BUFFER_SIZE + 1]; /* note that the +1 is
                             there so that that last byte will be zero so
                             that printf can be used to print the buffer */
#ifdef LT_NO_BLOCK
static FILE *ltLogFp;
#endif

/*===========================================================================
 * Function    : ltOpenLogPid
 * Purpose     : opens the log file for writing
 * Globals  in : ltLogFileName
 *         out : 
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

#define FILE_EXT ".log"

static char ltLogFileName[200];

static FILE *
ltOpenLogPid(void)
{
    char  logName[200];
    FILE *fp;

#ifdef _WIN32
    sprintf(logName, "%s%d_%ld%s", ltLogFileName, getpid(),
                                   GetCurrentThreadId(), FILE_EXT);
#else
    sprintf(logName, "%s%d%s", ltLogFileName, getpid(), FILE_EXT);
#endif

    fp = fopen(logName, "a");
    if(!fp) /* cannot open log file */
    {
        fprintf(stderr, "Cannot open log file\n");
        exit(1);
    }
    return fp;
}

/*===========================================================================
 * Function    : ltOpenLog
 * Purpose     : opens the log file for writing
 * Globals  in :
 *         out : ltLogFp
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

void
ltOpenLog(char *fileName)
{
    char *dirName;

    dirName = getenv("API_LOG_DIR");

    /* save the file name (without pid and extension) for later */
    if(dirName && *dirName)
        sprintf(ltLogFileName, "%s/%s", dirName, fileName);
    else 
        strcpy(ltLogFileName, fileName);
#ifdef LT_NO_BLOCK
    ltLogFp = ltOpenLogPid();
#endif
}

/*===========================================================================
 * Function    : ltWriteLog
 * Purpose     : writes text to the log file
 * Globals  in : ltLogFp
 *         out :
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

void
ltWriteLog(char *text)
{
    time_t t = time((time_t *)0);
    FILE  *fp;
    short  try;

#ifdef LT_NO_BLOCK
    fp = ltLogFp;
#else
    fp = ltOpenLogPid();
#endif
    for(try = 0; try < 2; try++)
        if(fprintf(fp, "%.24s %s: %s\n", ctime(&t), ltClientCode, text) == EOF)
#ifdef LT_NO_BLOCK
            ltLogFp =
#endif
                      fp = ltOpenLogPid();
        else
            break;
    fflush(fp);
#ifndef LT_NO_BLOCK
    fclose(fp);
#endif
}

/*===========================================================================
 * Function    : ltWrite2Log
 * Purpose     : writes two pieces of text to the log file
 * Globals  in : ltLogFp
 *         out :
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

void
ltWrite2Log(char *text, char *text2)
{
    time_t t = time((time_t *)0);
    FILE  *fp;
    short  try;

#ifdef LT_NO_BLOCK
    fp = ltLogFp;
#else
    fp = ltOpenLogPid();
#endif
    for(try = 0; try < 2; try++)
        if(fprintf(fp, "%.24s %s: %s%s\n",
                   ctime(&t), ltClientCode, text, text2) == EOF)
#ifdef LT_NO_BLOCK
            ltLogFp =
#endif
                      fp = ltOpenLogPid();
        else
            break;
    fflush(fp);
#ifndef LT_NO_BLOCK
    fclose(fp);
#endif
}

/*===========================================================================
 * Function    : ltWriteErrnoLog
 * Purpose     : writes text and errno to the log file
 * Globals  in : ltLogFp
 *               errno
 *         out :
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

void
ltWriteErrnoLog(char *text)
{
    int    e = errno;
    time_t t = time((time_t *)0);
    FILE  *fp;
    short  try;

#ifdef LT_NO_BLOCK
    fp = ltLogFp;
#else
    fp = ltOpenLogPid();
#endif
    for(try = 0; try < 2; try++)
        if(fprintf(fp, "%.24s %s: %s, errno %d : %s\n",
                   ctime(&t), ltClientCode, text, e, strerror(e)) == EOF)
#ifdef LT_NO_BLOCK
            ltLogFp =
#endif
                      fp = ltOpenLogPid();
        else
            break;
    /*fprintf(fp, "          %s\n", strerror(e));*/
    fflush(fp);
#ifndef LT_NO_BLOCK
    fclose(fp);
#endif
}

#ifdef LT_NO_BLOCK

/*===========================================================================
 * Function    : ltCloseLog
 * Purpose     : closes the log file
 * Globals  in : ltLogFp
 *         out :
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

void
ltCloseLog(void)
{
    fclose(ltLogFp);
}

/*===========================================================================
 * Function    : ltReopenLog
 * Purpose     : closes and re-opens the log file in case a log file has
 *               been deleted
 * Globals  in : ltLogFp
 *         out : ltLogFp
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

void
ltReopenLog(void)
{
    fclose(ltLogFp);
    ltLogFp = ltOpenLogPid();
}

#endif /* LT_NO_BLOCK */

/*===========================================================================
 * Function    : ltGetField
 * Purpose     : gets a field from a buffer starting from a specified
 *               position and terminated by a "|" or the end of the buffer
 * Globals  in : 
 *         out :
 * Errors      : LT_ERR_FIELD_TOO_LONG
 * Used by     :
 * Description : note that no special action is taken if the field is empty;
 *               leading blanks are ignored; field is always null-terminated,
 *               even when it is empty, and even when maxFieldLen characters
 *               have been put into the field already (so field should look
 *               like:  char field[maxFieldLen + 1];  )
 *===========================================================================*/

short                        /* status */
ltGetField(unsigned char *buf,/* the buffer */
           LT_POS start,     /* start position in buf */
           LT_POS length,    /* last byte of message is at buf[length - 1] */
           char *field,      /* where to put extracted field; null terminated */
           short maxFieldLen,/* max strlen for field; error on overflow */
           LT_POS *next)     /* start of next field after extraction; either
                                next == length or it points to the first char
                                after the "|" at the end of the field just
                                extracted */
{
    LT_POS i;     /* index into input, i.e. buf[] */
    short  j = 0; /* index into output, i.e. field[] */

    for(i = start; i < length; i++)
    {
        char c = buf[i];

        if(c == LT_FIELD_SEPARATOR)
        {
            i++;  /* skip over the field separator */
            break;
        }
        else
        {
            if(j == 0 && c == ' ') continue; /* skip leading blanks */
            if(j >= maxFieldLen) return LT_ERR_FIELD_TOO_LONG;
            field[j++] = c;
        }
    }
    field[j] = '\0';    /* terminate the field that has been extracted */
    if(next) *next = i; /* point to the start of the next field */

    return 0;    /* no errors */
}

#ifndef LT_NO_BLOCK

/*===========================================================================
 * Function    : ltGetSeqNum
 * Purpose     : gets the next sequence number
 * Globals  in :
 *         out : 
 * Errors      :
 * Used by     :
 * Description : 255 + 1 gives 1
 *===========================================================================*/

unsigned char
ltGetSeqNum(short session)
{
    return ltSessions[session].seqNum = (ltSessions[session].seqNum == 255)
                                      ? 1
                                      : ltSessions[session].seqNum + 1;
}

#endif /* LT_NO_BLOCK */

/*===========================================================================
 * Function    : ltPrepareHeader
 * Purpose     : prepares the header for a message and writes it to buffer
 * Globals  in :
 *         out : 
 * Errors      : 
 * Used by     : ltSendMessage
 *               ltGetImage
 * Description :
 *===========================================================================*/

/* all messages start with:
             4 bytes containing the length of the message (including these
                         four bytes); note that this is binary numeric
             1 byte  containing the sequence number of the message (this is
                         included in the length as well)
   the #defines for these follow */

#define LT_MSG_LEN_BYTES  (sizeof(LT_POS))
#define LT_SEQ_NUM_BYTES  (sizeof(unsigned char))

LT_POS                                /* buffer offset for rest of message */
ltPrepareHeader(short  session,       /* session */
                unsigned char seqNum, /* sequence number to be given to
                                         this message */
                char  *field,         /* field (possibly NULL) to include */
                LT_POS length)        /* message length not including header
                                         or field */
{
    LT_POS offset = LT_MSG_LEN_BYTES + LT_SEQ_NUM_BYTES;

    ltSessions[session].buffer[LT_MSG_LEN_BYTES] = seqNum;
    if(field)
    {
        strcpy((char *)ltSessions[session].buffer + offset, field);
        offset += strlen(field);
    }
    *((LT_POS*)ltSessions[session].buffer) = htonl(offset + length);

    return offset;
}

/*===========================================================================
 * Function    : ltSendMessage
 * Purpose     : constructs and sends a message; if transType is null then
 *               entire message is assumed to be in field
 * Globals  in :
 *         out : 
 * Errors      : from ltWrite
 * Used by     :
 * Description :
 *===========================================================================*/

short                               /* status */
ltSendMessage(short session,        /* session to write to */
              unsigned char seqNum, /* sequence number to be given to
                                       this message */
              char *transType,      /* transaction code - can be null */
              char *field)          /* field */
{
    LT_POS   msgLength;
    LT_POS   offset;
    int      connection;
#ifdef LT_NO_BLOCK
    short    status;
    unsigned seconds;
#endif

    if(session < 0 || session >= LT_MAX_NUM_OF_SESSIONS)
        return LT_ERR_TOO_MANY_SESSIONS;
    connection = ltSessions[session].connection;

    msgLength = strlen(field);
    if(transType) msgLength += 1 + strlen(transType);
    offset = ltPrepareHeader(session, seqNum, NULL, msgLength);
    msgLength += offset;
    if(msgLength >= LT_MAX_BUFFER_SIZE) return LT_ERR_FIELD_TOO_LONG;

    if(transType)
        sprintf((char *)ltSessions[session].buffer + offset,
                "%s%c%s", transType, LT_FIELD_SEPARATOR, field);
    else
        strcpy((char *)ltSessions[session].buffer + offset, field);
#ifdef LT_LOG_FILE
    ltWrite2Log("Send:", (char *)ltSessions[session].buffer + offset);
#endif

#ifdef LT_NO_BLOCK
    seconds = msgLength / 100;
    if(seconds < 100) seconds = 100;
    ltSetAlarm(LT_WHERE_ALARM_WRITE, seconds);
    status = ltWrite(
#ifdef LT_ENCRYPT
                     session,
#endif
                             connection, msgLength, (char *)ltSessions[session].buffer);
    ltClearAlarm();
    return status;
#else
    return ltWrite(
#ifdef LT_ENCRYPT
                   session,
#endif
                            connection, msgLength, (char *)ltSessions[session].buffer);
#endif
}

/*===========================================================================
 * Function    : ltReceiveMessage
 * Purpose     : reads a message into buffer and extracts its transaction
 *               code and sequence number
 * Globals  in :
 *         out : 
 * Errors      : from ltReadBuf
 *               from ltGetField
 *               LT_ERR_BAD_MESSAGE_LENGTH
 * Used by     :
 * Description : the function ltReadBuf is called from only this function;
 *               it is a macro that is mapped to ltRead for client code,
 *               and to ltReadWithCheck for server code
 *===========================================================================*/

short                                   /* status */
ltReceiveMessage(short   session,       /* session to read from */
                 unsigned char *seqNum, /* sequence number read from message */
                 char   *transType,     /* transaction code read from message */
                 LT_POS *next,          /* start of next field in buffer */
                 LT_POS *length,        /* number of bytes in buffer */
                 LT_POS *more)          /* number of bytes of overflow */
{
    LT_POS netLength;
    short  status = 0;
    short  retval = 0;
    int    connection = ltSessions[session].connection;
#ifdef LT_LOG_FILE
    short  i;
    char   saveByte;
#endif

    *seqNum = 0;
    *more = 0;
    status = ltReadBuf(
#ifdef LT_ENCRYPT
                       session,
#endif
                                connection, (LT_POS)LT_MSG_LEN_BYTES,
                       (char *) &netLength);
    if(status != 0) return status;

    *length = ntohl(netLength);
    *length -= LT_MSG_LEN_BYTES;    /* length includes the length bytes */
    if(*length < 2) return LT_ERR_BAD_MESSAGE_LENGTH;

    if(*length > LT_MAX_BUFFER_SIZE)
    {/* message will not fit into buffer */
        *more = *length - LT_MAX_BUFFER_SIZE;
        *length = LT_MAX_BUFFER_SIZE;
    }
    ltSessions[session].buffer[*length] = 0;
    status = ltReadBuf(
#ifdef LT_ENCRYPT
                       session,
#endif
                               connection, *length, (char *)ltSessions[session].buffer);
    if(status != 0) return status;

    *seqNum = ltSessions[session].buffer[0];
#ifdef LT_LOG_FILE
    /* do not print any non-printing characters (e.g. in an image);
       print at most approx 80 characters anyway */
    for(i = LT_SEQ_NUM_BYTES; i < 80; i++)
        if(!isprint(ltSessions[session].buffer[i])) break;
    saveByte = ltSessions[session].buffer[i];
    ltSessions[session].buffer[i] = 0;  /* null terminate */
    ltWrite2Log("Receive:", (char *)ltSessions[session].buffer + LT_SEQ_NUM_BYTES);
    ltSessions[session].buffer[i] = saveByte;  /* restore byte */
#endif
    retval =  ltGetField(ltSessions[session].buffer, (LT_POS)LT_SEQ_NUM_BYTES,
                         *length, transType, LT_MAX_TRANS_CODE_LENGTH, next);

    return retval;
}

/*===========================================================================
 * Function    : ltSendInfo
 * Purpose     : sends an informative message and expects no response
 * Globals  in :
 *         out :
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

short
ltSendInfo(short session, char *text)
{
    short status;

    if((status = ltSendMessage(session, 0, LT_TRANS_INFO, text)) != 0)
    {
#ifdef LT_LOG_FILE
        ltWriteLog("Error sending Informative Text");
#endif
        return status;
    }
    return 0;
}
