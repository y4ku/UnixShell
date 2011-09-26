#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
//#include <wait.h>

using namespace std;

typedef struct commandTime {
   pid_t pid;
   char * command;
   long userTime;
   long systemTime;
   long uuserTime;
   long usystemTime;
   bool background;
   bool end;
};

char ** vectorCommand(char *, char *);
bool backgroundCheck(char *, char *);
char ** getArray(vector<char*>);
pid_t forkPipe(char ** commands, int std_in, int std_out, bool, bool);
char * fileNameWrite(char *, int count);
char * fileNameRead(char *, int count);
vector<commandTime *> statuses; 
void printStats();
void waitLoop();
void backgroundWait();
void quit();
struct rusage timings;

int main ( void ){

char input[1024];
int result = 0;
int count = 0;
pid_t * pipePid;
char * filename[2];

char * Amp = (char *)"&";
char * Pipe = (char *)"|";
char * Space = (char *)" ";
char * FileIn = (char *)"<";
char * FileOut = (char *)">";
bool fileWrite = false;
bool fileRead = false;
bool background = false;

while(result > -1){

    backgroundWait();
    
	cout << "$> ";

    cin.getline(input, 1024, '\n');

    int check = 0;

    if(backgroundCheck(input, Amp)){
        background = true;
    }

    char ** pipedCommands = vectorCommand(input, Pipe);
    
    count = 0;

    while(pipedCommands[count] != NULL)
        count++;

    int fileDesc0 = 0;
    int fileDesc1 = 0;
    int position = -1;

    pipePid = new pid_t[count];

    int std_in = -1, std_out;
    
    for(int i = 0; pipedCommands[i] != NULL; i++){

        if(strcmp(pipedCommands[i], "stats") == 0){
            printStats();
            break;
        }

        if(strcmp(pipedCommands[i], "exit") == 0){
            quit();
            break;
        }

        filename[0] = NULL;
        filename[1] = NULL;

        const char * source = pipedCommands[i];

        statuses.push_back(new commandTime);

        statuses.back()->command = new char[strlen(source)+1];        

        strcpy(statuses.back()->command, source);

        statuses.back()->background = background;

        if(filename[0] = fileNameWrite(pipedCommands[i], i)){
            fileDesc0 = open(filename[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            fileWrite = true;
            position = i;
        }
        
        if(filename[1] = fileNameRead(pipedCommands[i], i)){
            fileDesc1 = open(filename[1], O_RDONLY);
            fileRead = true;
            position = i;
        }


        int pipes[2]; 
        
        if(i < count-1){
            pipe(pipes);
            std_out = pipes[1];
        }
        else
            std_out = -1;

        if(fileWrite){
            std_out = fileDesc0;
        }
        if(fileRead){
            std_in = fileDesc1;
        } 

        char ** commands = vectorCommand(pipedCommands[i], Space);     
                
        pipePid[i] = forkPipe(commands, std_in, std_out, fileWrite, fileRead);

        //Setting the pid for the stats struct and setting times to zero to check for finished processes later
        statuses.back()->pid = pipePid[i];
        statuses.back()->userTime = 0;
        statuses.back()->systemTime = 0;
        statuses.back()->uuserTime = 0;
        statuses.back()->usystemTime = 0;


        if(fileWrite){
            close(std_out);
            fileWrite = false;
        }
    
        if(fileRead){
            close(std_in);
            fileRead = false;
        }
        

        close(std_in);
        close(std_out);
        std_in = pipes[0];

        delete commands;
    }
    
    if(!background){    
        waitLoop();
    }

    if(background){
        backgroundWait();
        background = false;
    }
        
    delete pipedCommands;
}

return 0;

}

bool backgroundCheck(char * input, char * delimeter){ 
    char * amp;
    if(amp = strchr(input, '&')){
        *amp = NULL;
        return true;
    }
    else 
        return false;
}


char ** vectorCommand(char * input, char * delimeter){
    char * token;
	vector<char*> charVector;
		
    
    token = strtok(input, delimeter);
    while(token != NULL){
            charVector.push_back(token);
            token = strtok(NULL, delimeter);
    }
    
    charVector.push_back(NULL);

    return getArray(charVector);    

}

char ** getArray(vector<char*> charVector){
    char ** tempArray;

    tempArray = new char*[charVector.size() + 1];
    
    for(int i = 0; i < charVector.size(); i++){
        tempArray[i] = charVector.at(i);
      }     

    charVector.clear();
    
    return tempArray;
}

pid_t forkPipe(char ** commands, int std_in, int std_out, bool fileWrite, bool fileRead){
    if(pid_t child = fork())
        return child;

    if(std_in != -1){
        dup2(std_in, 0);
        close(std_in);
    }
        
    if(std_out != -1){
        dup2(std_out, 1);
        close(std_out);
    }

    if(fileWrite){
        dup2(std_out, 1);
        close(std_out);
    }
    
    if(fileRead){
        dup2(std_in, 0);
        close(std_in);
    }


    execvp(commands[0], commands);
    cerr << "EXEC FAILED" << endl;
    exit(-1);

}

char * fileNameWrite(char * pipeCommands, int count){
    char * token;
    token = strtok(pipeCommands, ">" );
    if(token = strtok(NULL, "> ")){
        return token;
    }
    else return NULL;

}

char * fileNameRead(char * pipeCommands, int count){
    char * token;
    token = strtok(pipeCommands, "<" );
    if(token = strtok(NULL, "< ")){
        return token;
    }
    else return NULL;

}

void printStats(){
    for(int i = 0; i < statuses.size(); i++){
        cout << endl;
        cout << "-------------------------------------" << endl;
        cout << "Command: " <<  statuses.at(i)->command;
        if(statuses.at(i)->background == true) cout << "&";
        cout << " PID: " << statuses.at(i)->pid << endl;
        cout << "User Time (s): " << statuses.at(i)->userTime << " || ";
        cout << "System Time (s): " << statuses.at(i)->systemTime << endl;
        cout << "User Time (micro s): " << statuses.at(i)->uuserTime << " || ";
        cout << "System Time (micro s): " << statuses.at(i)->usystemTime << endl;
        cout << "-------------------------------------" << endl;
        cout << endl;
      }     
}

void waitLoop(){
    for(int i = 0; i < statuses.size(); i++){
        if(statuses.at(i)->background == false && statuses.at(i)->end == false){
            wait4(statuses.at(i)->pid, NULL, 0, &timings);
            
            //Setting userTime and systemTime to stats struct
            cout << endl;
            cout << "-------------------------------------" << endl;
            statuses.at(i)->userTime = timings.ru_utime.tv_sec;
            statuses.at(i)->systemTime = timings.ru_stime.tv_sec;
            statuses.at(i)->uuserTime = timings.ru_utime.tv_usec;
            statuses.at(i)->usystemTime = timings.ru_stime.tv_usec;
            statuses.at(i)->end = true;
            cout << "-------------------------------------" << endl;
            cout << endl;

        }
    }

}

void backgroundWait(){
    for(int i = 0; i < statuses.size(); i++){
        if(statuses.at(i)->end == false){
            if(wait4(statuses.at(i)->pid, NULL, WNOHANG, &timings) > 0){
            
                //Setting userTime and systemTime to stats struct
                statuses.at(i)->userTime = timings.ru_utime.tv_sec;
                statuses.at(i)->systemTime = timings.ru_stime.tv_sec;
                statuses.at(i)->uuserTime = timings.ru_utime.tv_usec;
                statuses.at(i)->usystemTime = timings.ru_stime.tv_usec;
                statuses.at(i)->end = true;
                //statuses.at(i)->background = false;

                //Print because background process has finished
                cout << endl;
                cout << "-------------------------------------" << endl;
                cout << "Background Process: " << statuses.at(i)->command;
                if(statuses.at(i)->background == true) cout << "&";
                cout << " PID: " << statuses.at(i)->pid << " has finished." << endl;
                cout << "User Time (s): " << statuses.at(i)->userTime << " || ";
                cout << "System Time (s): " << statuses.at(i)->systemTime << endl;
                cout << "User Time (micro s): " << statuses.at(i)->uuserTime << " || ";
                cout << "System Time (micro s): " << statuses.at(i)->usystemTime << endl;
                cout << "-------------------------------------" << endl;
                cout << endl;
            }
        }
    }
}

void quit(){
    //Garbage Collection
    for(int i = 0; i < statuses.size(); i++){
        delete statuses.at(i)->command; 
    }
    for(int i = 0; i < statuses.size(); i++){
        delete statuses.at(i); 
    }
    exit(0);
}
