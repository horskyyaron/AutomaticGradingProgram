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

#define MAX_PATH 150
#define MAX_CONFIG_FILE_LINE 150
#define MAX_STU_NAME_SIZE 100
#define MAX_COMMENT_SIZE 100

struct Student {
    char name[MAX_STU_NAME_SIZE];   
    int grade;
    char comment[MAX_COMMENT_SIZE];
};

struct Paths {
    char confPath[MAX_PATH];
    char studentsDir[MAX_PATH];
    char inputPath[MAX_PATH];
    char expectedOutPath[MAX_PATH];
    char originRoot[MAX_PATH];
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

int getStudentDir(char* confPath, char* studentsDir, char* originRoot){
    int len = strlen(confPath);
    int i = len - 1;
    char c = confPath[i];
    char confFileDir[MAX_CONFIG_FILE_LINE];
    while(c!='/'){
        i--;
        c = confPath[i];
        if(i == 0)
            break;
    }
    //copying the path to the conf file directory.
    strncpy(confFileDir,confPath,i);
    confFileDir[i] = '\0';
    if(xChdir(confFileDir)==0){
        if(xChdir(studentsDir)==0){
            getcwd(studentsDir,MAX_CONFIG_FILE_LINE);
        }
    }
    if(xChdir(originRoot)==0){
        return 0;
    }
    return -1;
}

void readConfFile(char* path,char* students, char* in, char* out) {
    int fd = xOpen(path, O_RDONLY);
    xReadLine(fd, students);  
    xReadLine(fd, in); 
    xReadLine(fd, out);
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

int compileCFile(char *dir, char* fileName){
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
    argv[3] = "student_exec.out";
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

int runStudentProgram(){
    int status;
    char *argv[2];
    argv[0] = "./student_exec.out";
    argv[1] = NULL;

    //running student's program.
    int pid = (int) fork();
    if (pid == 0) {
        //redirecting output and input file descriptors.
        int input = open("./input.txt",O_RDWR);
        int output = open("student_out.txt",O_CREAT | O_WRONLY, 0777);
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
        argv[0] = "./comp.out";
        argv[1] = "student_out.txt";
        argv[2] = "correct_output.txt";
        argv[3] = NULL;
        execvp(argv[0],argv);
        exit(-1);
    }
    waitpid(pid,&res,0);


    //removing student output after comparing with expected output file.
    if(remove("./student_out.txt") < 0) {
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
    if(compileCFile(sDirPathBuffer, cFile)==0){
        //run and produce student output file.
        runStudentProgram();
        //grade.
        calcGrade(compareOutput(p), s);
    } else {
        s->grade = 10;
        strcpy(s->comment,"COMPILATION_ERROR");
    }
}



void addGradeToResultsFile(struct Student *student){
}

void getPaths(struct Paths* p, char* confPath){
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
    readConfFile(p->confPath, p->studentsDir, p->inputPath, p->expectedOutPath);
    getStudentDir(p->confPath, p->studentsDir,p->originRoot); 

}


int main(int argc, char **argv) {
    if(!isCorrectNumOfArgs(argc-1)){
        printf("wrong number of inputs\n");
        exit(-1);
    }

    struct Paths paths;
    DIR *studentsDirPtr;
    struct dirent *dirItem;
    struct Student student;

    //getting all the paths from the configuration file.
    getPaths(&paths, argv[1]);


    if((studentsDirPtr = opendir(paths.studentsDir)) == NULL){
        perror("Error in: opendir");
        exit(-1);
    }
    
    //scanning the student directory, ignoring non-directory files.
    while((dirItem = readdir(studentsDirPtr)) != NULL) {
        if(dirItem->d_type == DT_DIR && strcmp(dirItem->d_name,".") != 0 && strcmp(dirItem->d_name,"..") != 0){
            // checking student, results will be saved to student's struct.
            strcpy(student.name, dirItem->d_name);
            gradeStudent(&paths, &student); 
            printf("%s,%d,%s\n",student.name,student.grade,student.comment);
            // adding student's results to results file,
            addGradeToResultsFile(&student);    
        }
    }
    return 0; 
}
