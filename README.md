# MultiThreadedWordCounter
Quickly count the words and valid words from a file using multi-threaded technology in C++

I created this because I was curious if multi-threading a word counting program would cause it to run faster than a single threaded program and if so, how much.

With the way it is currently set up it takes about 1.23 seconds (1233 milliseconds) in "Debug Mode" with 4 cores available to process a 4.349MB file with multithreading. This is done utilizing the Producer-Consumer model of multithreading where in this case one producer thread reads the file while multiple (as many as cores are available) consumer threads process the read in lines. Utilizing Monitors, the threads to not have to busy wait when waiting on suitable operating conditions. This means the program may take slightly longer, but is much more efficient.

When running the 4.349MB (without user prompting in Debug Mode with 4 cores) the program gives these stats: 2.83user 0.68system 0:01.35elapsed 259%CPU

