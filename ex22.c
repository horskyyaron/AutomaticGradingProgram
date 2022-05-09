#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>

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


void gradeStudent(struct Paths* p, char *name, struct Student* s) {
    printf("%s\n", p->studentsDir);
    
    strcpy(s->name, name);
    s->grade = 89;
    strcpy(s->comment,"no comment");
}

void addGradeToResultsFile(struct Student *student){
    printf("added student results to file. student: %s\n", student->name);
}

void getPaths(struct Paths* p, char* confPath){
    getcwd(p->originRoot,sizeof(p->originRoot));
    strcpy(p->confPath, confPath);
    readConfFile(p->confPath, p->studentsDir, p->inputPath, p->expectedOutPath);
    getStudentDir(p->confPath, p->studentsDir,p->originRoot); 

}

int main(int argc, char **argv) {
    if(!isCorrectNumOfArgs(argc-1)){
        printf("wrong number of inputs\n");
        exit(-1);
    }

    struct Paths paths;
    DIR *dip;
    struct dirent *dit;
    struct Student student;

    //getting all the paths from the configuration file.
    getPaths(&paths, argv[1]);


    if((dip = opendir(paths.studentsDir)) == NULL){
        perror("Error in: opendir");
        exit(-1);
    }
    
    //scanning the student directory, ignoring non-directory files.
    while((dit = readdir(dip)) != NULL) {
        if(dit->d_type == DT_DIR && strcmp(dit->d_name,".") != 0 && strcmp(dit->d_name,"..") != 0){
            // checking student, results will be saved to student's struct.
            gradeStudent(&paths, dit->d_name, &student); 
            // adding student's results to results file,
            addGradeToResultsFile(&student);    
        }
    }
             






//    
//    printf("%s\n", originRoot);
//    printf("%s\n", confPath);
//    printf("%s\n", studentsDir);
//    printf("%s\n", inputPath);
//    printf("%s\n", expectedOutPath);
//

    //for every student S in the student dir:
    //  grade = checkGrade(s)
    //  add grade to csv file.








        
    
    return 0; 
}
