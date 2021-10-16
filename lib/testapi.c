/*===========================================================================
 * File     : testapi.c
 * Release  : 1.02
 *
 * Purpose  : To provide a typical, but basic client LTO API application.
 *            Mainly for testing purposes.
 *
 * Contents : SigHandler
 *            ReadLine
 *            ShowSearch
 *            main
 *
 * Copyright (c) Land Titles Office 1994-99,2001-2,2005
 *==========================================================================*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef LT_RESET
#include <unistd.h>
#endif
#include <signal.h>
#include "ltoapi.h"

#ifdef _WIN32
#include <ctype.h>
#include <io.h>
#else
#define O_BINARY   0    /* needed by NT (where it has a non-zero value) */
#endif

#ifdef LT_RESET

/*===========================================================================
 * Function    : sigHandler
 * Purpose     : handles the alarm signal and calls ltReset so that ltQuery
 *               will terminate
 *===========================================================================*/

static void
SigHandler(void)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
    ltReset(0);  /* testapi runs with only one session so hard-code it */
}

#endif

/*===========================================================================
 * Function    : ReadLine
 * Purpose     : reads a line of input; a backslash as the last non-space
 *               will allow a continuation line
 *===========================================================================*/

static void
ReadLine(char *prompt, char *line)
{
    short i = 0;
    short endOfLastLine = 0;
    char  c;

    do
    {
        printf("%s", prompt);                  /* prompt the user for input */
        while((c = getchar()) != '\n') line[i++] = c; /* read line of input */
        while(i > endOfLastLine && line[i - 1] == ' ')
            i--;                                   /* strip trailing blanks */
        line[i] = 0;                             /* null-terminate the line */
        i--;                        /* in case there is a continuation line */
        endOfLastLine = i;
        prompt = "=";                                /* continuation prompt */
    } while(i >= 0 && line[i] == '\\');
}

/*===========================================================================
 * Function    : GetTransactionType
 * Purpose     : reads a line of input and extracts the transaction type
 *               out of the line
 *===========================================================================*/

static short
GetTransactionType(char *line, char *ttype)
{
    short i = 0;
    short j = 0;
    short status = 0;


	if (*line == 0) return status;

    for (i=0; i < (short) strlen(line); i++)
    {
		if (line[i] == '|') /* stop at the vertical bar bar */
		{ /* transaction type found */
			status = 1;
			break;
		}
	    else
		   ttype[j++] = line[i];
    }
    ttype[j]=0;
	return status;
}


/*===========================================================================
 * Function    : ShowSearch
 * Purpose     : displays a search on the screen in a simplistic manner
 * Returns     : whether search should be written to a file
 *===========================================================================*/

#define INITIAL_ROWS_ON_SCREEN     18
#define ROWS_ON_SCREEN             23

#define STATE_START_LINE            1
#define STATE_NEW_LINE              2
#define STATE_LINE_NUMBER           3
#define STATE_TEXT                  4

