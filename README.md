## Introduction
---
**ChainSketch** uses the selective replacement strategy to mitigate the over-estimation issue. **ChainSketch** utilizes the hash chain and compact structure to improve memory efficiency. We implement the ChainSketch on OVS platform, P4-based testbed and large-scale simulations to process heavy hitter and heavy changer detection. The results of trace-driven tests show that, ChainSketch greatly improves the F1-Score compared with the state-of-the-art solutions.

We build the **ChainSketch** code based on the open-source code of [MV-Sketch](https://github.com/Grace-TL/MV-Sketch) which inspired us a lot. We thank them for their contributions.


---
## Files and Directories
- chainsketch.hpp: the implementation of chainsketch
- main\_hitter.cpp: example about heavy hitter detection
- main\_changer.cpp: example about heavy changer detection
- data: traces for test
- OVS: the implementation of chainsketch on OVS
- P4: the implementation of chainsketch on P4
---


## Compile and Run the examples
ChainSketch is implemented with C++. We show how to compile the examples on
Ubuntu with g++ and make.

### Requirements
- Ensure __g++__ and __make__ are installed.  Our experimental platform is
  equipped with Ubuntu 18.04.3, g++ 7.5.0.

- Ensure the necessary library libpcap is installed.
    - It can be installed in most Linux distributions (e.g., apt-get install
      libpcap-dev in Ubuntu).

- Prepare the pcap files.
    - We provide two small pcap files in "data" folder for testing.  
    - Specify the path of each pcap file in "iptraces.txt". 
    - Note that one pcap file is regarded as one epoch in our examples. To run
      the heavy changer example, you need to specify at least two pcap files.
      

### Compile
- Compile examples with make

```
    $ make main_hitter
    $ make main_changer
```
  

### Run
- Run the examples, and the program will output some statistics about the detection accuracy. 

```
$ ./main_hitter
$ ./main_changer
```

- Note that you can change the configuration of ChainSketch, e.g. number of rows and buckets in the example source code for testing.




