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
#include <condition_variable>
#include <atomic>
#include <functional>
#include <algorithm>
#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <chrono> 

using namespace std;
using namespace std::chrono;

using StrMap = std::unordered_map<std::string, bool>;
using StrVec = std::vector<std::string>;
using StrQueue = std::queue<std::string>;


const std::string dictionaryFile = "english.txt";
const std::string instruct =
        "Please entire the desired file path or type exit to quit";
StrMap dictMap;
StrQueue jobs;  // hold the lines to be worked on by the programs
const size_t maxQSize = 100;

std::condition_variable queueBlocker;  // condition variable -- avoid spin locks
std::mutex queueMutex;  // mutex for synchronized operations on the queue

// total count of words and valid count
std::atomic<size_t> totalCount = ATOMIC_VAR_INIT(0);
std::atomic<size_t> validCount = ATOMIC_VAR_INIT(0);
std::atomic<bool> isReading = ATOMIC_VAR_INIT(true);  // bool true if still read

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
void updateWordCounts(std::string line) {
    line = cleanLine(line);
    std::istringstream ss(line);
    std::string word;
    // loop through each word and update counters
    while (ss >> word) {
        totalCount++;  // update total word count
        std::string lowered = toLowerCase(word);
        if (isValidWord(dictMap, lowered)) {
            // this is a valid word
            validCount++;  // increase valid word counts
        }
    }
}

/**
 * Print the number of words and number of valid words
 * @param counts
 * @return string of info
 */
std::string getStats() {
    return "Words: " + to_string(totalCount)
            + ", Valid Words: " + to_string(validCount);
}

/**
 * Read the file in and add each line to the job queue for the producer
 * @param fp The file path
 */
void reader(std::string& fp) {
    ifstream filest(fp);
    std::string line;

    // read each line and add to queue
    while (std::getline(filest, line)) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueBlocker.wait(lock, [] { return jobs.size() < maxQSize; });
        {
            jobs.push(line);  // add to end of queue
            // Wake-up a thread that is waiting for entry to be added
            queueBlocker.notify_one();
        }
    }

    isReading = !isReading;  // done reading
}

/**
 * Thread-safe Check if the job queue is empty
 * @return 
 */
bool jobEmptyLoopChecker() {
    bool flag;
    queueMutex.lock();
    flag = jobs.empty();
    queueMutex.unlock();
    return flag;
}

/**
 * Count all words in a file
 * Functions a the consumer in the producer-consumer threading model
 * This consumer reads from the queue all of the produced input
 */
void counter() {
    // loop while producer is working or queue is not empty
    while (isReading || !jobEmptyLoopChecker()) {
        std::string line;  // declare variable before lock
        {
            // add the lock
            std::unique_lock<std::mutex> lock(queueMutex);
            queueBlocker.wait(lock, [] {
                return !jobs.empty(); });
            line = jobs.front();
            jobs.pop();

            // unlock
            queueBlocker.notify_one();
        }

        // process the data
        updateWordCounts(line);
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
    // spawn a producer thread that reads in the files
    threadGroup.push_back(std::thread(reader, std::ref(fp)));

    // begin spawning threads -- using up the remaining threads
    for (int thr = 1; thr < cores; thr++) {
        threadGroup.push_back(std::thread(counter));
    }

    // join all threads
    for (auto& t : threadGroup) {
        t.join();
    }

    std::cout << getStats() << std::endl;
}

/*
 * Main launch pad of program
 */
int main(int argc, char** argv) {
    // Load the dictionary
    dictionary(dictionaryFile);

    // determine how many cores can be used
    size_t cores = std::thread::hardware_concurrency();
    std::cout << cores << " concurrent threads are supported.\n";

    std::string resp;
    std::cout << instruct << std::endl;
    while (std::cin >> resp) {
        if (resp == "exit" || resp == "quit") {
            return 0;
        }
        
        // Get starting timepoint -- for timing
        auto start = high_resolution_clock::now();
    
        // launch threads to process
        std::vector<std::thread> threadGroup;
        threadLauncher(threadGroup, cores, resp);
        
        // Get ending timepoint -- for timing
        auto stop = high_resolution_clock::now(); 
        
        // Get duration. Substart timepoints to  
        // get durarion. To cast it to proper unit 
        // use duration cast method 
        auto duration = duration_cast<microseconds>(stop - start); 
  
        cout << "Time taken by function: "
            << duration.count() << " microseconds" << endl; 

        std::cout << std::endl << instruct << std::endl;  // prompt for next
        
        // Reset global variables for next process
        isReading = true;  // set as producer as reading again for next process
        totalCount = 0;
        validCount = 0;
    }

    return 0;
}