static short
ShowSearch(char *transnType, long length, char *s, short getMore, long *imageOffset, short status)
{
    char   c;
    short  i;
    short  rowsOnScreen = INITIAL_ROWS_ON_SCREEN;
    char   trans[100];
    char   folTypeIn[100];
    char   folIn[100];
    char   folTypeOut[100];
    char   folOut[100];
    char   nLines[100];
    char   pages[100];
    char   size[100];
    char   kBytes[100];
    char   requestId[100];
    char   parish[100];
    char   county[100];
    char   lga[100];
    char   appNum[100];
    int    nAuths;
    char   listItem[100];
    char   docType[100];
    char   docId[100];
    int    nSubTypes;
    int    nChars;
    short  initLen;
    short  digitsSeen;
    short  state;
    char   transId[100];
    char   iRef[100];
    int    nCols;
    int    nRows;
    int    ii=0;
    char   name[1000];
    char   titles[1000];

    *imageOffset = 0;

    c = s[0];
    if(c == 0) return 0;
    if(s[1] != '|')
    {
        printf("%s\n", s);
        return 0;
    }
    s += 2;  /* skip over "x|" */
anotherResponse:
    switch(c)
    {
    /**********************
     *      TITLES        *
     **********************/
    case 'S': /* normal search result */
        sscanf(s, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]| %[^|]|%n",
               trans, folTypeIn, folIn, folTypeOut, folOut, nLines, &nChars);
        printf("Search: trans %s fol type in %s fol in %s\n"
               "        fol type out %s fol out %s lines %s\n",
               trans, folTypeIn, folIn, folTypeOut, folOut, nLines);
        break;
    /**********************
     *      MESSAGES      *
     **********************/
    case 'U': /* unformatted single-line information message */
        printf("Info message: %s\n", s);
        return 0;
    case 'M': /* formatted information message */
        sscanf(s, "%[^|]|%n", nLines, &nChars);
        printf("Info message: lines %s\n", nLines);
        break;
    case 'T': /* formatted certificate */
        sscanf(s, "%[^|]|%n", nLines, &nChars);
        printf("Certificate: lines %s\n", nLines);
        break;
    /**********************
     *      IMAGES        *
     **********************/
    case 'P': /* image size (from IS)*/
        sscanf(s, "%[^|]|%[^|]|%s", pages, size, kBytes);
        printf("Pages %s size %s KBytes %s\n", pages, size, kBytes);
        return 0;
    case 'E': /* superseded plan (from IS) */
        printf("Plan has been superseded by %s\n", s);
        return 0;
    case 'Q': /* document subtypes and sizes (from DS) */
        sscanf(s, "%d%n", &nSubTypes, &nChars);
        s += nChars;
        for(; nSubTypes > 0; nSubTypes--)
        {
            char   subType[100];
            char   docStatus[100];
            char   docStsMsg[500];
            char   dateTime[100];

            s++; /* skip '|' */
            if(*s != '|')
            {
                sscanf(s, "%[^|]%n", subType, &nChars);
                s += nChars;
            }
            else
                subType[0] = 0;
            s++;
            sscanf(s, "%[^|]|%n", docStatus, &nChars);
            s += nChars;
            if(*s != '|')
            {
                sscanf(s, "%[^|]%n", docStsMsg, &nChars);
                s += nChars;
            }
            else
                docStsMsg[0] = 0;
            s++;
            if(*s != '|')
            {
                sscanf(s, "%[^|]%n", pages, &nChars);
                s += nChars;
            }
            else
                pages[0] = 0;
            s++;
            if(*s != '|')
            {
                sscanf(s, "%[^|]%n", size, &nChars);
                s += nChars;
            }
            else
                size[0] = 0;
            s++;
            if(*s != '|')
            {
                sscanf(s, "%[^|]%n", kBytes, &nChars);
                s += nChars;
            }
            else
                kBytes[0] = 0;
            s++;
            if(*s != '|' && *s != 0) /* allow for end of last subtype record */
            {
                sscanf(s, "%[^|]%n", dateTime, &nChars);
                s += nChars;
            }
            else
                dateTime[0] = 0;
            printf("Subtype %s: status %s [%s]\n",
                   subType, docStatus, docStsMsg);
            printf("      %s pages; size %s; %s KBytes; scanned at",
                   pages, size, kBytes);
            if(dateTime[0])
                printf(" %.2s:%.2s %.2s/%.2s/%.4s\n",
                   dateTime + 8, dateTime + 10, dateTime + 6, dateTime + 4,
                   dateTime);
            else
                printf("\n");
        }
        return 0;
    case 'F': /* superseded document (from DS) */
        sscanf(s, "%[^|]|%s", docType, docId);
        printf("Document has been superseded by doc type %s number %s\n",
               docType, docId);
        return 0;
    case 'R': /* image request id */
        if (strcmp(transnType,"LVR") == 0)
          printf("Activaction Key %s\n", s);			
        else
          printf("Request id %s\n", s);
        return 0;
    case 'D': /* (image) data */
        sscanf(s, "%[^|]|%n", requestId, &nChars);
        printf("Data with identifier of \"%s\":\n",
               requestId);
        s += nChars;
        *imageOffset = 2 + nChars;
        length -= 2 + nChars;
        if( isprint(s[0])
         && (length == 1 || isprint(s[1])
                         && (length == 2 || isprint(s[2])
                                         && (length == 3 || isprint(s[3])))))
        {/* data appears to be text */
            initLen = 60;
            for(i = 0; i < initLen && i < length; i++)
                putchar(isprint(s[i]) ? s[i] : '.');
        }
        else
        {/* binary data */
            initLen = 30;
            for(i = 0; i < initLen && i < length; i++)
                printf("%.02x", (unsigned int)s[i]);
        }
        printf("%s (%ld%s bytes)\n", length > initLen ? " ..." : "",
                                     length, getMore ? "+" : "");
        return 0;
    /**********************
     *      CRR           *
     **********************/
    case 'N': /* proprietor data */
        sscanf(s, "%[^|]|%[^|]|%[^|]|%[^|]|%n",
               parish, county, lga, nLines, &nChars);
        printf("Parish %s county %s LGA %s lines %s\n",
               parish, county, lga, nLines);
        break;
    case 'C': /* CRR restrictions */
        /* new format */
        if (strcmp(transnType,"CRR") == 0)
        {
            sscanf(s, "%d%n", &nAuths, &nChars);
            printf("Number of Authorities %d\n", nAuths);
        }
        else
        {
            sscanf(s, "%[^|]|%d%n", appNum, &nAuths, &nChars);
            printf("Application number %s\n", appNum);
        }

        for(; nAuths > 0; nAuths--)
        {
            char authCode[100];
            char authStatus[100];
            char authFee[100];
            char authGstFee[100];
            int  fee;
            int  gst;

            s += nChars;
            sscanf(s, "|%[^|]|%[^|]|%[^|]|%[^|]%n",
                   authCode, authStatus, authFee, authGstFee, &nChars);
            fee = atoi(authFee);
            gst = atoi(authGstFee);
            printf("Authority code %s status %s"
                   " Total fee $%d.%.02d gst $%d.%.02d\n",
                   authCode, authStatus,
                   fee / 100, fee % 100, gst / 100, gst % 100);
        }
        /* see if C (from CRR) is followed by certificate */
        if (strcmp(transnType,"CRR") == 0)
        { /* certificate is a multiline format so set c = 'M' */
            c = 'M';
            s += nChars + 1;
            goto anotherResponse;
        }

        /* see if C (from WR) is followed by M */
        s += nChars;
        if(strncmp(s, "|M|", 3) == 0)
        {
            c = 'M';
            s += 3;  /* skip over "|M|" */
            goto anotherResponse;
        }
        return 0;
    /**********************
     *      OTHER         *
     **********************/

    case 'L': /* list */
        printf("List Format: [%s]\n", transnType);
        if ((strcmp(transnType,"IPIVF") == 0) ||
            (strcmp(transnType, "GPR") == 0) ||
            (strcmp(transnType, "LVA") == 0) ||
            (strcmp(transnType, "LVAF") == 0) ||
            (strcmp(transnType, "LV") == 0) ||
            (strcmp(transnType, "LVF") == 0) ||
            (strcmp(transnType, "RSF") == 0) ||
            (strcmp(transnType, "RLC") == 0) ||
            (strcmp(transnType, "DOC") == 0) ||
            (strcmp(transnType, "PLAN") == 0) ||
            (strcmp(transnType, "SIGAN") == 0) ||
            (strcmp(transnType, "SIGAS") == 0) ||
            (strcmp(transnType, "CWON") == 0) ||
            (strcmp(transnType, "CWOS") == 0) ||
            (strcmp(transnType, "GENN") == 0) ||
            (strcmp(transnType, "GENS") == 0) ||
            (strcmp(transnType, "DEED") == 0) ||
            (strcmp(transnType, "RMAP") == 0) ||
            (strcmp(transnType, "LFX") == 0 && status == 23))
        {
          nCols = 1; /* always one column */
          sscanf(s, "%d%n",&nRows,&nChars);
          printf("nRows %d\n",nRows);
        } 
        else if (strcmp(transnType,"LFX") == 0)
        { /* Lease Folio Search */
          nCols = 1; /* always one column */

          /* complex name */
          if(*s != '|')
          {
             sscanf(s, "%[^|]%n", name, &nChars);
             s += nChars;
          }
          else
             name[0] = 0;         
          printf("Complex Name: %s\n",name);

          s++;
		  /* No. of head titles */
          if(*s != '|')
          {
             sscanf(s, "%[^|]%n", iRef, &nChars);
             s += nChars;
          }
          else
             iRef[0] = 0;
		  //printf("No. Head Titles: %s\n",iRef);

          s++;

          /* head titles */
          if(*s != '|')
          {
             sscanf(s, "%[^|]%n", titles, &nChars);
             s += nChars;
          }
          else
             titles[0] = 0;
		  printf("Head Titles [%s]: %s\n", iRef, titles);

          s++;

          /* get nrows */
          sscanf(s, "%d%n",&nRows,&nChars);
          printf("nRows %d\n",nRows);

        }
        else
        {
          sscanf(s, "%[^|]%n",transId,&nChars);
          printf("Transaction Id %s\n",transId);
          s += nChars;

          if (!sscanf(s, "|%[^|]%n",iRef,&nChars))
          {
             nChars = 1; /* to skip the "|" */
             iRef[0] = 0;
          } /* if */

          s += nChars;
          sscanf(s, "|%d|%d%n",&nCols,&nRows,&nChars);

          if (strcmp(transnType,"OP") == 0)
            printf("nOwners %s nCols %d nRows %d\n",iRef,nCols,nRows);
          else
          {
            if ((strcmp(transnType,"SASU") == 0) ||
               (strcmp(transnType,"SAST") == 0) ||
               (strcmp(transnType,"SAT") == 0))
              printf("LPI Reserve Field <>\n");
            else
              printf("Input Reference <%s>\n",iRef);
            printf("nRows %d  nCols %d\n",nRows,nCols);
          }
        } /* if */
        /* lines before the list */
        rowsOnScreen -= 6;

        printf("List:\n");
        for(; nRows > 0; nRows--)
        {
           ii = nCols;
           for (; ii > 0; ii--)
           {
               s += nChars;
               sscanf(s, "|%[^|]%n",listItem, &nChars);
               printf("%s ",listItem);
           } /* for */
           printf("\n");
           if (--rowsOnScreen == 0) 
           {
              char line[200];  /* for receiving input from the user */
             ReadLine("Enter: output to file (f) or continue scrolling (Y/n)? ",
                          line);
                if(strcmp(line, "f") == 0)
                   return 1;  /* search should be written to a file */
                else if(strcmp(line, "n") == 0)
                   break;
                else /* continue scrolling */
                   rowsOnScreen = ROWS_ON_SCREEN;
            } /* if */
        } /* for */
        return 0;
    case 'X': /* XML */
        printf("XML: %s\n", s);
        return 0;
    default:
        printf("The response type <%c> was not recognised\n", c);
        return 0;
    }

    s += nChars;

    printf("==============================================================" \
           "===============\n");
    state = STATE_START_LINE;
    while(c = *s++)
    {
        short newLine = 0;  /* whether a new line char has just been printed */

        switch(state)
        {
        /* '|' has already been seen for start of line; expect one of the
           following next:
                   nn      a line number;  followed by "|line of text"
                   P       a page break;   followed by "|nn|line of text"
                   E       end of text;    followed by '|' then EOF      */
        case STATE_START_LINE:
            putchar(c);
            digitsSeen = 1;
            state = isdigit(c) ? STATE_LINE_NUMBER : STATE_NEW_LINE;
            break;

        /* 'P' or 'E' has already been seen; ignore the expected '|' and
           print a new line character; expect the following next:
                after "P|", expect "nn|line of text"
                after "E|", expect EOF (so next state becomes irrelevant) */
        case STATE_NEW_LINE:
            putchar('\n');
            newLine = 1;
            digitsSeen = 0;
            state = STATE_LINE_NUMBER;
            break;

        /* reading the digits of the line number; expect one of the following
           next:
                 nn     more line number digits; followed by "n|line of text"
                                                          or "|line of text"
                 |      end of line number;      followed by "line of text" */
        case STATE_LINE_NUMBER:
            if(c == '|')
            {
                if(digitsSeen == 2) putchar(' ');
                else if(digitsSeen == 1) printf("  ");
                state = STATE_TEXT;
            }
            else
            {
                putchar(c);
                digitsSeen++;
            }
            break;

        /* reading line of text; expect one of the following next:
                 more of line of text
                 |                        end of line of text          */
        case STATE_TEXT:
            if(c == '|')
            {
                putchar('\n');
                newLine = 1;
                state = STATE_START_LINE;
            }
            else
            {
                putchar(c);
            }
            break;
        }
        if(newLine && --rowsOnScreen == 0)
        {
            char line[200];  /* for receiving input from the user */

            ReadLine("Enter: output to file (f) or continue scrolling (Y/n)? ",
                     line);
            if(strcmp(line, "f") == 0)
                return 1;  /* search should be written to a file */
            else if(strcmp(line, "n") == 0)
                break;
            else /* continue scrolling */
                rowsOnScreen = ROWS_ON_SCREEN;
        }
    }
    printf("==============================================================" \
           "===============\n");
    return 0;
}

