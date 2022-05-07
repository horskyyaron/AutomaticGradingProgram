#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

int isCorrectNumOfArgs(int numOfArgs) {
    if(numOfArgs != 2) {
        printf("wrong num of args, you enetered %d/2 args.\n", numOfArgs);
        return 0;
    } else {
        printf("good number of args\n");
    }
    return 1;
}

int xOpen(const char* p, int flags) {
    int fd = open(p,flags);
    if(fd < 0) {
        perror("after open ");
        exit(-1);
    }
    return fd;
   
}



int getFilesRatio(const char* p1, const char* p2) {
    
    int fd1 = xOpen(p1,O_RDONLY);
    int fd2 = xOpen(p2,O_RDONLY);
    char buf1;
    char buf2;
    int bytesReadFromFd1;
    int bytesReadFromFd2;
     
    bytesReadFromFd1 = read(fd1,&buf1,sizeof(char)); 
    bytesReadFromFd2 = read(fd2,&buf2,sizeof(char)); 
    while(1) {
       if(buf1!=buf2) {
           return 1;
       } 
       if(bytesReadFromFd1) {
           bytesReadFromFd1 = read(fd1,&buf1,sizeof(char)); 
       }
       if(bytesReadFromFd2) {
           bytesReadFromFd2 = read(fd2,&buf2,sizeof(char));
       }
       
       printf("c1=%c|%d, c2=%c|%d\n",(int)buf1,(int)buf1,(int)buf2,(int)buf2);
       if(bytesReadFromFd1 == 0 && bytesReadFromFd2 == 0) {
           break;
       }
    }

    
    return 0;
   
}

int main(int argc, char** argv) {
    int res;
    if(isCorrectNumOfArgs(argc-1)){
        res = getFilesRatio(argv[1],argv[2]);
        if(res == 0) {
            printf("files are identical\n");
        } else {
            printf("files are NOT identical\n");
        }
    } else {
        return 0;
    }
}
