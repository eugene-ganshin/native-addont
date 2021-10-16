/*===========================================================================
 * File     : ltoapi.h
 * Release  : 1.02
 * Purpose  : the public interface to the LTO API
 * Contents :
 *
 * Copyright (c) Land Titles Office 1994-9,2000-4
 *===========================================================================*/

/* Function declarations */
#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

short ltInit(void);
short ltOpen(char *clientCode,
#ifdef LT_ENCRYPT
                               unsigned
#endif
                                        char *key, char *host, short port,
             short *session);
short ltQuery(short session, char *request, long length, char *response,
             long *actualLength, short *getMore, short *badField, long *charge);
short ltGetMore(short session, long length, char *response, long *actualLength,
             short *getMore);
short ltReset(short session);
short ltClose(short session);
short ltEnd(void);



/* Variables */
#ifndef LT_NO_BLOCK
extern char ltAPIVersion[];
#endif

/* Status codes - Errors */

#define LT_ERR_API_NOT_INIT           -1
#define LT_ERR_OUT_OF_MEMORY          -2
#define LT_ERR_BAD_QUERY              -3
#define LT_ERR_BAD_FORMAT             -4
#define LT_ERR_BAD_MESSAGE_LENGTH     -6
#define LT_ERR_FIELD_TOO_LONG         -7
#define LT_ERR_BAD_METHOD             -8
#define LT_ERR_TOO_MANY_SESSIONS     -10
#define LT_ERR_BAD_OPEN              -12
#define LT_ERR_UNKNOWN_TRANS         -14
#define LT_ERR_BAD_SEQUENCE          -15
#define LT_ERR_ON_RECEIVE            -16
#define LT_ERR_ON_SEND               -17
#define LT_ERR_UNKNOWN_HOST          -19
#define LT_ERR_ON_CONNECT            -20
#define LT_ERR_ON_BIND               -21
#define LT_ERR_ON_ACCEPT             -23
#define LT_ERR_SERVER_SHUTDOWN       -24
#define LT_ERR_RESET                 -26
#define LT_ERR_BAD_RESET             -27
#define LT_ERR_BUFFER_TOO_SMALL      -32
#define LT_ERR_USE_GET_MORE          -33
#define LT_ERR_BAD_GET_MORE          -34
#define LT_ERR_WINSOCK_DLL_UNAVAIL   -35


/* Status codes - Warnings */

/* Severe                  */
#define LT_WRN_DATA_EXCEPTION             1
#define LT_WRN_NO_SEARCH_RESULT           2

/* System Level Conditions */
#define LT_WRN_ITS_CLOSED                10
#define LT_WRN_SERVICE_UNAVAILABLE       11
#define LT_WRN_PRINTERS_UNAVAILABLE      12
#define LT_WRN_NO_PRINTER_DEFN           13

/* Data Input Errors       */
#define LT_WRN_REF_NOT_ON_DB             20
#define LT_WRN_BAD_FIELD                 21
#define LT_WRN_EXCEPTION_SEARCH          22
#define LT_WRN_BAD_XML                   24
#define LT_WRN_BAD_XML_TRANS             25

/* Images                  */
#define LT_WRN_NO_IMAGES                 30
#define LT_WRN_TOO_MANY_PAGES            31
#define LT_WRN_PAGE_OUT_OF_RANGE         31
#define LT_WRN_IMAGE_PRINT_ONLY          32
#define LT_WRN_IMAGE_UNUSED_NUMBER       33
#define LT_WRN_IMAGE_NOT_RECEIVED        34
#define LT_WRN_IMAGE_WTH_REJ_CAN_UNC     35
#define LT_WRN_IMAGE_SYSTEM_UNAVAIL      36
#define LT_WRN_IMAGE_SYSTEM_ERROR        37

/* Documents               */
#define LT_WRN_IMAGE_MISSING_SUB_TYPE    38

/* Plans                   */
#define LT_WRN_PLAN_SUPERSEDED           40
#define LT_WRN_PLAN_HISTORICAL           41

