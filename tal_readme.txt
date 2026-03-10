1. run code:
./pattern_finder --path inputs_si/ --alive 0.7
./pattern_finder --path ./single_graph --single-graph --min-density 0.7 
compile - 
cd /home/cohent59/PROJECT_RUN_PATTERN/pattern_finder
g++ -std=c++17 -O2 -g -Wall -Wextra -Wpedantic -Iinclude -I/usr/local/anaconda3/include -o pattern_finder src/*.cpp -lboost_program_options

2.Clone devora project:
    git clone https://github.com/tsents/Graph-Search.git
    cd Graph-Search
    go build -o subgraph_isomorphism
    mkdir -p dat
    command to run ./subgraph_isomorphism G folder

    ./subgraph_isomorphism G/TRY_S.json G/SMALL.json

    test devora code
    ./subgraph_isomorphism ../test/Devora_code/TRY_S.json ../test/Devora_code/SMALL.json

    
3. Clone data from yael:
    git clone https://github.com/yaelgin2/new-graph-measures/tree/test_motifs_solution/local_tests
    cd local_tests
4. 