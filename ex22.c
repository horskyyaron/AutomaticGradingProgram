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


int validateDirPath(char* dirPath);

struct Student {
    char name[150];   
    int grade;
    char comment[150];
};

struct Paths {
    char confPath[150];
    char confDir[150];
    char studentsDir[150];
    char inputPath[150];
    char expectedOutPath[150];
    char originRoot[150];
    char studentOutPut[150];
    char studentExec[150];
    char compProgPath[150];
    char resultsFile[150];
    char errorFile[150];
};

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
    char cFilePath[150] = "";
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
        if(errorFd <0){
            perror("Error in: open");
            exit(-1);
        } 
        dup2(errorFd,STDERR_FILENO);
        xClose(errorFd);
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
        if(inputFd < 0 || outputFd < 0 | errorFd < 0){
            perror("Error in: open");
        }
        dup2(inputFd,STDIN_FILENO);
        dup2(outputFd,STDOUT_FILENO);
        dup2(errorFd,STDERR_FILENO);
        xClose(inputFd);
        xClose(outputFd);
        xClose(errorFd);
        execvp(argv[0],argv);
        exit(-1);
    }
    
    //waiting for students prog to finish. 
    waitpid(pid,&status,0);
    if(status != 0){
        return -1;
    }

    return 0;
}

void removeFile(char* file){
    if(remove(file) < 0){
        perror("Exit in: remove");
        exit(-1);
    }
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
    removeFile(p->studentOutPut);
    removeFile(p->studentExec);
    
    return WEXITSTATUS(res);
}

void setGrade(struct Student* s, int grade, const char* comment){
    s->grade = grade;
    strcpy(s->comment,comment);
}

void calcGrade(int compareRes, struct Student* s){
    int res;
    switch(compareRes){
        case 1:
            setGrade(s,100,"EXCELLENT");
            break;
        case 2:
            setGrade(s,50,"WRONG");
            break;
        case 3:
            setGrade(s,75,"SIMILAR");
            break;
    }
}

//return value: 0 succes, -1 error.
int gradeStudent(struct Paths* p, struct Student* s) {
    int hasCFile = 0;
    char cFile[150]="";
    char sDirPathBuffer[150]="";
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
        setGrade(s,0,"NO_C_FILE");
        return 0;
    } else if(hasCFile == -1) {
        //error in opening the student's dir.
        return -1;
    }

    //compile
    if(compileCFile(sDirPathBuffer, cFile, p)==0){
        //run and produce student output file.
        if(runStudentProgram(p)<0){
            setGrade(s,0,"RUNTIME_ERROR");
        } else {
            //grade.
            calcGrade(compareOutput(p), s);
        }
    } else {
        setGrade(s,10,"COMPILATION_ERROR");
    }
    return 0;
}

int xWrite(int fd, char* buf) {
    if(write(fd,buf,strlen(buf))<0){
        perror("Error in: write");
        exit(-1);
    }
    return 0;
}

void addGradeToResultsFile(struct Student *s, struct Paths* p){
   int fd = open(p->resultsFile,O_CREAT | O_APPEND | O_WRONLY ,0777);  
   if(fd < 0) {
       perror("Error in: open");
       return;
   }
   char buf[sizeof(s->name)+sizeof(s->grade)+sizeof(s->comment)];
   sprintf(buf,"%s,%d,%s\n",s->name,s->grade,s->comment);
   xWrite(fd,buf);
   xClose(fd);
}

void getAbsPath(char* path){
    char buf[150];
    memset(buf,0,sizeof(buf));
    realpath(path,buf); 
    strcpy(path,buf);              
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
              xWrite(STDOUT_FILENO, "Not a valid directory\n");
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
                    xWrite(STDOUT_FILENO, "Input file not exist\n");
                } else {
                    xWrite(STDOUT_FILENO, "Output file not exist\n");
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
        xWrite(STDOUT_FILENO,"Invalid number of arguments\n"); 
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
    strcpy(p.resultsFile,"./results.csv");
    strcpy(p.errorFile,"./errors.txt");

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
