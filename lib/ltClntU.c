/*============================================================================
 * File     : ltClntU.c
 * Release  : 1.02
 *
 * Purpose  : Client Transport, Upper.
 *
 * Contents : ltCheckClient
 *            ltGetMore
 *            ltQueryResponse
 *            ltQuery
 *            ltClose
 *            ltInit
 *            ltEnd
 *            ltOpen
 *            ltReset
 *
 * Copyright (c) Land Titles Office 1994,1995,1996,1997,1998,1999
 *============================================================================*/

#include <stdio.h>

#ifdef VAXC

#include <types.h>
#include <socket.h>
#include <in.h>
#include <inet.h>
#include <ucx$inetdef.h>
#include <netdb.h>

#elif defined(_WIN32)

#include <winsock2.h>

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#endif

#include <string.h>
#include <stdlib.h>

#ifdef LT_RESET
#include <setjmp.h>
#endif

#include "ltint.h"
#include "ltoapi.h"

static bool   ltIsInitialised = FALSE;   /* whether API has been initialised */
char          ltAPIVersion[10] = "1.02";
char          ltClientCode[LT_MAX_CLIENT_CODE_LENGTH + 1] = "";

/*===========================================================================
 * Function    : ltCheckClient
 * Purpose     : checks the LTO API's version and client code with the server
 * Globals  in : ltSessions
 *         out :
 * Errors      :
 * Used by     : ltOpen
 *===========================================================================*/

static short
ltCheckClient(short session)
{
    char      recvBuff[LT_MAX_PACKET+1];
    char      sendBuff[LT_MAX_PACKET+1];
    char      tranType[LT_MAX_TRANS_CODE_LENGTH+1];
#ifdef LT_LOG_FILE
    char      errMsg[LT_ERR_MESS_LEN+1];
#endif
    LT_POS    next=0;
    LT_POS    recvBuffLen=0;
    LT_POS    more=0;
    unsigned  char RseqNo=0;
    unsigned  char SseqNo=0;

    sprintf(sendBuff,"%s%c%s", ltAPIVersion, LT_FIELD_SEPARATOR,
                               ltSessions[session].clientCode);

    LT_CHKSTAT(ltSendMessage(session, SseqNo = ltGetSeqNum(session),
                             LT_TRANS_CLIENT_VERS, sendBuff),
               "Error sending version data to server")

    LT_CHKSTAT(ltReceiveMessage(session, &RseqNo, tranType, &next,
                                &recvBuffLen, &more),
               "Error receiving data from server")

    for(;;)
    {
        char *buffer = (char *)ltSessions[session].buffer;

        if(strcmp(tranType, LT_TRANS_INFO) == 0)
        {
            LT_CHKSTAT(ltGetField((unsigned char*)buffer, next, recvBuffLen, recvBuff, 50,
                                   &next),
                       "Error getting field from 'LT_TRANS_INFO'")

#ifdef LT_LOG_FILE
            sprintf(errMsg, "Information message (%s)", recvBuff);
            ltWriteLog(errMsg);
#endif

            LT_CHKSTAT(ltReceiveMessage(session, &RseqNo, tranType, &next,
                                        &recvBuffLen, &more),
                       "Error receiving data from server")

            continue;
        }
        else if(strcmp(tranType, LT_TRANS_DISCONNECT) == 0 )
        {
            LT_CHKSTAT(ltGetField((unsigned char*)buffer, next, recvBuffLen, recvBuff, 75,
                                   &next),
                       "Error getting field from 'LT_TRANS_DISCONNECT'")

#ifdef LT_LOG_FILE
            sprintf(errMsg,"Client told to shutdown because of (%s)",recvBuff);
            ltWriteLog(errMsg);
#endif
            ltDisconnectSession(session);
            ltSessions[session].active = FALSE;
            return LT_ERR_SERVER_SHUTDOWN;
        }
        else if(strcmp(tranType, LT_TRANS_ACCEPT) == 0 )
        {
            if(RseqNo != SseqNo)
            {
#ifdef LT_LOG_FILE
                sprintf(errMsg,
                    "Out of sequence, send Seqno = %d, recv Seqno = %d",
                    SseqNo, RseqNo);
                ltWriteLog(errMsg);
#endif
                return LT_ERR_BAD_SEQUENCE;
            }
#ifdef LT_LOG_FILE
            ltWriteLog("Version and Client OK");
#endif
            return 0;
        }
        else
        {
#ifdef LT_LOG_FILE
            ltWriteLog("Unknown Transaction Type");
#endif
            return LT_ERR_UNKNOWN_TRANS;
        }
    }
}
 