/*===========================================================================
 * Function    : main
 * Purpose     : allows the user to enter requests which are serviced
 *===========================================================================*/

#define BUFFER_SIZE  256000L

int main()
{
    short port;             /* server port number */
    short session;          /* LTO API session number */
    short status;           /* status code */
    char  response[BUFFER_SIZE + 1];/* buffer for receiving response from API*/
    long  actLen=0;           /* actual length of data returned in response */
    short getMore=0;          /* whether there is more data */
    short badField;         /* indicates which field in the query is bad */
    long  charge;           /* transaction charge in cents */
    char  line[20000];      /* for receiving input from the user */
    char *host = API_SERVER;/* host name of machine to connect to */
    char  clientCode[20 + 1];
    long  imageOffset;      /* for handling the saving of D format contents */
    unsigned char key[8];
    char  transnType[10];  /* transaction type eg. RT,OP, etc .. */
#ifdef LT_ENCRYPT
    short i;
    unsigned short u[8];
#endif

#ifdef LT_RESET
    struct sigaction act;

    /* setup alarm handler */
    act.sa_handler = SigHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &act, NULL);
#endif

    if((status = ltInit()) != 0)  /* initialise the LTO API */
    {/* open failed */
        printf("Error from ltInit: status %d\n", status);
        goto error;
    }

    printf("Opening connection to machine %s with API version %s"
#ifdef LT_ENCRYPT
                                                             " (encrypted)"
#endif
                                                                        "\n",
           host, ltAPIVersion);

    /* Get the port at which the Server is running */
    ReadLine("Server port: ", line);
