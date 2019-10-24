/*
 * Copyright(C) 2019 zampaze@miamioh.edu
 */

/* 
 * File:   zampaze_hw6.cpp
 * Author: zampaze
 *
 * Created on October 16, 2019, 6:33 PM
 */


#include <cstdlib>
#include <cctype>
#include <thread>
#include <mutex>
#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>

using namespace std;

using StrMap = std::unordered_map<std::string, bool>;
using StrVec = std::vector<std::string>;

const std::string dictionaryFile = "english.txt";
const std::string instruct = 
    "Please entire the desired file path or type exit to quit";
StrMap dictMap;

std::mutex mut;

/**
 * Load the dictionary
 * @param file file to read valid words from
 * @return vector of valid words
 */
StrMap dictionary(const std::string& file = dictionaryFile) {
    std::ifstream dictif(file);
    std::string word;
    while (dictif >> word) {
        dictMap[word] = false;
    }

    return dictMap;
}

/**
 * Determine if a word is valid based off the supplied dictionary
 * @param dict
 * @param word
 * @return true if the word is a valid word
 */
bool isValidWord(const StrMap& dict, const std::string& word) {
    auto it = dict.find(word);
//    std::cout << dict.size();

    if (it == dict.end()) {
        // not valid
        return false;
    }

    return true;
}

/**
 * Clean a line by removing punctuation
 * @param line
 * @return cleaned line
 */
std::string cleanLine(const std::string& line) {
    // Remove punctuation in line
    std::string str = line;
    std::replace_if(str.begin(), str.end(), ::ispunct, ' ');
    return str;
}

/**
 * Make a word lowercase
 * @param word
 * @return lowercased word
 */
std::string toLowerCase(const std::string& str) {
    // Convert the word to lower case to check against the dictionary.
    std::string word = str;
    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    return word;
}

/**
 * Update count on if a word is valid or not
 * @param dict
 * @param line
 * @param counts
 */
void updateWordCounts(const StrMap& dict, std::string& line,
        int counts[]) {
    line = cleanLine(line);
    std::istringstream ss(line);
    std::string word;
    // loop through each word and update counters
    while (ss >> word) {
        mut.lock();  // lock for increasing count operation
        counts[0]++;  // update total word count
        mut.unlock();  // done with increasing count operation
        std::string lowered = toLowerCase(word);
        if (isValidWord(dict, lowered)) {
            // this is a valid word
            mut.lock();  // lock for increasing count operation
            counts[1]++;  // increase valid word counts
            mut.unlock();  // done with increasing count operation
        }
    }
}

/**
 * Print the number of words and number of valid words
 * @param counts
 * @return string of info
 */
std::string getStats(int counts[]) {
    return "Words: " + to_string(counts[0])
            + ", Valid Words: " + to_string(counts[1]);
}


/**
 * Run the specified thread
 * @param list
 * @param result
 * @param startIdx
 * @param count
 */
void thrMain(const StrVec fullFile, int counts[], 
        const int startIdx, const int count) {
    int end = startIdx + count;
    for (int i = startIdx; i < end; i++) {
        std::string line = fullFile[i];
//        std::cout << dictMap.size();
        updateWordCounts(dictMap, line, counts);
    }
}

/**
 * Launch all the threads needed
 * @param urlVec
 * @param results
 * @param threadGroup
 * @param threads
 */
void threadLauncher(std::vector<std::thread>& threadGroup, int cores,
        std::string& fp) {
    // figure out how to divide up
    int size;
    
    ifstream filest(fp);
    StrVec fullFile;
    std::string line;
    while (std::getline(filest, line)) {
        fullFile.push_back(line);
    }
    
    int ffs = fullFile.size();
    size = ffs / cores;
    
    int counts[2] = {0, 0};  // hold word counts: 0 = total, 1 = valid

    // begin making threads
    for (int thr = 0, start = 0; thr < cores; thr++, start += size) {
        if (cores != 1 && thr + 1 == cores && ffs == thr)
            size++;
        threadGroup.push_back(std::thread(thrMain, 
                std::ref(fullFile), std::ref(counts), start, size));
    }
    
    // join all threads
    for (auto& t : threadGroup) {
        t.join();
    }
    
    std::cout << getStats(counts);
}

/*
 * Main launch pad of program
 */
int main(int argc, char** argv) {
    // Load the dictionary
    dictMap = dictionary(dictionaryFile);

    // determine how many cores can be used
    size_t cores = std::thread::hardware_concurrency() / 2;  // only physical
    std::cout << cores << " concurrent threads are supported.\n";
    
    std::string resp;
    std::cout << instruct << std::endl;
    while (std::cin >> resp) {
        if (resp == "exit" || resp == "quit") {
            return 0;
        }

        // get file path
        std::string fp = resp;
        
        // launch threads to process
        std::vector<std::thread> threadGroup;
        threadLauncher(threadGroup, cores, fp);
        
        std::cout << std::endl << instruct << std::endl;  // prompt for next
    }

    return 0;
}
