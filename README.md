# MultiThreadedWordCounter
Quickly count the words and valid words from a file using multi-threaded technology

I created this because I was curious if multi-threading a word counting program would cause it to run faster than a single threaded program.

With the way it is currently set up it takes about 15 seconds to process a 148MB file in multi-threaded and only 13 seconds for the same file, but in a single threaded version. Multithreading may be sped up more if each thread opened up a separate read into the file instead of loading the whole file into memory first and then splitting that into separate threads.

