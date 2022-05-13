#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>

#define MAX_PATH 150
#define MAX_CONFIG_FILE_LINE 150
#define MAX_STU_NAME_SIZE 100
#define MAX_COMMENT_SIZE 50 

int validateDirPath(char* dirPath);

struct Student {
    char name[MAX_STU_NAME_SIZE];   
    int grade;
    char comment[MAX_COMMENT_SIZE];
};

struct Paths {
    char confPath[MAX_PATH];
    char confDir[MAX_PATH];
    char studentsDir[MAX_PATH];
    char inputPath[MAX_PATH];
    char expectedOutPath[MAX_PATH];
    char originRoot[MAX_PATH];
    char studentOutPut[MAX_PATH];
    char studentExec[MAX_PATH];
    char compProgPath[MAX_PATH];
    char resultsFile[MAX_PATH];
    char errorFile[MAX_PATH];
};

void testPrint(char* msg, char* str){
    if(msg != NULL) {
        printf("%s %s\n", msg, str);
    } else {
        printf("%s\n", str);
    }
}
int isCorrectNumOfArgs(int argc) {
    return argc == 1;
}

int xOpen(const char* p, int flags) {
    int fd = open(p,flags);
    if(fd < 0) {
        perror("Error in: open");
        return -1;
    }
    return fd;

}

int xAccess(const char* p, int mode) {
    if(access(p,mode)<0){
        perror("Error in: access");
        return -1;
    }
    return 0;
}

int xClose(int fd) {
    if(close(fd) !=0){
        perror("Error in: close");
        return -1;
    }

    return 0;
}

int xReadLine(int fd, char* buf) {
    int i = 0;
    char c = 'a';
    while(c != '\n'){
        if(read(fd,&c,sizeof(char))<0){
            //reading error in the configuration file will lead to unwanted results. terminate program in this case.
            perror("Error in: read");
            exit(-1);
        }
        buf[i] = c;
        i++;
    }
    buf[i-1] = '\0';
    return 0;
}

int xChdir(char* path){
    if(chdir(path)!=0){
        perror("Error in: chdir");
        return -1;
    } 
    return 0;
}

void readConfFile(struct Paths* p) {
    int fd = xOpen(p->confPath, O_RDONLY);
    xReadLine(fd, p->studentsDir);  
    xReadLine(fd, p->inputPath); 
    xReadLine(fd, p->expectedOutPath);
    xClose(fd);
    
}

void buildStudentDir(char* buf, char* studentsDirPath, char* s_name) {
    strcpy(buf,studentsDirPath);
    buf[strlen(buf)] = '/';
    strcat(buf,s_name);
}

// return value: 1 if file found, 0 otherwise. -1 if error found
int getCFile(char* dir,char* cFileBuf) {
    int hasCFile = 0; 
    DIR *dip;
    struct dirent *dit;

    if((dip = opendir(dir)) == NULL){
        perror("Error in: opendir");
        return -1;
    }
    
    //scanning the student directory for c-cFileBuf.
    while((dit = readdir(dip)) != NULL) {
        if(dit->d_type == DT_REG){
            int len = strlen(dit->d_name);
            if(dit->d_name[len-1] == 'c' && dit->d_name[len-2] == '.'){
                hasCFile = 1;
                strcpy(cFileBuf, dit->d_name);
                break;
            }
        }
    }
    return hasCFile;
}

int compileCFile(char *dir, char* fileName, struct Paths* p){
    int status; 
    char cFilePath[MAX_PATH] = "";
    strcat(cFilePath, dir);
    strcat(cFilePath,"/");
    strcat(cFilePath, fileName);

    //building the exec command arguments.
    char *argv[5];
    argv[0] = "gcc";
    argv[1] = cFilePath; 
    argv[2] = "-o";
    argv[3] = p->studentExec;
    argv[4] = NULL;

    //compiling in the child process.
    int pid = (int) fork();
    if (pid == 0) {
        //redirecting stderr to erros.txt file.
        int errorFd= open(p->errorFile,O_CREAT | O_APPEND | O_WRONLY ,0777);  
        dup2(errorFd,STDERR_FILENO);
        close(errorFd);
        execvp(argv[0],argv);
        exit(-1);
    }

    //waiting for cimpilation to finish. return gcc exit code.
    waitpid(pid,&status,0);
    return WEXITSTATUS(status);
}

