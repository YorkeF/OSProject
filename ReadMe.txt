# OSProject
Files needed:
---
monitor.h
monitor_helpers.cpp
monitor_init_and_queue.h
sharedmemory.h
monitor_transactions.cpp
driver.cpp
transactions.txt

To Compile:
`g++ -o driver driver.cpp -pthread`

To Run:
`./driver transactions.txt`

To adjust input transactions, edit the transactions.txt file, or use another input.txt file like `./driver input.txt`

Files tested on CSX0, CSX1, and CSX2