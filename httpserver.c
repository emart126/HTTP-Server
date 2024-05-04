#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include "queue.h"
#include "rwlock.h"
#include "asgn2_helper_funcs.h"
#include "List.h"

#define ARRAY_SIZE(arr) (sizeof((arr))) / sizeof((arr)[0])
#define BAD_REQUEST     "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n";
#define NOT_IMPLEMENTED                                                                            \
    "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n";
#define UNSUPPORTED_VERSION                                                                        \
    "HTTP/1.1 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion Not Supported\n";
#define NOT_FOUND "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n";
#define FORBIDDEN "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
#define INTERNAL_SERVER_ERROR                                                                      \
    "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal Server Error\n";

// Global variables
queue_t *q;
List listURI;

// Send error message
void errorMessage(const char *msg) {
    write(2, msg, strlen(msg));
    exit(1);
}

// Process the arguments given
void processArgs(int argc, char *argv[], int *port, int *nThreads) {
    int opt = 0;
    opt = getopt(argc, argv, "t:");
    if (opt == 't') {
        *nThreads = atoi(optarg);
    }

    if (argv[optind] == NULL) {
        errorMessage("Missing port number\n");
    }

    *port = atoi(argv[optind]);
    if (*port < 1 || *port > 65535) {
        errorMessage("Invalid port number\n");
    }
}

// Get the substring from index start to len in str[], place in sub[]
void subStr(int start, int len, char str[], char sub[]) {
    for (int i = 0; i < len; i++) {
        sub[i] = str[start + i];
    }
    sub[len] = '\0';
    return;
}

// write response with bad status code and stop client socket
void reset(char full[], char *res, regex_t regx, int socket) {
    sprintf(full, "%s", res);
    write_n_bytes(socket, full, strlen(full));
    regfree(&regx);
    close(socket);
    return;
}

// Get different header fields
void headerFields(char headerFields[], int *reqID, int *contentLen) {
    char *reHeader = "([a-zA-Z0-9.-]{1,128}): ([a-zA-Z0-9.-]{1,128})\r\n";
    regex_t regexH;
    regmatch_t pmatchH[3];
    regoff_t off, len;

    char match[500];
    char key[129];
    char value[129];
    char *headerP = headerFields;

    if (strlen(headerFields) == 0) {
        // No Header fields
        return;
    }

    if (regcomp(&regexH, reHeader, REG_NEWLINE | REG_EXTENDED)) {
        errorMessage("RegexH failed to compile\n");
    }

    while (1) {
        if (regexec(&regexH, headerP, ARRAY_SIZE(pmatchH), pmatchH, 0) != 0) {
            break;
        }

        off = pmatchH[0].rm_so;
        len = pmatchH[0].rm_eo;
        subStr((int) off, (int) len, headerFields, match);

        // Key
        off = pmatchH[1].rm_so;
        len = pmatchH[1].rm_eo - pmatchH[1].rm_so;
        subStr(0, (int) len, headerP + pmatchH[1].rm_so, key);

        // Value
        off = pmatchH[2].rm_so;
        len = pmatchH[2].rm_eo - pmatchH[2].rm_so;
        subStr(0, (int) len, headerP + pmatchH[2].rm_so, value);

        if (strcmp(key, "Request-Id") == 0) {
            *reqID = atoi(value);
        }
        if (strcmp(key, "Content-Length") == 0) {
            *contentLen = atoi(value);
        }

        headerP += pmatchH[0].rm_eo;
    }
    regfree(&regexH);
}

void flushBuffer(char buf[], int size) {
    for (int i = 0; i < size; i++) {
        buf[i] = '\0';
    }
    return;
}