/*===========================================================================
 * Function    : ltGetMore
 * Purpose     : reads more of the query response into a buffer
 * Globals  in :
 *         out : ltSessions
 * Errors      :
 * Used by     :
 * Description : 
 *==========================================================================*/

short
ltGetMore(short session, long length, char *response, long *actualLength,
          bool *getMore)
{
    short stat;
    LT_POS more;

    if(!ltIsInitialised) return LT_ERR_API_NOT_INIT;
    if(!ltSessions[session].bufferMore) return LT_ERR_BAD_GET_MORE;

    /* check if response buffer is big enough */
    if(length < ltSessions[session].bufferMoreLen)
    {/* not big enough */
        *getMore = TRUE;
        ltSessions[session].bufferMore = TRUE;
        ltSessions[session].bufferMoreLen -= length;
        more = length;
    }
    else
    {/* big enough */
        *getMore = FALSE;
        ltSessions[session].bufferMore = FALSE;
        more = ltSessions[session].bufferMoreLen;
    }
    stat = ltRead(
#ifdef LT_ENCRYPT
                  session,
#endif
                           ltSessions[session].connection, more, response);
    if(stat != 0) return stat;
    *actualLength = more;
    length -= more;

    if(length > 0)
        response[more] = 0; /* null-terminate the string if room */
    return 0;
}

/*===========================================================================
 * Function    : ltQueryResponse
 * Purpose     : extracts badField and response from a message from the
 *               server
 * Globals  in :
 *         out :
 * Errors      :
 * Used by     :
 * Description : note that response is always null terminated if there is
 *               room in the response buffer; this is not included in
 *               actualLength
 *==========================================================================*/

static short
ltQueryResponse(short  session,    /* session - in case another read is
                                      required */
                LT_POS *next,      /* the start position in buffer of
                                      the next field */
                LT_POS length,     /* the length of buffer as returned by
                                      ltReceiveMessage */
                LT_POS more,       /* the overflow of buffer as returned by
                                      ltReceiveMessage */
                long  *charge,     /* charge for the query */
                short *badField,   /* badField returned from remoteSearch */
                LT_POS lengthResponse, /* size of response buffer */
                char  *response,   /* data returned from remoteSearch */
                LT_POS *actualLength, /* length of data returned in response */
                bool  *getMore)    /* whether there is more to get */
{
    char  strStatus[6];
    short status=0;
    char  strCharge[15];
    char *buffer = (char *)ltSessions[session].buffer;

    *getMore = FALSE;

    LT_CHKSTAT(ltGetField((unsigned char*)buffer, *next, length, strCharge, 14, next),
               "Error getting charge field from message")
    *charge = atol(strCharge);
    LT_CHKSTAT(ltGetField((unsigned char*)buffer, *next, length, strStatus, 5, next),
               "Error getting status field from message")
    status = atoi(strStatus);
    if(status == LT_WRN_BAD_FIELD)
    {
        char  strBadField[30];

        LT_CHKSTAT(ltGetField((unsigned char*)buffer, *next, length, strBadField, 29, next),
                   "Error getting badField field from message")
        *badField = atoi(strBadField);
    }
    else
    {
        *badField = 0;
    }

    *actualLength = length - *next;
    if(lengthResponse < *actualLength) return LT_ERR_BUFFER_TOO_SMALL;
    /* since the big messages go from server to client, ltBuffer is
       deliberately set small (e.g. 100) so that the status fields can
       be read but not much else; the remainder of the message is read
       directly into the user-supplied buffer; this avoids reading the
       whole message into ltBuffer and then copying it into the user-
       supplied buffer; so if this test fails then the user-supplied
       buffer must be very small because it must be smaller than ltBuffer
       which is at most (e.g.) 100 */

    memcpy(response, ltSessions[session].buffer + *next, *actualLength);
    /* calculate space remaining in response buffer */
    lengthResponse -= length - *next;

    if(more > 0)
    {/* more to get */
        short stat;

        /* check if space left in response buffer is big enough */
        if(lengthResponse < more)
        {/* not big enough - so fill up the response buffer and indicate more*/
            *getMore = TRUE;
            ltSessions[session].bufferMore = TRUE;
            ltSessions[session].bufferMoreLen = more - lengthResponse;
            more = lengthResponse;
        }
        stat = ltRead(
#ifdef LT_ENCRYPT
                      session,
#endif
                               ltSessions[session].connection, more,
                      response + length - *next);
        if(stat != 0) return stat;
        *actualLength += more;
        lengthResponse -= more;

        if(lengthResponse > 0)
            response[length + more - *next] = 0; /* null-terminate the string */
    }
    else
    {/* no more */
        if(lengthResponse > 0)
            response[length - *next] = 0; /* null-terminate the string */
    }
    return status;
}