#ifdef LT_ENCRYPT
    if(isdigit(line[0]))
    {
#endif
        port = atoi(line);
 
        /* Get the Client Code                      */
        /* Note: Truncates characters entered above */
        /*       the maximum field size.            */
        ReadLine("Client code: ", line);
        strncpy(clientCode, line, sizeof(clientCode) - 1);

#ifdef LT_ENCRYPT
        ReadLine("Key: ", line);
        if(sscanf(line, "%hu%hu%hu%hu%hu%hu%hu%hu",
                  &u[0], &u[1], &u[2], &u[3], &u[4], &u[5], &u[6], &u[7])
            != 8)
        {
            printf("Bad key\n");
            return 0;
        }
        for(i = 0; i < 8; i++) key[i] = (unsigned char)u[i];
    }
    else
    {/* read from file */
        FILE *fp = fopen(line, "r");

        if(!fp)
        {
            printf("Cannot find file <%s>\n", line);
            return 0;
        }
        if(fscanf(fp, "%hd%s%hu%hu%hu%hu%hu%hu%hu%hu", &port, clientCode,
                  &u[0], &u[1], &u[2], &u[3], &u[4], &u[5], &u[6], &u[7])
            != 10)
        {
            printf("Bad file\n");
            return 0;
        }
        for(i = 0; i < 8; i++) key[i] = (unsigned char)u[i];
        fclose(fp);
    }