/* Documents               */
#define LT_WRN_IMAGE_SUPERSEDED          40
#define LT_WRN_IMAGE_NO_RECORD           42
#define LT_WRN_CROWN_SEE_PLAN_REF        43
#define LT_WRN_IMAGE_CONFIDENTIAL        44
#define LT_WRN_SCIMS_NOT_AVAILABLE       45
#define LT_WRN_IMAGE_AMEND_PENDING       46

/* Dealings                */
#define LT_WRN_DEAL_DELAYED              50
#define LT_WRN_DEAL_DE_REGISTERED        51

/* Documents               */
#define LT_WRN_IMAGE_UNREGISTERED        52
#define LT_WRN_IMAGE_WAL_CREATION        53

/* CRR                     */
#define LT_WRN_CRR_SYSTEM_UNAVAIL        60
#define LT_WRN_CRR_SYSTEM_ERROR          61
#define LT_WRN_CRR_BAD_PARISH_COUNTY     62
#define LT_WRN_CRR_CLEARANCES_UNAVAIL    63

/* Street Address          */
#define LT_WRN_ADDR_SYSTEM_UNAVAIL       70
#define LT_WRN_ADDR_SYSTEM_ERROR         71
#define LT_WRN_ADDR_STREET_OR_PROP       72
#define LT_WRN_ADDR_TOO_MANY_SUBUNITS    73
#define LT_WRN_ADDR_TOO_MANY_STR_NUMS    74
#define LT_WRN_ADDR_SPECIFY_MORE         75
#define LT_WRN_ADDR_TOO_MANY_FOL_REFS    76

/* Name Search          */
/* Current owner name   */
#define LT_WRN_SYSTEM_UNAVAIL            80
#define LT_WRN_SYSTEM_ERROR              81
#define LT_WRN_EXCESSIVE_DATA            82
#define LT_WRN_TIME_OUT                  83

/* Transaction server          */
#define LT_WRN_TRANS_SYSTEM_UNAVAIL      90
#define LT_WRN_TRANS_SYSTEM_ERROR        91
#define LT_WRN_TRANS_TIMEOUT             99

/* Others                  */
#define LT_WRN_BUFFER_MORE              100
#define LT_WRN_PROVISIONAL_CHARGE       101

/* Sydney Water   */
#define LT_WRN_WATER_SYSTEM_UNAVAIL           110
#define LT_WRN_WATER_SYSTEM_ERROR             111
#define LT_WRN_WATER_VALIDATION               112
#define LT_WRN_WATER_BAD_APP_NUM              113
#define LT_WRN_WATER_MANUAL_DELIVERY          114
#define LT_WRN_WATER_DOC_EXPIRED              115
#define LT_WRN_WATER_DOC_NOT_READY            116
#define LT_WRN_WATER_DOC_UNAVAIL              117
#define LT_WRN_WATER_DOC_ERROR                118
#define LT_WRN_WATER_DOC_NOT_REQUESTED        119
#define LT_WRN_WATER_PROPERTY_NOT_IDENTIFIED  120
#define LT_WRN_WATER_MORE_THAN_ONE_PROPERTY   121

/* Office of State Revenue */
#define LT_WRN_OSR_SYSTEM_UNAVAIL             130
#define LT_WRN_OSR_SYSTEM_ERROR               131
#define LT_WRN_OSR_BAD_XML                    132
#define LT_WRN_OSR_BAD_ENQ_ID                 133

/* Certificate Authentication Code Inquiry */
#define LT_WRN_CAC_DETAILS_DONT_MATCH         140
#define LT_WRN_CAC_NO_MATCH                   141
#define LT_WRN_CAC_TOO_MANY_ATTEMPTS          142

/* Bathurst: CRE and Ref Map */
#define LT_WRN_CADASTRAL_SYSTEM_UNAVAIL       150
#define LT_WRN_CADASTRAL_SYSTEM_ERROR         151
#define LT_WRN_CADASTRAL_DOC_UNAVAIL          152

#ifdef __cplusplus
}
#endif