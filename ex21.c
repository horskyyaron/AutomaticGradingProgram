#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>


int isCorrectNumOfArgs(int numOfArgs) {
    if(numOfArgs != 2) {
        printf("wrong num of args, you enetered %d/2 args.\n", numOfArgs);
        return 0;
    }
    return 1;
}

int isSimilar(char c1, char c2) {
    return (c1 == toupper(c2)) || (c1 == tolower(c2)) || (isspace(c1) && isspace(c2));
}

int xOpen(const char* p, int flags) {
    int fd = open(p,flags);
    if(fd < 0) {
        perror("Error in: open\n");
        exit(-1);
    }
    return fd;

}


int xRead(int fd,int prevReadCount, char*buf){
    int bytesRead = prevReadCount;
    for(int i=0; i<2; i++){
        if(bytesRead) {
            bytesRead = read(fd,buf,sizeof(char));
            if(bytesRead < 0) {
                perror("Error in: read\n");
                exit(-1);
            }
            //in case of WINDOWS os, read the \n as well to prevent incosistency in the algorithm.
            if(*buf != '\r')
                break;
        }
    }

    return bytesRead;
}

int xClose(int fd) {
    if(close(fd) !=0){
        perror("Error in: close\n");
        exit(-1);
    }
}

//check if char is either a digit 0-9 or an alphabet a-z A-Z
int isDigOrChar(char c) {
    return isdigit(c) || (c<=90 && c>=65) || (c<=122 && c>=97);
}

int getFilesRatio(const char* p1, const char* p2) {
    int identical = 1;
    int similar = 3;
    int diff = 2;
    int res = identical;
    int fd1 = xOpen(p1,O_RDONLY);
    int fd2 = xOpen(p2,O_RDONLY);
    int totalBytesF1 = 0,totalBytesF2 = 0;
    char buf1,buf2;
    int bytesReadFromFd1,bytesReadFromFd2;

    bytesReadFromFd1 = read(fd1,&buf1,sizeof(char));
    bytesReadFromFd2 = read(fd2,&buf2,sizeof(char));
    totalBytesF1+=bytesReadFromFd1;
    totalBytesF2+=bytesReadFromFd2;
    while(1) {
        //finished scanning both files.
        if(bytesReadFromFd1 == 0 && bytesReadFromFd2 == 0){
            break;
        }

        if(bytesReadFromFd1 == 0 && bytesReadFromFd2 != 0) {
            while(bytesReadFromFd2 != 0) {
                bytesReadFromFd2 = xRead(fd2,bytesReadFromFd2,&buf2);
                totalBytesF2+=bytesReadFromFd2;
                if(isDigOrChar(buf2)){
                    res = diff;
                    break;
                }
            }
            break;
        }
        if(bytesReadFromFd2 == 0 && bytesReadFromFd1 != 0) {
            while(bytesReadFromFd1 != 0) {
                bytesReadFromFd1 = xRead(fd1,bytesReadFromFd1,&buf1);
                totalBytesF1+=bytesReadFromFd1;
                if(isDigOrChar(buf1)){
                    res = diff;
                    break;
                }
            }
            break;
        }


        //reading until comparing two chars.
        if(isDigOrChar(buf1) && !isDigOrChar(buf2)){
            res = similar;
            while(!isDigOrChar(buf2)){
                bytesReadFromFd2 = xRead(fd2,bytesReadFromFd2,&buf2);
                totalBytesF2+=bytesReadFromFd2;
                if(bytesReadFromFd2==0)
                    break;
            }
        }
        if(isDigOrChar(buf2) && !isDigOrChar(buf1)){
            while(!isDigOrChar(buf1)){
                res = similar;
                bytesReadFromFd1 = xRead(fd1,bytesReadFromFd2,&buf1);
                totalBytesF1+=bytesReadFromFd1;
                if(bytesReadFromFd2==0)
                    break;
            }
        }

        if(buf1!=buf2){
            if(isSimilar(buf1,buf2)){
                res = similar;
            } else {
                res = diff;
                break;
            }
        }


        bytesReadFromFd1 = xRead(fd1,bytesReadFromFd1,&buf1);
        bytesReadFromFd2 = xRead(fd2,bytesReadFromFd2,&buf2);
        totalBytesF1+=bytesReadFromFd1;
        totalBytesF2+=bytesReadFromFd2;
    }
    xClose(fd1);
    xClose(fd2);
    if(res == identical && totalBytesF2 != totalBytesF1){
        res = similar;
    }
    return res;

}

int main(int argc, char** argv) {
    int res=-1;
    if(isCorrectNumOfArgs(argc-1)){
        res = getFilesRatio(argv[1],argv[2]);
    }
    return res;
}