int getMethod(char buffer[], size_t bufSize, char uri[], char fullResponse[], char *startResponse,
    regex_t regex, int fileSoc, int *statusCode) {
    startResponse = "HTTP/1.1 200 OK\r\nContent-Length: ";
    int bytesRead;
    int bytesWritten;
    int fileOpen = open(uri, O_RDWR);

    if (fileOpen < 0) {
        // Forbidden
        if (errno == EISDIR) {
            *statusCode = 403;
            startResponse = FORBIDDEN;
            reset(fullResponse, startResponse, regex, fileSoc);
            close(fileOpen);
            return -1;
        }

        // Not Found
        *statusCode = 404;
        startResponse = NOT_FOUND;
        reset(fullResponse, startResponse, regex, fileSoc);
        close(fileOpen);
        return -1;
    }

    // write content len of file
    off_t contentLen = lseek(fileOpen, 0, SEEK_END);
    sprintf(fullResponse, "%s%d\r\n\r\n", startResponse, (int) contentLen);
    write_n_bytes(fileSoc, fullResponse, strlen(fullResponse));
    lseek(fileOpen, 0, SEEK_SET);

    // Write message body
    while ((bytesRead = read_n_bytes(fileOpen, buffer, bufSize)) != 0) {
        if (bytesRead < 0) {
            // Internal server err
            *statusCode = 500;
            startResponse = INTERNAL_SERVER_ERROR;
            reset(fullResponse, startResponse, regex, fileSoc);
            close(fileOpen);
            return -1;
        }
        int remainingBytes = bytesRead;
        bytesWritten = write_n_bytes(fileSoc, buffer, remainingBytes);
        if (bytesWritten < 0) {
            // Internal Server err
            *statusCode = 500;
            startResponse = INTERNAL_SERVER_ERROR;
            reset(fullResponse, startResponse, regex, fileSoc);
            close(fileOpen);
            return -1;
        }
    }
    close(fileOpen);
    return 0;
}

