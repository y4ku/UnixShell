#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <vector>

using namespace std;

int forkChild(vector<char**> *, struct rusage);
char ** vectorCommand(char *, char *);
char ** getArray(vector<char*> *);

int main ( void ){

struct rusage timings;
char input[1024];
int result = 0;
int count = 0;
char ** commandArray[];

char * Pipe = "|";
char * Space = " ";

while(result > -1){

    char * ctoken = " ";
    
	cout << "$> ";

    cin.getline(input, 1024, '\n');

    char ** pipedCommands = vectorCommand(input, Pipe);

    count = 0;

    while(pipedCommands[count] != NULL)
            count++;

    cout << "Number of commands: " << count << endl;  

    commandArray = new char ** [count];

    for(int i = 0; pipedCommands[i] != NULL; i++){      

        commandArray[i] = vectorCommand(pipedCommands[i], Space);
                

        /*if(count == 1){
        char ** commands = vectorCommand(pipedCommands[i], Space);
        result = forkChild(commands, timings); 
        delete commands;
        }
        
        else{
        char ** commands = vectorCommand(pipedCommands[i], Space);
        result = forkChild(commands, timings);
        
        delete commands;
        }*/

    }
        
    //forkChild(&commandVector, timings);

    delete pipedCommands;
}

return 0;

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

    return getArray(&charVector);    

}

char ** getArray(vector<char*> * charVector){
    char ** tempArray;

    tempArray = new char*[charVector->size() + 1];
    
    for(int i = 0; i < charVector->size(); i++){
        tempArray[i] = charVector->at(i);
      }     

    charVector->clear();
    
    return tempArray;
}

int forkChild(vector<char**> * commandVector, struct rusage timings){
    
    char ** tempArray;

    for(int i = 0; i < commandVector->size() + 1; i++){
    tempArray = commandVector->at(i);

    pid_t pid;
	
	pid = fork();

	if(pid < 0){
		cout << "Fork Failed" << endl;
		return -1;
	}

	else if (pid == 0){
		execvp(tempArray[0], tempArray);
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
    }
}	

