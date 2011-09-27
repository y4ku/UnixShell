#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

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
void printStructStat(int, bool);
void printStats();
void waitLoop();
void backgroundWait();
void quit();

struct rusage timings;
vector<commandTime *> statuses; 


int main ( void ){


cout << endl;
cout << "               Welcome to my Shell" << endl;
cout << endl;
cout << "By: J | A | K | U | B   M | I | S | T | E | R | K | A " << endl;
cout << endl;
cout << "              A CS385 statsh Project" << endl;
cout << endl;
cout << "ACCC ID: jmisterk " << endl;
cout << "Contact: jmister2@uic.edu || san.y4ku@gmail.com" << endl;
cout << "-----------------------------------------------------------------" << endl;
cout << endl;


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

    //Function to wait for processes that are in the background
    backgroundWait();
    
    //Mark for Command Line
	cout << "$> ";

    //Get Input from User
    cin.getline(input, 1024, '\n');

    int check = 0;

    //Check early if it is a background process
    if(backgroundCheck(input, Amp)){
        background = true;
    }

    //Parse user commands by Pipe ("|")
    char ** pipedCommands = vectorCommand(input, Pipe);
    
    count = 0;

    //count number of pipes to later use in loop
    while(pipedCommands[count] != NULL)
        count++;

    //set file descriptions back to 0 just in case
    int fileDesc0 = 0;
    int fileDesc1 = 0;
    int position = -1;

    //set std_int to -1 for information for forkPipe method
    int std_in = -1, std_out;
    
    //Loop through all commands separated by pipe
    //if no pipe that means only one command
    for(int i = 0; pipedCommands[i] != NULL; i++){

        //check for user entering stats and print stats
        if(strcmp(pipedCommands[i], "stats") == 0){
            printStats();
            break;
        }

        //check for exit
        if(strcmp(pipedCommands[i], "exit") == 0){
            quit();
            break;
        }

        //clear out file name just in case
        //distincion between both and use one for ">" and the other for "<" in case both are in command
        // 0 = Write, 1 = Read
        filename[0] = NULL;
        filename[1] = NULL;

        //get command from commands split by pipe
        const char * source = pipedCommands[i];

        //create a new struct in struct vector
        statuses.push_back(new commandTime);

        //allocate enough room for command
        statuses.back()->command = new char[strlen(source)+1];        

        //copy command to the struct
        strcpy(statuses.back()->command, source);

        //set the background bool
        statuses.back()->background = background;

        //check for file with write
        if(filename[0] = fileNameWrite(pipedCommands[i], i)){
            fileDesc0 = open(filename[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            fileWrite = true;
            position = i;
        }
        
        //check for file with read
        if(filename[1] = fileNameRead(pipedCommands[i], i)){
            fileDesc1 = open(filename[1], O_RDONLY);
            fileRead = true;
            position = i;
        }

        //create array for pipes
        int pipes[2]; 
        
        //check in which position
        //last position has regular std_out
        if(i < count-1){
            pipe(pipes);
            std_out = pipes[1];
        }
        else
            std_out = -1;

        //If there is a file to write or read, set specific std_out and/or std_in
        if(fileWrite){
            std_out = fileDesc0;
        }
        if(fileRead){
            std_in = fileDesc1;
        } 

        //break command separated at pipe into null separated command
        char ** commands = vectorCommand(pipedCommands[i], Space);     
                
        //Setting the pid for the stats struct and setting times to zero to check for finished processes later
        //Fork the Process
        statuses.back()->pid = forkPipe(commands, std_in, std_out, fileWrite, fileRead);
        statuses.back()->userTime = 0;
        statuses.back()->systemTime = 0;
        statuses.back()->uuserTime = 0;
        statuses.back()->usystemTime = 0;

        //Close if file write and/or read and reset file bools
        if(fileWrite){
            close(std_out);
            fileWrite = false;
        }
    
        if(fileRead){
            close(std_in);
            fileRead = false;
        }
        
        //close parents std_in and std_out
        //set std_in for next command
        close(std_in);
        close(std_out);
        std_in = pipes[0];

        delete commands;
    }

    //if background bool is false then go to regular wait loop
    if(!background){    
        waitLoop();
    }
    
    //if background bool is true go to wnohang wait loop
    if(background){
        backgroundWait();
        background = false;
    }
        
    delete pipedCommands;
}

return 0;

}

//Checks for "&" and sets it to NULL if found
bool backgroundCheck(char * input, char * delimeter){ 
    char * amp;
    if(amp = strchr(input, '&')){
        *amp = NULL;
        return true;
    }
    else 
        return false;
}

//Tokenizes input by delimeter
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

//Allocates memory for an array depning on how many delimeted commands
char ** getArray(vector<char*> charVector){
    char ** tempArray;

    tempArray = new char*[charVector.size() + 1];
    
    for(int i = 0; i < charVector.size(); i++){
        tempArray[i] = charVector.at(i);
      }     

    charVector.clear();
    
    return tempArray;
}

//Forks pipe or any other child/process
pid_t forkPipe(char ** commands, int std_in, int std_out, bool fileWrite, bool fileRead){
    if(pid_t child = fork())
        return child;

    //if first command/in the middle
    if(std_in != -1){
        dup2(std_in, 0);
        close(std_in);
    }
    
    //if not last command
    if(std_out != -1){
        dup2(std_out, 1);
        close(std_out);
    }

    //if file write
    if(fileWrite){
        dup2(std_out, 1);
        close(std_out);
    }
    
    //if file read
    if(fileRead){
        dup2(std_in, 0);
        close(std_in);
    }

    //Execute child
    execvp(commands[0], commands);
    cerr << "EXEC FAILED" << endl;
    exit(-1);

}

//Check for ">" and return filename while setting NULL
char * fileNameWrite(char * pipeCommands, int count){
    char * token;
    token = strtok(pipeCommands, ">" );
    if(token = strtok(NULL, "> ")){
        return token;
    }
    else return NULL;

}

//Check for "<" and return filename while setting NULL
char * fileNameRead(char * pipeCommands, int count){
    char * token;
    token = strtok(pipeCommands, "<" );
    if(token = strtok(NULL, "< ")){
        return token;
    }
    else return NULL;

}

//print stats
void printStats(){
    for(int i = 0; i < statuses.size(); i++){
       
        printStructStat(i, false);
      }     
}

//regular wait4 wait loop, will wait until child is finished
void waitLoop(){
    for(int i = 0; i < statuses.size(); i++){
        if(statuses.at(i)->background == false && statuses.at(i)->end == false){
            wait4(statuses.at(i)->pid, NULL, 0, &timings);

            statuses.at(i)->userTime = (long)timings.ru_utime.tv_sec;
            statuses.at(i)->systemTime = (long)timings.ru_stime.tv_sec;
            statuses.at(i)->uuserTime = (long)timings.ru_utime.tv_usec;
            statuses.at(i)->usystemTime = (long)timings.ru_stime.tv_usec;
            statuses.at(i)->end = true;

            //Setting userTime and systemTime to stats struct
            printStructStat(i, false);
        }
    }

}

//Loops through and looks for processes that have not ended yet by check end bool
//if child has not finished, lets it continue in background
void backgroundWait(){
    for(int i = 0; i < statuses.size(); i++){
        if(statuses.at(i)->end == false){
            if(wait4(statuses.at(i)->pid, NULL, WNOHANG, &timings) > 0){
            
                //Setting userTime and systemTime to stats struct
                statuses.at(i)->userTime = (long)timings.ru_utime.tv_sec;
                statuses.at(i)->systemTime = (long)timings.ru_stime.tv_sec;
                statuses.at(i)->uuserTime = (long)timings.ru_utime.tv_usec;
                statuses.at(i)->usystemTime = (long)timings.ru_stime.tv_usec;
                statuses.at(i)->end = true;
                //statuses.at(i)->background = false;

                //Print because background process has finished
                printStructStat(i, true);
            }
        }
    }
}

//Helper function to print stats for above functions
void printStructStat(int i, bool b){

    cout << endl;
    cout << "-------------------------------------" << endl;
    if(b){
        cout << "Background Process: " << statuses.at(i)->command;
        if(statuses.at(i)->background == true) 
            cout << "&";
        cout << " PID: " << statuses.at(i)->pid << " has finished." << endl;
    }
    else{
        cout << "Command: " <<  statuses.at(i)->command;
        cout << " PID: " << statuses.at(i)->pid << endl;
    }
    cout << "User Time (tv_sec): " << statuses.at(i)->userTime << " || ";
    cout << "System Time (tv_sec): " << statuses.at(i)->systemTime << endl;
    cout << "User Time (tv_usec): " << statuses.at(i)->uuserTime << " || ";
    cout << "System Time (tv_usec): " << statuses.at(i)->usystemTime << endl;
    cout << "-------------------------------------" << endl;
    cout << endl;


}

//print commands history/stats and prints shell stats
//de-allocates memory that was allocated
void quit(){

    printStats();

    getrusage(RUSAGE_SELF, &timings);

    cout << endl; 
    cout << "|***********************************|" << endl;        
    cout << "-------------------------------------" << endl;
    cout << "Shell Statistics : " << endl;
    cout << "User Time (tv_sec): " << timings.ru_utime.tv_sec << " || ";
    cout << "System Time (tv_sec): " << timings.ru_stime.tv_sec << endl;
    cout << "User Time (tv_usec): " << timings.ru_utime.tv_usec << " || ";
    cout << "System Time (tv_usec): " << timings.ru_stime.tv_usec << endl;
    cout << "-------------------------------------" << endl;
    cout << "|***********************************|" << endl;
    cout << endl;
    

    //Garbage Collection
    for(int i = 0; i < statuses.size(); i++){
        delete statuses.at(i)->command; 
    }
    for(int i = 0; i < statuses.size(); i++){
        delete statuses.at(i); 
    }
    exit(0);
}