/*===========================================================================
 * Function    : ltQuery
 * Purpose     : Construct and send query request an wait for a response.
 * Globals  in : ltIsInitialised
 *         out : ltEnvIsSet
 *               ltEnv
 * Errors      :
 * Used by     :
 * Description : note that if ltReset is called asynchronously during ltQuery
 *               then ltQuery returns with LT_ERR_RESET
 *==========================================================================*/

short
ltQuery(short session, char *request, long lengthResponse, char *response,
        long *actualLength, bool *getMore, short *badField, long *charge)
{
    unsigned char   RseqNo=0;
    unsigned char   SseqNo=0;
#ifdef LT_LOG_FILE
    char            errMsg[LT_ERR_MESS_LEN+1];
#endif
    char            transType[LT_MAX_TRANS_CODE_LENGTH+1];
    LT_POS          next=0;
    LT_POS          length=0;
    LT_POS          more=0;
    short           status;

    *charge = 0L;
    *badField = 0;
    *response = 0;
    *actualLength = 0;
    *getMore = FALSE;

    if(!ltIsInitialised) return LT_ERR_API_NOT_INIT;
    if(ltSessions[session].bufferMore) return LT_ERR_USE_GET_MORE;

#ifdef LT_RESET
    if(setjmp(ltSessions[session].env))
    {/* come here after longjmp */
#ifdef LT_LOG_FILE
        ltWriteLog("ltReset");
#endif
        status = LT_ERR_RESET;
        ltClose(session);
        goto exit;
    }
    /* drop through for first call of setjmp */
    ltSessions[session].envIsSet = TRUE;
#endif

    /* logging uses ltClientCode so make sure it is current */
    strcpy(ltClientCode, ltSessions[session].clientCode);

    status = ltSendMessage(session, SseqNo = ltGetSeqNum(session),
                           NULL,    /* transaction code is in request */
                           request);
    if(status != 0)
    {
#ifdef LT_LOG_FILE
        sprintf(errMsg, "Error sending query request no: %d", SseqNo);
        ltWriteLog(errMsg);
#endif
        goto exit;
    }

    status = ltReceiveMessage(session, &RseqNo, transType, &next, &length,
                              &more);
    if(status != 0)
    {
#ifdef LT_LOG_FILE
        sprintf(errMsg, "Error receiving query request no: %d", SseqNo);
        ltWriteLog(errMsg);
#endif
        goto exit;
    }

    for(;;) /* loop if we get an informative message */
    {
        if(strcmp(transType, LT_TRANS_INFO) == 0)
        {
            char recvBuff[LT_MAX_PACKET+1];

            LT_CHKSTAT(ltGetField(ltSessions[session].buffer, next, length,
                                   recvBuff, 50, &next),
                       "Error getting field from info message")
#ifdef LT_LOG_FILE
            ltWrite2Log("ltQuery: Informative message is: ", recvBuff);
#endif
            LT_CHKSTAT(ltReceiveMessage(session, &RseqNo, transType, &next,
                                        &length, &more),
                       "Error receiving message from server")
            continue;
        }
        else if(strcmp(transType, LT_TRANS_QUERY_RESPONSE) == 0)
        {
            if(SseqNo != RseqNo)
            {
#ifdef LT_LOG_FILE
                sprintf(errMsg,
                    "Out of sequence, send Seqno = %d, recv Seqno = %d",
                    SseqNo, RseqNo);
                ltWriteLog(errMsg);
#endif
                status = LT_ERR_BAD_SEQUENCE;
                goto exit;
            }
            status = ltQueryResponse(session, &next, length, more, charge,
                                     badField, lengthResponse, response,
                                     actualLength, getMore);
            goto exit;
        }
        else if(strcmp(transType, LT_TRANS_DISCONNECT) == 0)
        {
#ifdef LT_LOG_FILE
            ltWrite2Log("Server shutdown: ", (char *)ltSessions[session].buffer + next);
#endif
            ltDisconnectSession(session);
            ltSessions[session].active = FALSE;
            status = LT_ERR_SERVER_SHUTDOWN; /* Shutdown */
            strcpy(response, (char *)ltSessions[session].buffer + next);
            *actualLength = length - next;
            goto exit;
        }
        else
        {
#ifdef LT_LOG_FILE
            ltWriteLog("Received unknown transaction type from server");
#endif
            ltClose(session);   /* initiate shutdown */
            status = LT_ERR_UNKNOWN_TRANS;
            goto exit;
        }
    }/* End For */

 exit:

#ifdef LT_RESET
    ltSessions[session].envIsSet = FALSE;
#endif

    return status;
}

