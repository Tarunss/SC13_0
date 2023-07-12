#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#define PAGE_SIZE 4104
// Other functions we will need to create full program
//gets the user input using read() system call from stdin

//takes in buffer and returns either set or get
char *getCommandStr(char *buffer) {
    char *command;
    //allocate memory for command
    command = (char *) calloc(sizeof(char), 5);
    for (int i = 0; i < 4; i++) {
        command[i] = buffer[i];
    }
    //printf("%s\n", command);
    if (buffer[5] == ' ') {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }
    //search for newline

    char temp[5] = "get \0";
    //bool eq;
    if (strcmp(command, temp) == 0) {
        int j = 0;
        while (buffer[j] != '\n' && j < PAGE_SIZE - 1) {
            j++;
        }
        if (buffer[j + 1] != '\0') {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }
        return command;
    }
    //i hate strcmp with a burning passion
    char temp2[5] = "set \0";
    if (strcmp(command, temp2) == 0) {
        return command;
    }
    free(command);
    fprintf(stderr, "Invalid Command\n");
    exit(1);
}
//takes in buffer and returns file location
char *getCommandLoc(char *buffer, int bytes) {
    //printf("%s", buffer);
    int fileLength = 0;
    //loop to get length of file location
    int i = 4;
    for (i = 4; buffer[i] != '\n' && i < bytes; i++) {
        fileLength++;
    }
    if (fileLength == bytes) {
        fprintf(stderr, "No newline at end of file\n");
        exit(1);
    }
    //printf("%c", buffer[i]);
    // if (buffer[i] != 0) {
    //     fprintf(stderr, "Invalid Command\n");
    //     exit(1);
    // }
    //printf("%d\n",fileLength);
    //copy filelocation into variable
    char *fileLoc;
    fileLoc = (char *) calloc(sizeof(char), fileLength + 1);
    for (int i = 4; buffer[i] != '\n' && i < bytes; i++) {
        fileLoc[i - 4] = buffer[i];
    }
    //printf("%s\n",fileLoc);
    return fileLoc;
}
char *getCommandCont(char *buffer, int bytes) {
    //loop until newline character
    int index = 4;
    while (buffer[index] != '\n') {
        index++;
    }
    //printf("%d\n",index);
    //printf("%lu\n",strlen(buffer));
    //now we have index of newline character, subtract page_size from it to obtain contents size
    char *contents;
    int length = bytes - (index + 1);
    contents = (char *) calloc(sizeof(char), (length + 1));
    for (int j = 0; j < length; j++) {
        //printf("%c\n", buffer[index-1]);
        contents[j] = buffer[index + 1];
        //printf("%d\n",j);
        //  printf("%lu\n",strlen(buffer) - index);
        index++;
    }
    return contents;
}

// setCommand takes in a file destination then uses write() to write into that file.
// Takes in file destination, and a string to write into that file
void setCommand(int filepath);

// getCommand takes in a string,then uses read() to get the contents fo that file.
// Takes in a file destination
void getCommand(int filepath) {
    int bytes_read = 0;
    char bufGet[PAGE_SIZE];
    while ((bytes_read = read(filepath, bufGet, PAGE_SIZE)) > 0) {
        int bytes_written = 0;
        while (bytes_written < bytes_read) {
            int bytes = write(STDOUT_FILENO, &bufGet[bytes_written], bytes_read - bytes_written);
            if (bytes <= 0) {
                fprintf(stderr, "cannot write to stdout");
            }
            bytes_written += bytes;
        }
    }
}

// Main will parse contents to create
int main() {
    // nread contains the # of characters within the buffer
    int nread;
    char buffer[PAGE_SIZE] = { 0 }; //string of characters
    int filepath;
    //get user command
    if ((nread = read(STDIN_FILENO, buffer, PAGE_SIZE)) > 0) {
        //Parse user input in order to call get and set
        char *fileLoc = getCommandLoc(buffer, nread);
        char *command = getCommandStr(buffer);
        //printf("%d\n",nread);
        //printf("%s\n",buffer);
        //printf("%s\n",fileLoc);
        //printf("%s\n", command);
        //if command is get
        if (strcmp(command, "get ") == 0) {
            filepath = open(fileLoc, O_RDONLY);
            if (filepath < 0) {
                fprintf(stderr, "Invalid Command\n");
                exit(1);
            }
            struct stat path;

            stat(fileLoc, &path);
            if (!S_ISREG(path.st_mode)) {
                fprintf(stderr, "Invalid Command\n");
                exit(1);
            }
            getCommand(filepath);
            //close the file path
            free(fileLoc);
            free(command);
            close(filepath);
            return 0;
        }
        //if command is set
        else if (strcmp(command, "set ") == 0) {
            char *contents = getCommandCont(buffer, nread);
            //printf("%s\n", contents);
            //open filepath

            // struct stat path_stat;
            // stat(fileLoc, &path_stat);
            // if (S_ISREG(path_stat.st_mode) != 0) {
            //     fprintf(stderr, "Invalid Command\n");
            //     exit(1);
            // }
            filepath = open(fileLoc, O_WRONLY | O_CREAT | O_TRUNC, 00700);
            if (filepath < 0) {
                fprintf(stderr, "Invalid Command\n");
                exit(1);
            }
            //setcommand
            int length = nread - (strlen(fileLoc) + (strlen(command) + 1));
            // while (contents[length] != '\0') {
            //     //printf("%c", fileContents[length]);
            //     length++;
            // }
            write(filepath, contents, length);
            int c = 0;
            char *newBuffer[PAGE_SIZE];
            while ((c = read(STDIN_FILENO, newBuffer, PAGE_SIZE)) > 0) {
                write(filepath, newBuffer, c);
            }
            printf("OK\n");
            close(filepath);
            free(fileLoc);
            free(command);
            free(contents);
            return 0;

        }
        // invalid command
        else {
            free(fileLoc);
            free(command);
            fprintf(stderr, "No newline at end of file");
            exit(1);
        }
        free(command);
        free(fileLoc);
    }

    //write(1,buffer1,nread);
    return 0;
}
