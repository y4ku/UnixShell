#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

typedef struct commandTime {
   pid_t pid;
   char * command;
   long userTime;
   long systemTime;
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
void quit();
struct rusage timings;


int main ( void ){

char input[1024];
int result = 0;
int count = 0;
pid_t * pipePid;
char * filename[2];

bool background;

char * Amp = "&";
char * Pipe = "|";
char * Space = " ";
char * FileIn = "<";
char * FileOut = ">";
bool fileWrite = false;
bool fileRead = false;

while(result > -1){

    char * ctoken = " ";
    
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

        //char * pcom;
        const char * source = pipedCommands[i];

        statuses.push_back(new commandTime);

        //statuses.back()->command = new char[sizeof(pcom)];
        statuses.back()->command = new char[sizeof(source)];        

        strcpy(statuses.back()->command, source);

        statuses.back()->background = false;

        cout << "From Vector: " << statuses.back()->command << endl;

        if(filename[0] = fileNameWrite(pipedCommands[i], i)){
            fileDesc0 = open(filename[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            fileWrite = true;
            position = i;
            //cout << "found write" << endl;
        }
        
        if(filename[1] = fileNameRead(pipedCommands[i], i)){
            fileDesc1 = open(filename[1], O_RDONLY);
            fileRead = true;
            position = i;
            //cout << "found read" << endl;
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
            //cout << "Setting stdout to fileDesc" << endl;
        }
        if(fileRead){
            std_in = fileDesc1;
        } 

        char ** commands = vectorCommand(pipedCommands[i], Space);     
                
        pipePid[i] = forkPipe(commands, std_in, std_out, fileWrite, fileRead);

        //Setting the pid for the stats struct
        statuses.back()->pid = pipePid[i];

        cout << statuses.back()->pid << endl;

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
    
    //if(!background)    
        waitLoop();

    if(background){
        cout << "BACKGROUND" << endl;
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

    if(std_in != -1){ //&& std_in != 0){
        dup2(std_in, 0);
        close(std_in);
    }
        
    if(std_out != -1){ //&& std_in != 1){
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
        cout << "Command: " <<  statuses.at(i)->command << " PID: " << statuses.at(i)->pid << endl;
        cout << "User Time: " << statuses.at(i)->userTime << " || ";
        cout << "System Time : " << statuses.at(i)->systemTime << endl;
      }     
}

void waitLoop(){
    for(int i = 0; i < statuses.size(); i++){
        if(statuses.at(i)->background == false && statuses.at(i)->end == false){
            wait4(statuses.at(i)->pid, NULL, NULL, &timings);
            //Setting userTime and systemTime to stats struct
            statuses.at(i)->userTime = timings.ru_stime.tv_usec;
            statuses.at(i)->systemTime = timings.ru_utime.tv_usec;
            statuses.at(i)->end = true;
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


/*int forkChild(char ** arr, struct rusage timings){
    pid_t pid;
	
	pid = fork();

	if(pid < 0){
		cout << "Fork Failed" << endl;
		return -1;
	}

	else if (pid == 0){
		execvp(arr[0], arr);
		cout << "Unknown command" << endl;
    	return 0;
	}

	else{
        //wait(NULL);
		wait4(pid, NULL, NULL, &timings);
		cout << "Child Complete --- " << endl;
        cout << "System Time: " << timings.ru_stime.tv_sec << " " << timings.ru_stime.tv_usec << endl;
        cout << "User Time:  " << timings.ru_utime.tv_sec << " " << timings.ru_utime.tv_usec << endl;

		return 0;
	}
}	*/
