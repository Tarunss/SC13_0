#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>
#include <math.h>
#include <ctype.h>
#include "asgn2_helper_funcs.h"
#define PARSE_REQUEST                                                                              \
    "(^[a-zA-Z]{1,8}) /([a-zA-Z0-9._-]{2,64}) "                                                    \
    "HTTP/([0-9.0-9]{1,3})\r\n(.*)\r\n(.*)"
#define PARSE_HEADERS "([a-zA-Z0-9.-]{1,128}): (.{1,128})\r\n"
#define BUFSIZE       2048
// struct to define requestlines

// struct to define requests
typedef struct {
    char *command;
    char *uri;
    char *version;
    char *header_fields;
    char *msg;
    int msg_length;
} Request;

// this module is made to handle errors that may arise when passing through get and set commands
// writes an error message into socket associated with error code
int handle_errors(int fd, int *error_code) {
    if ((*error_code) != 200 || (*error_code) != 201) {
        char *msg;
        switch ((*error_code)) {
        case (400):
            //bad request
            msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n";
            write_all(fd, msg, strlen(msg));
            return 1;
            break;
        case (403):
            //forbidden
            msg = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
            write_all(fd, msg, strlen(msg));
            return 1;
            break;
        case (404):
            //not found
            msg = "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n";
            write_all(fd, msg, strlen(msg));
            return 1;
            break;
        case (500):
            msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal "
                  "Server Error\n";
            write_all(fd, msg, strlen(msg));
            return 1;
            break;
        case (501):
            msg = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n";
            write_all(fd, msg, strlen(msg));
            return 1;
            break;
        case (505):
            msg = "HTTP/1.1 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion "
                  "Not Supported\n";
            write_all(fd, msg, strlen(msg));
            return 1;
            break;
        default: return 0;
        }
    }

    return 0;
}

// get_input method
// Gets the input from file, and returns it into the buffer that was passed
int get_buff(int fd, char *bufString, int *error_code) {
    fprintf(stderr, "Calling in read to buffer\n");

    int bytes = read_until(fd, bufString, BUFSIZE, "\r\n");
    if (bytes < 0) {
        *error_code = 400;
        fprintf(stderr, "Error reading in buffer\n");
    }
    return bytes;
}

// parse_input method
// Takes in a request structure, a string w/input, and an error code string
// Will change error code to according digits based on whether parsing succeeds
// Parses input into pointers, and sets request's fields to said pointers