#endif

    /* open a session to the server:
           client code                           ?????
           server machine name                   API_SERVER
           server port number on server machine  port    */

    if((status = ltOpen(clientCode, key, host, port, &session)) != 0)
    {/* open failed */
        printf("Error from ltOpen: status %d\n", status);
        goto error;
    }

    printf("Enter query requests or 'q' to quit\n");

    /* loop forever reading user requests and servicing them */
    for(;;)
    {
        short writeFile;
        long  totalLength = actLen;
        short initialGetMore = getMore;

        ReadLine(">", line);

        if(line[0] == 0) continue;

        /* finish if the user has entered a 'q' */
        if(line[0] == 'q') break;

        /* send the query to the server */
#ifdef LT_RESET
        alarm(105);
#endif
        status = ltQuery(session, line, BUFFER_SIZE, response, &actLen,
                         &getMore, &badField, &charge);

        response[actLen] = 0; /* null terminate for printing */
#ifdef LT_RESET
        alarm(0);
#endif

        if(status != 0) printf("status %hd  ", status);

        if(status == LT_WRN_BAD_FIELD) printf("badField %hd  ", badField);

        printf("$%ld.%02ld\n", charge / 100, charge % 100);

        if(response[0] == 0) continue;
        GetTransactionType(line,transnType);
        writeFile = ShowSearch(transnType, actLen, response, getMore, &imageOffset, status);
    tryAgain:
        if(!writeFile)
            ReadLine("Would you like to put the output into a file (y/N)? ",
                     line);
        if(writeFile || strcmp(line, "y") == 0)
        {
            int filDes;

            writeFile = 0;
            ReadLine("file? ", line);
            filDes = open(line, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
            if(filDes == -1)
            {
                printf("could not open file: %s\n", line);
                goto tryAgain;
            }
            /* note that for convenience, when the result of an IG is
               written to a file, only the image data is written, not
               the leading   "D|request-id|"   */
            write(filDes, response + imageOffset, actLen - imageOffset);
            while(getMore)
            {
                status = ltGetMore(session, BUFFER_SIZE, response, &actLen,
                                   &getMore);
                totalLength += actLen;
                printf("Another %ld bytes have been read\n", actLen);
                write(filDes, response, actLen);
            }
            close(filDes);
        }
        else while(getMore)
        {/* get more if more there - ignore data */
            status = ltGetMore(session, BUFFER_SIZE, response, &actLen,
                               &getMore);
            totalLength += actLen;
            printf("Another %ld bytes have been read\n", actLen);
        }
        if(initialGetMore)
            printf("Total of %ld bytes received\n", totalLength);

        if(status == LT_ERR_SERVER_SHUTDOWN)
        {
            printf("\nServer has shutdown\n");
            goto error; 
        }
    }

    ltClose(session);    /* close the LTO API session */

    return 0;

error:
    ltEnd();   /* shutdown the LTO API */

    return status;  /* to keep the compiler quiet */
}
