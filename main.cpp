#include <iostream>
#include <string>
#include <fstream>
#include <ctype.h>
#include <thread>

using namespace std;

//program will generate the next GLIMIT codes. If LIMIT=-1, then it is limitless
const int CODE_SIZE=44, N_THREADS = 62, CONCURRENT_THREADS = 16; //N_THREADS is the total number of threads required, CONCURRENT_THREADS are usable threads
int MIN_LU = 35, MAX_LU = 39, CONSECUTIVE_LU=12; //MAX L/U in total, MIN L/U in total and consecutive L/U in a row
long long GLIMIT=100000000000;


string ucase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"; //valid uppercase letters U
string lcase = "abcdefghijklmnopqrstuvwxyz"; //valid lowercase letters L
string nums = "0123456789"; //valid numbers N
string code[N_THREADS]={
    "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z",
    "0","1","2","3","4","5","6","7","8","9"}; //valid code that will be generated and written in file. Each starting code is for each thread
int counts[N_THREADS][123]={0},step[N_THREADS][CODE_SIZE]={0},id[N_THREADS]={0}; //counts[] stores the number of times a letter has appeared and step is the state of the program.
bool ignoreFirst[N_THREADS]={0}; //Ignore first valid code found if the program starts from a state
bool pairs[N_THREADS][123][123]={0}, restrictions[CODE_SIZE][62]; //Keeps pairs found so far and restricted positions for certain elements
long long LIMIT[N_THREADS]={0};
ofstream codes_file[N_THREADS]; //File that will have all found codes so far.
thread threads[N_THREADS]; //Array of threads

//Main Functions used
inline bool generateCode(int& thread_id, int repeats, bool two, int sets,int LU_repeats, int max_LU, int min_pos);
inline bool tryEachChar(int& thread_id, string& letters, int& j, int& start, int& repeats, bool& two, int& sets,int& LU_repeats, int max_LU, int min_pos);

//Tries each character in string letters (ucase,lcase,nums) and adds it to code
//offset is used to store the correct state of step.
//j is the position of the character in the code (0-indexed).
//start is used when the program starts from a given state.
//repeats is how many times a consecutive N,L or U appeared last.
//two is true if the code ends with 2 same characters.
//sets is the number of sets (AA,BB,77,etc.) there are in the whole string.
inline bool tryEachChar(int& thread_id, string& letters, int offset, int& j, int& start, int& repeats, bool& two, int& sets, int& LU_repeats, int max_LU,int min_pos){
    int next_two,next_sets;
    for(unsigned i = start; i < letters.size(); ++i){
        //If the code still satisfies the constraints:
        //letter count is < 3 and no more than 2 same letters together and sets is less than 2 (if set is going to be added) and the last 2 letters haven't appeared in the string so far (AA,EE,7a,etc)
        if(!restrictions[j][offset+i] && counts[thread_id][letters[i]]<3 && (!two || code[thread_id].back()!=letters[i]) && (two || code[thread_id].back()!=letters[i] || sets<2) && (code[thread_id].size()<2 || !pairs[thread_id][code[thread_id].back()][letters[i]] )){
            //Setting if we have AA,ee,77, etc. and adding 1.
            if(code[thread_id].size() && code[thread_id].back() == letters[i]) next_two=1,next_sets=sets+1;
            else next_two=0,next_sets=sets;

            //Store pair of 2 letters.
            if(code[thread_id].size()) pairs[thread_id][code[thread_id].back()][letters[i]]=1;

            //Update information with new added character
            code[thread_id]+=letters[i];
            ++counts[thread_id][letters[i]];
            step[thread_id][j]=offset+i;
            if(generateCode(thread_id,repeats,next_two,next_sets,LU_repeats,max_LU,min_pos)) return 1;

            //Remove information of deleted character
            --counts[thread_id][letters[i]];
            code[thread_id].pop_back();

            if(code[thread_id].size()) pairs[thread_id][code[thread_id].back()][letters[i]]=0;
        }
    }
    return 0;

}

//Saves state to file
//if program reached end without LIMIT being 0, then it writes the last state
void saveState(int& thread_id, bool reached_end){
    ofstream state;
    state.open("state-"+to_string(thread_id)+".txt");
    for(int i = 0; i < CODE_SIZE; ++i){
        if(!reached_end) state<<step[thread_id][i]<<' ';
        else state<<61<<' ';
    }
    state<<id[thread_id]+1;
    state.close();
}