int runStudentProgram(struct Paths* p){
    int status;
    char *argv[2];
    argv[0] = p->studentExec;
    argv[1] = NULL;

    //running student's program.
    int pid = (int) fork();
    if (pid == 0) {
        //redirecting output and input file descriptors.
        xChdir(p->originRoot);
        int inputFd = open(p->inputPath,O_RDWR);
        int outputFd = open(p->studentOutPut ,O_CREAT | O_WRONLY, 0777);
        int errorFd= open(p->errorFile,O_CREAT | O_APPEND | O_WRONLY ,0777);  
        dup2(inputFd,STDIN_FILENO);
        dup2(outputFd,STDOUT_FILENO);
        dup2(errorFd,STDERR_FILENO);
        close(inputFd);
        close(outputFd);
        close(errorFd);
        execvp(argv[0],argv);
        exit(-1);
    }
    
    //waiting for cimpilation to finish. return gcc exit code.
    waitpid(pid,&status,0);
    if(WEXITSTATUS(status) == -1){
        return -1;
    }

    return 0;
}

int compareOutput(struct Paths* p){
    int res;
    int pid = (int) fork();
    if(pid == 0) {
        char *argv[4];
        argv[0] = p->compProgPath;
        argv[1] = p->studentOutPut;
        argv[2] = p->expectedOutPath; 
        argv[3] = NULL;
        execvp(argv[0],argv);
        exit(-1);
    }
    waitpid(pid,&res,0);

    //removing student output after comparing with expected output file.
    if(remove(p->studentOutPut) < 0) {
        perror("Exit in: remove");
        exit(-1);
    }
    
    if(remove(p->studentExec) < 0) {
        perror("Exit in: remove");
        exit(-1);
    }
    
    return WEXITSTATUS(res);
}

void calcGrade(int compareRes, struct Student* s){
    int res;
    switch(compareRes){
        case 1:
            s->grade = 100;
            strcpy(s->comment, "EXCELLENT");
            break;
        case 2:
            s->grade = 50;
            strcpy(s->comment, "WRONG");
            break;
        case 3:
            s->grade = 75;
            strcpy(s->comment, "SIMILAR");
            break;
    }
}

//return value: 0 succes, -1 error.
int gradeStudent(struct Paths* p, struct Student* s) {
    int hasCFile = 0;
    char cFile[MAX_PATH]="";
    char sDirPathBuffer[MAX_PATH]="";
    int res, grade;

    //building the path to student's folder,
    buildStudentDir(sDirPathBuffer, p->studentsDir, s->name);
    //if student's dir is restricted.
    if(validateDirPath(sDirPathBuffer)==-1) {
        return -1;
    }

    //scan for c file in student's dir.
    hasCFile = getCFile(sDirPathBuffer, cFile);

    if(hasCFile == 0){
        //NO_C_FILE error.
        s->grade = 0;
        strcpy(s->comment,"NO_C_FILE");
        return 0;
    } else if(hasCFile == -1) {
        return -1;
    }

    //compile
    if(compileCFile(sDirPathBuffer, cFile, p)==0){
        //run and produce student output file.
        runStudentProgram(p);
        //grade.
        calcGrade(compareOutput(p), s);
    } else {
        s->grade = 10;
        strcpy(s->comment,"COMPILATION_ERROR");
    }
    return 0;
}

int xWrite(int fd, char* buf, size_t count) {
    if(write(fd,buf,strlen(buf))<0){
        perror("Error in: write");
        printf("%d",errno);
        exit(-1);
    }
    return 0;
}

void addGradeToResultsFile(struct Student *s, struct Paths* p){
   int fd = open(p->resultsFile,O_CREAT | O_APPEND | O_WRONLY ,0777);  
   char buf[sizeof(s->name)+sizeof(s->grade)+sizeof(s->comment)];
   sprintf(buf,"%s,%d,%s\n",s->name,s->grade,s->comment);

   xWrite(fd,buf,strlen(buf));
          
}

void getAbsPath(char* path){
    char buf[100];
    memset(buf,0,sizeof(buf));
    realpath(path,buf); 
    strcpy(path,buf);              
}

void getAbsPathForConfPaths(struct Paths* p) {
    getAbsPath(p->studentsDir);
    getAbsPath(p->inputPath);
    getAbsPath(p->expectedOutPath);
}


