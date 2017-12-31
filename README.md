# cgParamtuner
Param tuner using brutal tester

Stuff to change is on top.

Compile: 
g++ -std=c++0x -o paramTuner main.cpp


Lets review the different availables "options" (TODO: parse them from command line)


Your CSB binary path :
std::string CSB_BINARY("/path/to/csb_binary/csb");


Brutal tester folder :
std::string BRUTAL_DIR("/path/to/brutal_tester/");


The executable will call the CSB binary passing the params in the command line.
ie :

./csb_binary 10000 12 42 5 0.5



const int MAX_SCORES = 5;


float SCORE_BASES[MAX_SCORES] {
    10000, 12, 42, 5, 0.5
};


Those are the actual parameters you want to tune, and fight against.


float SCORE_RANGES[MAX_SCORES] {
   1000, 10, 1, 2, 0.5
};


The SCORE_RANGES are the deviation from the actual SCORE_BASE value. 
In the example, a mutation on the first value will be the range [9000-11000].



float MIN_SCORE_FOR_UPGRADE = 55.0;

const int GAMES_POP = 100;

const int GAMES_VAL = 200;


MIN_SCORE_FOR_UPGRADE, is the minimum score required for a solution to be considered "good enough" to be set as the new SCORE_BASES.


GAMES_POP is the amount of games that are played for each solution.


If a solution's score is >= MIN_SCORE_FOR_UPGRADE, after GAMES_POP games, then another GAMES_VAL are played, to lower the "false positives".


If the new score is >= MIN_SCORE_FOR_UPGRADE, then only it upgrades for SCORE_BASES.

