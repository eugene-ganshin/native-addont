/*===========================================================================
 * File     : ltint.h
 * Release  : 1.02
 * Purpose  : All private definitions for a client program.
 * Contents :
 *
 * Copyright (c) Land Titles Office 1994-99,2000-7
 *===========================================================================*/

#ifndef _WIN32

typedef int SOCKET;
#define INVALID_SOCKET -1

#endif

#define TRUE   1
#define FALSE  0

#ifndef LT_MAX_BUFFER_SIZE
#define LT_MAX_BUFFER_SIZE           10000
#endif

#define LT_MAX_PACKET                 1024
#define LT_MAX_NUM_OF_SESSIONS           4
#define LT_ERR_MESS_LEN                200

#define LT_MAX_CLIENT_CODE_LENGTH       20
#define LT_MAX_CLIENT_REF_SIZE          40

#define LT_FIELD_SEPARATOR              '|'

/* public transaction codes */
#define LT_TRANS_IMAGE_STATUS          "IS"   /* Image Status                */
#define LT_TRANS_IMAGE_REQUEST         "IR"   /* Request Image               */
#define LT_TRANS_DOC_STATUS            "DS"   /* Document Status             */
#define LT_TRANS_DOC_REQUEST           "DR"   /* Request Document            */
#define LT_TRANS_IMAGE_GET             "IG"   /* Get Image                   */
#define LT_TRANS_FILE_GET              "FG"   /* Get File                    */
#define LT_TRANS_WATER_REQUEST         "WR"   /* Request Sydney Water Docs   */
#define LT_TRANS_WATER_CC              "WCC"  /* Get Sydney Water Convey.Cert*/
#define LT_TRANS_WATER_SSD             "WSSD" /* Get Sydney Water SSD Doc    */
#define LT_TRANS_WATER_SLP             "WSLP" /* Get Sydney Water SLP Doc    */
#define LT_TRANS_CRE_REQUEST           "CRE"  /* Request CRE Map             */
#define LT_TRANS_CRE_GET               "CREG" /* Get CRE Map                 */
#define LT_TRANS_REF_MAP_REQUEST       "RMAP" /* Request Ref Map             */
#define LT_TRANS_REF_MAP_GET           "RMAPG"/* Get Ref Map                 */
#define LT_TRANS_XML                   "X"    /* XML transaction             */
#define LT_TRANS_XML_OSR_CLR           "OCLR" /* XML OSR Clearance           */
#define LT_TRANS_XML_OSR_VAL           "OVAL" /* XML OSR Valuation           */
#define LT_TRANS_XML_OSR_WFS           "OWFS" /* XML OSR Workflow Status     */
#define LT_TRANS_OWNER_PURCHASER       "OP"   /* Owner Purchaser Search      */
#define LT_TRANS_NOS_IMAGE             "NOSIR"/* NOS notice of sale image    */

/* private transaction codes */
#define LT_TRANS_CLIENT_VERS            "?"
#define LT_TRANS_INFO                   "!"
#define LT_TRANS_ACCEPT                 "="
#define LT_TRANS_DISCONNECT             "-"
#define LT_TRANS_QUERY_RESPONSE         "<"

#define LT_TRANS_TEST_TIMEOUT          "TT"

#define LT_MAX_TRANS_CODE_LENGTH         5

typedef short bool;

typedef long LT_POS;

/* structures */

#ifdef LT_ENCRYPT

typedef struct
{
    unsigned long r1;
    unsigned long r2;
    unsigned long r3;
} LT_ENC_REG;

#endif

typedef struct
{
    SOCKET   connection;           /* the socket for the session */
    unsigned char *buffer;         /* for receiving and assembling messages */
#ifdef LT_ENCRYPT
    LT_ENC_REG sendReg;            /* for encrypting data for sending */
    LT_ENC_REG recvReg;            /* for decrypting received data */
#endif
#ifndef LT_NO_BLOCK
    bool     active;               /* whether this session is in use */
    unsigned char seqNum;          /* current sequence number for session */
    char     clientCode[LT_MAX_CLIENT_CODE_LENGTH]; /* client code for session*/
    bool     bufferMore;           /* whether there is more response to get */
    LT_POS   bufferMoreLen;        /* how much more response there is */
#ifdef LT_RESET
    bool     envIsSet;             /* whether field env has a valid value */
    jmp_buf  env;                  /* for long jumps */
#endif
#endif
} LT_SESSION;

/* macros */

/* evaluates an expression, checks its status and if error it logs a message
   and returns the status; use s228 so that this variable name does not clash
   with any variable used in expression */

#ifdef LT_LOG_FILE
#define LT_CHKSTAT(expression, message)                  \
    {                                                    \
        short s228 = (expression);                       \
                                                         \
        if (s228 != 0)                                   \
        {                                                \
            char errMsg[90];                             \
                                                         \
            sprintf(errMsg, "%s: %hd", (message), s228); \
            ltWriteLog(errMsg);                          \
            return s228;                                 \
        }                                                \
    }
#else
#define LT_CHKSTAT(expression, message)                  \
    {                                                    \
        short s228 = (expression);                       \
                                                         \
        if (s228 != 0)                                   \
            return s228;                                 \
    }
#endif

/* variables */

extern unsigned char ltBuffer[];
extern char ltClientCode[];
extern short ltNSessions;
extern LT_SESSION ltSessions[];

/* functions */

short ltConnect(char *host, short port, SOCKET *connection);
short ltDisconnectSession(short session);
short ltWrite(
#ifdef LT_ENCRYPT
              short session,
#endif
                             SOCKET connection, long nBuffer, char *buffer);
short ltRead(
#ifdef LT_ENCRYPT
             short session,
#endif
                            SOCKET connection, long nBuffer, char *buffer);
#ifdef LT_ENCRYPT
void ltInitKey(LT_ENC_REG *c, unsigned char *key);
#endif

void ltOpenLog(char *fileName);
void ltWriteLog(char *text);
void ltWrite2Log(char *text, char *text2);
void ltWriteErrnoLog(char *text);
#ifdef LT_NO_BLOCK
#define ltReadBuf ltReadWithCheck
#else
#define ltReadBuf ltRead
unsigned char ltGetSeqNum(short session);
#endif
short ltGetField(unsigned char *buf, LT_POS start, LT_POS length, char *field,
                  short maxFieldLen, LT_POS *next);
short ltSendMessage(short session, unsigned char seqNum, char *transType,
                  char *field); 
short ltReceiveMessage(short session, unsigned char *seqNum, char *transType,
                  LT_POS *next, LT_POS *length, LT_POS *more);