int parse_input(Request *req, char *buff, int *error_code, int bytes_read) {
    //setting up regex for parsing
    regex_t reg;
    regmatch_t matches[6];
    int rc;

    //compile regex
    rc = regcomp(&reg, PARSE_REQUEST, REG_EXTENDED);
    if (rc != 0) {
        printf("error compiling parsing string\n");
        exit(1);
    }

    //execute regex
    //printf("compiled first regex\n");
    rc = regexec(&reg, buff, 6, matches, 0);
    //printf("%s\n", copyBuff);

    //matches have been found
    if (rc == 0) {
        req->command = buff + matches[1].rm_so;
        req->uri = buff + matches[2].rm_so;
        req->version = buff + matches[3].rm_so;
        req->header_fields = buff + matches[4].rm_so;
        req->msg = buff + matches[5].rm_so;
        //printf("successfully assigned commands and requests\n");
        //add null terminators
        req->command[matches[1].rm_eo] = '\0';
        req->uri[matches[2].rm_eo - matches[2].rm_so] = '\0';
        req->version[matches[3].rm_eo - matches[3].rm_so] = '\0';
        req->header_fields[(matches[4].rm_eo - matches[4].rm_so)] = '\0';
        req->msg_length = bytes_read - ((matches[4].rm_eo + 2));

    } else { // no match, bad request
        printf("Bad Request\n");
        //exit(1);
        req->command = NULL;
        req->uri = NULL;
        req->version = NULL;
        req->header_fields = NULL;
        req->msg = NULL;
        (*error_code) = 400; //400 is bad request
        return (*error_code);
    }
    //check for empty header_fields
    char *savedValue;
    char *key;
    char *value;
    char *next = &req->header_fields[0]; // beginning of string
    char *last_byte = &req->header_fields[strlen(req->header_fields)];
    while (next < last_byte - 2) {
        //printf("%s\n", next);
        if (strncmp(next, "\r\n", 2) != 0) {
            regex_t headerReg;
            regmatch_t headerFields[3];
            int rc2;
            // compile regex
            rc2 = regcomp(&headerReg, PARSE_HEADERS, REG_EXTENDED);
            if (rc2 != 0) {
                printf("error with first header parsing string\n");
                exit(1);
            }
            // match first key value pair
            char temp[256];
            int i = 0;
            while (next[i] != '\r' && next[i + 1] != '\n') {
                //copy first key value pair into
                temp[i] = next[i];
                i++;
            }
            temp[i] = '\r';
            temp[i + 1] = '\n';
            temp[i + 2] = '\0';
            rc2 = regexec(&headerReg, temp, 3, headerFields, 0);
            if (rc2 != 0) {
                //invalid header field, set everthing to null and return
                //printf("invalid header\n");
                req->command = NULL;
                req->uri = NULL;
                req->version = NULL;
                req->header_fields = NULL;
                req->msg = NULL;
                *error_code = 400;
                return (*error_code);
            }
            //assign value and key
            key = temp + headerFields[1].rm_so;
            value = temp + headerFields[2].rm_so;
            //assign null terminators (for comparison)
            char keyNull = key[headerFields[1].rm_eo];
            char valNull = value[headerFields[2].rm_eo - headerFields[2].rm_so];
            key[headerFields[1].rm_eo] = '\0';
            value[headerFields[2].rm_eo - headerFields[2].rm_so] = '\0';
            //check to see if key value matches content_length: int
            if (strcmp(key, "Content-Length") == 0) {
                savedValue = next + headerFields[2].rm_so;
            }
            //take out null terminators to continue looping
            key[headerFields[1].rm_eo] = keyNull;
            value[headerFields[2].rm_eo - headerFields[2].rm_so] = valNull;
            //do another regex match on header_fields in order to test for validity
            next += (i + 2);
            //free regex
            regfree(&headerReg);
        }
    }
    req->header_fields = savedValue;
    //free regex
    regfree(&reg);
    fprintf(stderr, "successfully assigned second pass regex\n");
    return (*error_code);
}

// PUT
// put takes in a request and writes to a file, either:
// A) Creating the file if it doesn't exist, and writing a message to said file
// B) Overwriting the file if it does exist with message in request
//
int put(int fd, Request *req, int *error_code) {
    fprintf(stderr, "starting put operation\n");
    //check to see if file exists
    *error_code = 200;
    //stats for socket
    //cast string into ssize_t
    ssize_t contentLength = 0;
    sscanf(req->header_fields, "%zu", &contentLength);
    //stats for file destination
    struct stat path;
    int exist = stat(req->uri, &path);
    //check to see if file exists
    if (exist != 0) {
        //file doesnt exist, so it will be created
        *error_code = 201;
    } else if (!S_ISREG(path.st_mode)) {
        //check to see if file is a directory
        (*error_code) = 403;
        return *error_code;
    }
    int filepath = creat(req->uri, 00700);

    //we need to write a message into the file
    fprintf(stderr, "Writing message offset from req->msg to file\n");

    int err = write_all(filepath, req->msg, req->msg_length);
    if (err < 0) {
        *error_code = 500;
        return *error_code;
    }
    //if there is still more to write, use pass_bytes to write the difference
    if (contentLength - req->msg_length > 0) {
        int err2 = pass_bytes(fd, filepath, (contentLength - req->msg_length));
        if (err2 < 0) {
            *error_code = 500;
            return *error_code;
        }
    }
    //now we create the message
    char *msg = NULL;
    if (*error_code == 201) {
        msg = "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n";
        //write this msg to the socket
    } else if (*error_code == 200) {
        msg = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
    }
    //write stub to socket
    int err3 = write_all(fd, msg, strlen(msg));
    if (err3 < 0) {
        *error_code = 500;
        return *error_code;
    }
    close(filepath);
    return *error_code;
}