int putMethod(char buffer[], size_t bufSize, char *bufP, regoff_t startIndex, int contentLenInt,
    char uri[], char fullResponse[], char *startResponse, regex_t regex, int fileSoc, int bytesRead,
    int *statusCode) {
    int isCreated = 0;
    int bytesWritten;
    int fileOpen = open(uri, O_WRONLY | O_TRUNC, 0666);

    if (fileOpen < 0) {
        if (errno == ENOENT) {
            fileOpen = open(uri, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            isCreated = 1;
        } else {
            fprintf(stderr, "Internal: open on put\n");
            *statusCode = 500;
            startResponse = INTERNAL_SERVER_ERROR;
            reset(fullResponse, startResponse, regex, fileSoc);
            return -1;
        }
    }

    int bytesToWrCurrBuf = bytesRead - startIndex;
    if (contentLenInt < bytesToWrCurrBuf) {
        bytesToWrCurrBuf = contentLenInt;
    }
    int totalBytesToWrite = contentLenInt - bytesToWrCurrBuf;

    bytesWritten = write_n_bytes(fileOpen, bufP + startIndex, bytesToWrCurrBuf);
    if (bytesWritten < 0) {
        fprintf(stderr, "Internal server err (put: bytesWritten: %d)\n", bytesWritten);
        *statusCode = 500;
        startResponse = INTERNAL_SERVER_ERROR;
        reset(fullResponse, startResponse, regex, fileSoc);
        close(fileOpen);
        return -1;
    }

    // Check if buffer was full from initial read
    while (totalBytesToWrite > 0) {
        buffer[0] = '\0';
        bytesRead = read_n_bytes(fileSoc, buffer, bufSize);
        bytesWritten = write_n_bytes(fileOpen, buffer, bytesRead);
        totalBytesToWrite -= bytesWritten;
    }

    if (isCreated == 1) {
        *statusCode = 201;
        startResponse = "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n";
    } else {
        startResponse = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
    }
    sprintf(fullResponse, "%s", startResponse);
    write_n_bytes(fileSoc, fullResponse, strlen(fullResponse));
    close(fileOpen);
    return 0;
}

void *dispatcher_thread(void *args) {
    Listener_Socket *socket = (Listener_Socket *) args;
    while (1) {
        // Start Listening to new socket with args as soc
        int *currFileSoc = malloc(sizeof(int));
        *currFileSoc = listener_accept(socket);
        if (*currFileSoc == -1) {
            fprintf(stderr, "Err: %s\n", strerror(errno));
        }

        // Handoff request file descriptor to Worker Thread
        queue_push(q, (void *) currFileSoc);
    }
}

void *worker_thread(void *args) {
    void *fileSocP;
    int myFileSoc;

    char buffer[2100];
    char *bufP = buffer;

    // Request Buffers
    char method[9];
    char uri[65];
    char version[9];
    char headers[2100];

    int requestId;
    int contentLenInt;
    int statusCode;

    // Regex
    char *re = "^([a-zA-Z]{1,8}) (/[a-zA-Z0-9.-]{2,63}) "
               "(HTTP/[0-9]\\.[0-9])\r\n(([a-zA-Z0-9.-]{1,128}: .{1,128}\r\n)*)\r\n((.|\n)+)*$";
    regex_t regex;
    regmatch_t pmatch[8];
    regoff_t off, len;

    while (1) {
        char fullResponse[2100];
        char *startResponse = NULL;
        requestId = 0;
        contentLenInt = 0;
        statusCode = 200;

        // Flush Buffer to be empty
        flushBuffer(buffer, sizeof(buffer));

        // Wait for dispatcher to add to queue
        queue_pop(q, (void **) &fileSocP);
        myFileSoc = (*(int *) fileSocP);

        // Compile regex and collect match values
        if (regcomp(&regex, re, REG_NEWLINE | REG_EXTENDED)) {
            errorMessage("Regex failed to compile\n");
        }

        // Read Request from socket
        int readBytes = read_n_bytes(myFileSoc, buffer, sizeof(buffer));

        // regex, get match of readbytes
        if (regexec(&regex, bufP, ARRAY_SIZE(pmatch), pmatch, 0)) {
            statusCode = 400;
            startResponse = BAD_REQUEST;
            reset(fullResponse, startResponse, regex, myFileSoc);
            continue;
        }

        // Collect group tokens on valid request --------------------------------------------------------

        // Method
        off = pmatch[1].rm_so + (bufP - buffer);
        len = pmatch[1].rm_eo - pmatch[1].rm_so;
        subStr((int) off, (int) len, buffer, method);
        if (strcmp(method, "GET") != 0 && strcmp(method, "PUT") != 0) {
            statusCode = 501;
            startResponse = NOT_IMPLEMENTED;
            reset(fullResponse, startResponse, regex, myFileSoc);
            continue;
        }

        // URI
        off = pmatch[2].rm_so + (bufP - buffer);
        len = pmatch[2].rm_eo - pmatch[2].rm_so;
        subStr((int) off + 1, (int) len - 1, buffer, uri);

        // Version
        off = pmatch[3].rm_so + (bufP - buffer);
        len = pmatch[3].rm_eo - pmatch[3].rm_so;
        subStr((int) off, (int) len, buffer, version);
        if (strcmp(version, "HTTP/1.1") != 0) {
            statusCode = 505;
            startResponse = UNSUPPORTED_VERSION;
            reset(fullResponse, startResponse, regex, myFileSoc);
            continue;
        }

        // Header Fields
        if (pmatch[4].rm_so != 0) {
            off = pmatch[4].rm_so + (bufP - buffer);
            len = pmatch[4].rm_eo - pmatch[4].rm_so;
            subStr((int) off, (int) len, buffer, headers);
        }
        headerFields(headers, &requestId, &contentLenInt);

        // Message
        if (pmatch[6].rm_so != -1) {
            off = pmatch[6].rm_so;
            len = pmatch[6].rm_eo - pmatch[6].rm_so;
        }

        // Add URI to the list for file syncronization -------------------------------
        incrementURI(listURI, uri);

        // Get or PUT ----------------------------------------------------------------
        if (strcmp(method, "GET") == 0) {
            // GET method gets contents of existing URI
            listReaderLock(listURI, uri);

            int result = getMethod(buffer, sizeof(buffer), uri, fullResponse, startResponse, regex,
                myFileSoc, &statusCode);
            fprintf(stderr, "%s,/%s,%d,%d\n", method, uri, statusCode, requestId);

            listReaderUnlock(listURI, uri);

            decrementURI(listURI, uri);

            if (result == -1) {
                continue;
            }
        } else {
            // PUT method puts content into URI if it exists or not

            listWriterLock(listURI, uri);

            int result = putMethod(buffer, sizeof(buffer), bufP, off, contentLenInt, uri,
                fullResponse, startResponse, regex, myFileSoc, readBytes, &statusCode);
            fprintf(stderr, "%s,/%s,%d,%d\n", method, uri, statusCode, requestId);

            listWriterUnlock(listURI, uri);

            decrementURI(listURI, uri);

            if (result == -1) {
                continue;
            }
        }

        regfree(&regex);
        close(myFileSoc);
    }
    return args;
}

int main(int argc, char *argv[]) {
    int port = -1;
    int nThreads = 4;
    q = queue_new(nThreads);
    listURI = newList();

    // Get Thread and Port Argument
    processArgs(argc, argv, &port, &nThreads);

    // Initialize Socket with port
    Listener_Socket soc;
    if (listener_init(&soc, port) != 0) {
        errorMessage("listener_init error\n");
    }

    // Create Threads
    pthread_t threads[nThreads + 1];

    for (int i = 0; i < nThreads; i++) {
        int *threadNumber;
        threadNumber = (int *) malloc(sizeof(int));
        *threadNumber = i;
        pthread_create(threads + i, NULL, worker_thread, threadNumber);
    }
    pthread_create(threads + nThreads, NULL, dispatcher_thread, &soc);

    for (int i = 0; i < nThreads + 1; i++) {
        pthread_join(threads[i], NULL);
    }

    queue_delete(&q);
    freeList(&listURI);
    return (0);
}