//check if dir is valid. return -1 if invalid. 0 if valid.
int validateDirPath(char* dirPath){
    int res=0;
    struct stat s;
    if(stat(dirPath, &s)<-1){
        perror("Error in: stat");
    } 
    //check if dir exits and if it's a directory path.
    if(xAccess(dirPath,F_OK | X_OK)<0 || !S_ISDIR(s.st_mode)){
        res =-1;
    } 
    return res;  
}

void validateDir(struct Paths* p){
    if(validateDirPath(p->studentsDir)<0) {
       if(xChdir(p->confDir)==0){
          if(validateDirPath(p->studentsDir)<0){
              printf("Not a valid directory\n");
              exit(-1);
          }
          getAbsPath(p->studentsDir);
          if(xChdir(p->originRoot)!=0){
              exit(-1);
          }
       }
    } 
    getAbsPath(p->studentsDir);
}

//check if given path to a file is accesible and is a file type.
int isFileAndAccsessible(char* path){
    int res=0;
    struct stat s;
    if(stat(path, &s)<0){
        perror("Error in: stat");
        res= -1;
    } else {
        //check if file exits and if it's a file path.
        if(xAccess(path,F_OK | R_OK)<0 || !S_ISREG(s.st_mode)){
            res =-1;
        } 
    }
    return res;  
}

//check if given IO file path is accessible and of file type in root dir and conf file dir.
void validateIOFile(struct Paths* p, char* ioPath){
    if(isFileAndAccsessible(ioPath)<0) {
        if(xChdir(p->confDir)==0){
            if(isFileAndAccsessible(ioPath)<0){
                if(strcmp(ioPath,p->inputPath)==0){
                    printf("Input file not exist\n");
                } else {
                    printf("Output file not exist\n");
                } 
                exit(-1);
            }
            getAbsPath(ioPath);
            if(xChdir(p->originRoot)!=0){
                exit(-1);
            }
        }
    }
    getAbsPath(ioPath);
}

void validateIOFiles(struct Paths *p){
    validateIOFile(p,p->inputPath);
    validateIOFile(p,p->expectedOutPath);
}

void validateConfFileContent(struct Paths* p) {
    validateDir(p);
    validateIOFiles(p);
}

void getConfFileAndDirPaths(struct Paths* p, char* confPath) {
    char buf[100];
    // handling case where the conf argumnet comes as 'conf.txt' with no './' at the beginning.
    int hasSlash = 0;
    for(int i =0; i<strlen(confPath);i++){
        if(confPath[i]=='/'){
            hasSlash=1;
            break;
        }
    }
    if(!hasSlash){
        strcpy(buf,"./");
        strcat(buf, confPath);
    } else {
        strcpy(buf, confPath);
    }  
    realpath(buf, p->confPath); 
    char *confPathCpy = strdup(p->confPath);
    char *dir = dirname(confPathCpy);
    strcpy(p->confDir, dir);
    free(dir);
}

void getPaths(struct Paths* p, char* confPath) {
    getcwd(p->originRoot,sizeof(p->originRoot));
    getConfFileAndDirPaths(p,confPath);
    readConfFile(p);
    
    validateConfFileContent(p); 
    
}


int main(int argc, char **argv) {
    if(!isCorrectNumOfArgs(argc-1)){
        printf("wrong number of inputs\n");
        exit(-1);
    }

    struct Paths p;
    DIR *studentsDirPtr;
    struct dirent *dirItem;
    struct Student s;

    //getting all the paths from the configuration file.
    getPaths(&p, argv[1]);
    strcpy(p.studentOutPut,"./student_out.txt");
    strcpy(p.studentExec,"./student_exec.out");
    strcpy(p.compProgPath, "./comp.out");
    strcpy(p.resultsFile, "./results.csv");
    strcpy(p.errorFile, "./error.txt");

    if((studentsDirPtr = opendir(p.studentsDir)) == NULL){
        perror("Error in: opendir");
        exit(-1);
    }
    
    //scanning the student directory, ignoring non-directory files.
    while((dirItem = readdir(studentsDirPtr)) != NULL) {
        if(dirItem->d_type == DT_DIR && strcmp(dirItem->d_name,".") != 0 && strcmp(dirItem->d_name,"..") != 0){
            // checking student, results will be saved to student's struct.
            strcpy(s.name, dirItem->d_name);
            if(gradeStudent(&p, &s) == 0) {
                // adding student's results to results file,
                addGradeToResultsFile(&s,&p);    
            }         
        }
    }
    return 0; 
}
