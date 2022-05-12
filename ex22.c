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
};

int isCorrectNumOfArgs(int argc) {
    return argc == 1;
}

int xOpen(const char* p, int flags) {
    int fd = open(p,flags);
    if(fd < 0) {
        perror("Error in: open");
        exit(-1);
    }
    return fd;

}

int xClose(int fd) {
    if(close(fd) !=0){
        perror("Error in: close");
        exit(-1);
    }
    return 0;
}

int xReadLine(int fd, char* buf) {
    int i = 0;
    char c = 'a';
    while(c != '\n'){
        if(read(fd,&c,sizeof(char))<0){
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
        exit(-1);
    } else {
        return 0;
    }
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

// return 1 if file found, 0 otherwise. c file name is saved into buf.
int getCFile(char* dir,char* cFileBuf) {
    int hasCFile = 0; 
    DIR *dip;
    struct dirent *dit;

    if((dip = opendir(dir)) == NULL){
        perror("Error in: opendir");
        exit(-1);
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
        execvp(argv[0],argv);
        exit(1);
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
        int input = open(p->inputPath,O_RDWR);
        int output = open(p->studentOutPut ,O_CREAT | O_WRONLY, 0777);
        dup2(input,STDIN_FILENO);
        dup2(output,STDOUT_FILENO);
        close(input);
        close(output);
        execvp(argv[0],argv);
        exit(1);
    }

    //waiting for cimpilation to finish. return gcc exit code.
    waitpid(pid,&status,0);
    return status == 0;

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

void gradeStudent(struct Paths* p, struct Student* s) {
    int hasCFile = 0;
    char cFile[MAX_PATH]="";
    char sDirPathBuffer[MAX_PATH]="";
    int res, grade;

    //building the path to student's folder,
    buildStudentDir(sDirPathBuffer, p->studentsDir, s->name);

    //scan for c file in student's dir.
    if(!getCFile(sDirPathBuffer, cFile)){
        //NO_C_FILE error.
        s->grade = 0;
        strcpy(s->comment,"NO_C_FILE");
        return;
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

void getFullPathToFile(char* path, struct Paths* p){
   char *dirc, *basec, *bname, *dname;
   dirc = strdup(path);
   basec = strdup(path);
   dname = dirname(dirc);
   bname = basename(basec);

   //trying to change dir to file's dir from the origin dir.
   if(access(dname,F_OK)==0){
       if(xChdir(dname)==0){
           getcwd(path,MAX_PATH);
       }
   } else {
       //changing to conf.txt dir and trying to access from there (case where relative path is from conf.txt file and not from root).
       if(xChdir(p->confDir)==0){
           if(xChdir(dname)==0){
               getcwd(path,MAX_PATH);
           }
       }
   }

   strcat(path,"/");
   strcat(path, bname);

   //cd back to root.
   xChdir(p->originRoot);
   free(dirc);
   free(basec);
}

void getFullPathToDirFromFileInFolder(char* path, struct Paths* p){
   char *dirc, *basec, *bname, *dname;
   dirc = strdup(path);
   dname = dirname(dirc);

   //trying to change dir to file's dir from the origin dir.
   if(access(dname,F_OK)==0){
       if(xChdir(dname)==0){
           getcwd(path,MAX_PATH);
       }
   } else {
       //changing to conf.txt dir and trying to access from there (case where relative path is from conf.txt file and not from root).
       if(xChdir(p->confDir)==0){
           if(xChdir(dname)==0){
               getcwd(path,MAX_PATH);
           }
       }
   }

   //cd back to root.
   xChdir(p->originRoot);
   free(dirc);
} 

void getFullPathToFolderFromFolderPath(char* path,struct Paths* p) {
   //trying to change dir to file's dir from the origin dir.
   if(access(path,F_OK)==0){
       if(xChdir(path)==0){
           getcwd(path,MAX_PATH);
       }
   } else {
       //changing to conf.txt dir and trying to access from there (case where relative path is from conf.txt file and not from root).
       if(xChdir(p->confDir)==0){
           if(xChdir(path)==0){
               getcwd(path,MAX_PATH);
           }
       }
   }
   xChdir(p->originRoot);
}

void getPaths(struct Paths* p, char* confPath) {
    getcwd(p->originRoot,sizeof(p->originRoot));
    // handling case where the conf argumnet comes as 'conf.txt' with no './' at the beginning.
    int hasSlash = 0;
    for(int i =0; i<strlen(confPath);i++){
        if(confPath[i]=='/'){
            hasSlash=1;
            break;
        }
    }
    if(!hasSlash){
        strcpy(p->confPath,"./");
        strcat(p->confPath, confPath);
    } else {
        strcpy(p->confPath, confPath);
    }  
    readConfFile(p);
    strcpy(p->confDir,p->confPath);
    getFullPathToDirFromFileInFolder(p->confDir,p);
    getFullPathToFile(p->confPath,p);
    getFullPathToFile(p->inputPath,p);
    getFullPathToFile(p->expectedOutPath,p);
    getFullPathToFolderFromFolderPath(p->studentsDir,p);
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

    if((studentsDirPtr = opendir(p.studentsDir)) == NULL){
        perror("Error in: opendir");
        exit(-1);
    }
    
    //scanning the student directory, ignoring non-directory files.
    while((dirItem = readdir(studentsDirPtr)) != NULL) {
        if(dirItem->d_type == DT_DIR && strcmp(dirItem->d_name,".") != 0 && strcmp(dirItem->d_name,"..") != 0){
            // checking student, results will be saved to student's struct.
            strcpy(s.name, dirItem->d_name);
            gradeStudent(&p, &s); 
            // adding student's results to results file,
            addGradeToResultsFile(&s,&p);    
        }
    }
    return 0; 
}