/*===========================================================================
 * Function    : ltClose
 * Purpose     : closes a connection
 * Globals  in : ltIsInitialised
 *         out :
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

short
ltClose(short session)
{
    if(!ltIsInitialised) return LT_ERR_API_NOT_INIT;

    ltSendMessage(session, 0, LT_TRANS_DISCONNECT, "");
    ltDisconnectSession(session);
    ltSessions[session].active = FALSE;
    if(session != 0) free(ltSessions[session].buffer);
    return 0;
}

/*===========================================================================
 * Function    : ltInit
 * Purpose     : initialises LTO API
 * Globals  in :
 *         out : ltIsInitialised
 *               ltNSessions
 * Errors      : LT_ERR_WINSOCK_DLL_UNAVAIL
 * Used by     :
 * Description :
 *===========================================================================*/

short
ltInit(void)
{
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
  
    wVersionRequested = MAKEWORD(2, 0);
 
    err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0)
    {/* could not find a useable  WinSock DLL */
        return LT_ERR_WINSOCK_DLL_UNAVAIL;
    }
#endif

    ltIsInitialised = TRUE;
    ltNSessions = 0;

#ifdef LT_LOG_FILE
    ltOpenLog("client");
    ltWriteLog("starting client"
#ifdef LT_ENCRYPT
                               " (encrypted)"
#endif
                                             );
#endif

    return 0;
}

/*===========================================================================
 * Function    : ltEnd
 * Purpose     : terminates LTO API
 * Globals  in : 
 *         out : ltIsInitialised
 * Errors      :
 * Used by     :
 * Description :
 *===========================================================================*/

short
ltEnd(void)
{
    short i;

    if(!ltIsInitialised) return LT_ERR_API_NOT_INIT;

    for(i = 0; i < ltNSessions; i++)
        if(ltSessions[i].active)
            ltClose(i);

    ltIsInitialised = FALSE;

#ifdef LT_LOG_FILE
    ltWriteLog("closing client");
#endif

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

/*===========================================================================
 * Function    : ltOpen
 * Purpose     : opens a connection via accept & connect.
 * Globals  in : ltIsInitialised
 *         out :
 * Errors      :
 * Used by     :
 * Description : reclaims finished session numbers
 *===========================================================================*/

short
ltOpen(char *clientCode,
#ifdef LT_ENCRYPT
                         unsigned
#endif
                                  char *key, char *host, short port,
       short *session)
{
    short status = 0;
    short i;
    bool  found = FALSE;

    if(!ltIsInitialised) return LT_ERR_API_NOT_INIT;

    for(i = 0; i < ltNSessions; i++)
        if(!ltSessions[i].active)
        {/* found an inactive session */
            found = TRUE;
            break;
        }

    if(!found)
    {/* nothing found yet */
        if(ltNSessions >= LT_MAX_NUM_OF_SESSIONS)
            return LT_ERR_TOO_MANY_SESSIONS;
        i = ltNSessions++;
        ltSessions[i].active = FALSE;
    }

    strcpy(ltSessions[i].clientCode, clientCode);
    status = ltConnect(host, port, &ltSessions[i].connection);
    if(status != 0)
        return status;

    ltSessions[i].active = TRUE;
    ltSessions[i].seqNum = 255;
    ltSessions[i].bufferMore = FALSE;
    ltSessions[i].buffer = i == 0
                         ? ltBuffer
                         : (unsigned char *)malloc(LT_MAX_BUFFER_SIZE + 1);
    if(!ltSessions[i].buffer) return LT_ERR_OUT_OF_MEMORY;
    /* make sure the last byte is null */
    ltSessions[i].buffer[LT_MAX_BUFFER_SIZE] = 0;

#ifdef LT_ENCRYPT
    ltInitKey(&ltSessions[i].sendReg, key);
    ltInitKey(&ltSessions[i].recvReg, key);
#endif

#ifdef LT_RESET
    ltSessions[i].envIsSet = FALSE;
#endif
    *session = i;
    return ltCheckClient(*session);
}

/*===========================================================================
 * Function    : ltReset
 * Purpose     : resets ltQuery
 * Globals  in : ltEnvIsSet
 *               ltEnv
 *         out :
 * Errors      :
 * Used by     : user code during execution of ltQuery
 * Description : note that this function normally does not return
 *===========================================================================*/

#ifdef LT_RESET

short
ltReset(short session)
{
    if(ltSessions[session].envIsSet)   /* check if ltEnv has been set */
    {
        ltSessions[session].envIsSet = FALSE;
        longjmp(ltSessions[session].env, 1);  /* never returns */
    }
    else
        return LT_ERR_BAD_RESET;
}

#endif