//Function that generates valid codes, it checks if it is possible to add U,L,N in that order.
//repeats is how many times a consecutive N,L or U appeared last
//two is true if the code ends with 2 same characters.
//sets is the number of sets (AA,BB,77,etc.) there are in the whole string.
inline bool generateCode(int& thread_id, int repeats, bool two, int sets, int LU_repeats, int max_LU, int min_pos){
    if(code[thread_id].size() == CODE_SIZE){ //Stops when code is CODE_SIZE of length
        if(!ignoreFirst[thread_id]){ //If it recovers from a state, it will ignore the first code because it has been visited
            codes_file[thread_id]<<code[thread_id]<<'\n'; //Saves code to file
            --LIMIT[thread_id];
            if(LIMIT[thread_id]==0){ //If it has saved LIMIT codes, it writes its state to state.txt
                saveState(thread_id,0);
            }
        }
        else ignoreFirst[thread_id]=0;
        return LIMIT[thread_id]==0;
    }

    int j=code[thread_id].size(),start=step[thread_id][j], next_repeat=repeats, next_LU_repeats=LU_repeats;
    bool is_last_alpha = !code[thread_id].empty() && isalpha(code[thread_id].back());

    //Iterates through all uppercase characters if consecutive U is less than 4
    if(start<26){
        if(max_LU && (code[thread_id].empty() || !isupper(code[thread_id].back()) || repeats<4)){
            //Setting repeat for UUUU
            if(code[thread_id].empty() || !isupper(code[thread_id].back())) next_repeat=1;
            else next_repeat=repeats+1;

            //Setting LU_repeat
            if(is_last_alpha) next_LU_repeats=LU_repeats+1;
            else next_LU_repeats=1;

            if(next_LU_repeats<=CONSECUTIVE_LU && tryEachChar(thread_id,ucase,0,j,start,next_repeat,two,sets,next_LU_repeats,max_LU-1,min_pos+1)) return 1;
        }
        start=0;
    }
    else start-=26;

    //Iterates through all lowercase characters if consecutive L is less then 4
    if(start<26){
        if(max_LU && (code[thread_id].empty() || !islower(code[thread_id].back()) || repeats<4)){

            //Setting repeat for LLLL
            if(code[thread_id].empty() || !islower(code[thread_id].back())) next_repeat=1;
            else next_repeat=repeats+1;

            //Setting LU_repeat
            if(is_last_alpha) next_LU_repeats=LU_repeats+1;
            else next_LU_repeats=1;

            if(next_LU_repeats<=CONSECUTIVE_LU && tryEachChar(thread_id,lcase,26,j,start,next_repeat,two,sets,next_LU_repeats,max_LU-1,min_pos+1)) return 1;
        }
        start=0;
    }
    else start-=26;

    //Iterates through all digits if consecutive N is less than 3
    if(j<min_pos && (code[thread_id].empty() || !isdigit(code[thread_id].back()) || repeats<3)){


        //Setting repeat for NNN
        if(code[thread_id].empty() || !isdigit(code[thread_id].back())) next_repeat=1;
        else next_repeat=repeats+1;

        //Since next character is a digit, then no LU repeats
        next_LU_repeats = 0;


        if(tryEachChar(thread_id,nums,52,j,start,next_repeat,two,sets,next_LU_repeats,max_LU,min_pos)) return 1;
    }
    step[thread_id][j]=0;
    return 0;
}

//Reads file state.txt and stores information in array step[]
//It returns True if there is a valid state.
inline bool readState(int& thread_id,string filename){
    ifstream state(filename);
    int i=0,n;
    while(state>>n){
        if(i >= CODE_SIZE) {
            id[thread_id]=n; //Last number is for file id
        }
        else{
            step[thread_id][i]=n;
            ++i;
        }

    }
    state.close();
    return i>0;
}

//Converts character to index
inline int getIdx(char letter){
    if(isupper(letter)) return letter-'A';
    else if(islower(letter)) return letter-'a'+26;
    return letter-'0'+52;
}

//Run thread with thread_id
//It is basically the single threaded version
void run(int thread_id){
    ignoreFirst[thread_id] = readState(thread_id,"state-"+to_string(thread_id)+".txt");
    codes_file[thread_id].open("codes-"+to_string(thread_id)+"-"+code[thread_id]+"-"+to_string(id[thread_id])+".txt");
    for(int i = 0; i < code[thread_id].size(); ++i){
        ++counts[thread_id][code[thread_id][i]];
    }

    bool is_first_alpha = isalpha(code[thread_id][0]);

    if(!restrictions[0][getIdx(code[thread_id][0])]) generateCode(thread_id,1,0,0,is_first_alpha,MAX_LU - is_first_alpha, CODE_SIZE-MIN_LU+is_first_alpha); //executes generator


    codes_file[thread_id].close();

    if(LIMIT[thread_id]>0){ //If it has saved LIMIT codes, it writes its state to state.txt
        saveState(thread_id,1);
    }
}



//Reads restrictions.txt file.
void readRestricted(string filename){
    ifstream rfile(filename);
    int i = 0;
    char letter;
    while(i < CODE_SIZE && rfile>>letter){
        if(letter=='|') ++i; // '|' indicates new line
        else{
            // Updates restrictions
            restrictions[i][getIdx(letter)]=1;
        }
    }
}

int main()
{
    readRestricted("restrictions.txt");

    //Runs only CONCURRENT_THREADS at the time and waits for them to finish.
    //thread_id is the id of the thread to run
    //j is the first thread_id that started to run from the previous batch

    int thread_id=0,j=0;
    while(thread_id<N_THREADS){
        j=thread_id;
        //Starting threads
        for(int i = 0; i < CONCURRENT_THREADS && thread_id < N_THREADS; ++i, ++thread_id){
            LIMIT[thread_id]=GLIMIT;
            threads[thread_id]=thread(run,thread_id);
        }
        //Waiting for threads to finish
        for(int i = 0; i < CONCURRENT_THREADS && j < N_THREADS; ++i,++j){
            threads[j].join();
        }
    }

    cout<<"COMPLETED"<<endl;

    return 0;
}