// GET
// get takes in a request and looks for the file specified within it to return
// contents from it.
// returns a message tailored to assignment 2 document
int get(int fd, Request *req, int *error_code) {
    // check to see if file exists, open to read only
    //printf("writing message to file destination\n");
    *error_code = 200;
    int filepath = open(req->uri, O_RDONLY);
    struct stat path;
    int exists = stat(req->uri, &path);
    off_t bytes = path.st_size;
    //printf("%'jd\n", bytes);

    if (exists != 0) {
        //file does not exist
        (*error_code) = 404;
        return *error_code;
    } else if (!S_ISREG(path.st_mode)) {
        //file does not exist
        (*error_code) = 403;
        return *error_code;
    }

    else {
        //get the total amount of bytes from uri specified
        //calculating number of bytes for correct allocation of memory
        int numbytes = 0;
        int n = bytes;
        if (n == 0) {
            numbytes = 1;
        } else {
            while (n != 0) {
                n = n / 10;
                numbytes++;
            }
        }
        char msg[34] = "HTTP/1.1 200 OK\r\nContent-Length: ";
        char cbytes[numbytes];
        snprintf(cbytes, sizeof(cbytes) + 1, "%'jd", bytes);
        strcat(msg, cbytes);
        strcat(msg, "\r\n\r\n");

        //write the correct response to socket
        write_all(fd, msg, strlen(msg));
        //read bytes from specified file and write it into the socket
        bytes = path.st_size;

        int err4 = pass_bytes(filepath, fd, bytes);
        if (err4 < 0) {
            *error_code = 500;
            return *error_code;
        }
        //close filepath
        close(filepath);
    }
    //printf("%d\n", *error_code);
    return *error_code;
}

//Main is where most functions will be called
//creation of echo server
//new Listener Socket
int main(int argc, char *argv[]) {
    if (argc != 2) {
        return 1;
    }
    //check port command arguments
    int port = atoi(argv[1]);
    //create Listener socket
    Listener_Socket sock;
    //check initialization
    int sockfd = listener_init(&sock, port);
    if (sockfd < 0) {
        printf("socket error\n");
        return 1;
    }
    //server is constantly accepting stuff (is running)
    while (1) {
        int listenfd = listener_accept(&sock);
        //struct Request req; //where we will store parsed data
        //proccess code
        if (listenfd > 0) {
            //buffer string to hold input
            char buff[BUFSIZE] = { 0 };
            //error code will tell us what errors we run into along the way
            int error_code = 200;
            int bytes = get_buff(listenfd, buff, &error_code);
            fprintf(stderr, "%d\n", bytes);
            if (error_code != 200) {
                handle_errors(listenfd, &error_code);
            } else {
                //printf("%d\n", bytes);
                Request req;
                //allocate memory for req
                error_code = parse_input(&req, buff, &error_code, bytes);
                //printf("%d\n", error_code);

                //handle errors that may have arisen from ill-formatted requests
                if (error_code != 200) {
                    handle_errors(listenfd, &error_code);
                }
                //check if command is get or set
                else if (strncmp(req.version, "1.1", 3) != 0) {
                    error_code = 505;
                    handle_errors(listenfd, &error_code);
                } else if (strcmp(req.command, "GET") == 0) {
                    error_code = get(listenfd, &req, &error_code);
                    handle_errors(listenfd, &error_code);
                } else if (strcmp(req.command, "PUT") == 0) {
                    error_code = put(listenfd, &req, &error_code);
                    handle_errors(listenfd, &error_code);
                } else {
                    error_code = 501;
                    handle_errors(listenfd, &error_code);
                }
            }
        }
        close(listenfd);
    }
    return 0;
}
