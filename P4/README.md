## P4
---
Here is the hardware prototype of **Chainsketch** in the Barefoot Tofino switch.


---
## Files and Directories
- chainsketch.p4: the implementation of chainsketch
- control/setup.py: the control plane program to set up the switch 
- measurement.py: a simpel control plane program to read the buckets in **Chainsketch**
---


## Compile and Run

### Requirements
- **Chainsketch** is tested on 
    - Tofino SDK (version after 8.9.1) on the switch.
      

### Compile and Install
- Compile **Chainsketch** with P4 Studio. Assuming that the SDE environment variable points to the SDE folder.

```
    $ cd ~/bf-sde-8.9.1/pkgsrc/p4-build
    $ ./configure --prefix=$SDE/install --with-tofino P4_NAME=chainsketch P4_PATH=/PATH/TO/chainsketch.p4 --enable-thrift
    $ make 
    $ make install
    $ cd ~/bf-sde-8.9.1/pkgsrc/p4-examples
    $ ./configure --prefix=$SDE/install 
    $ make
    $ make install
```
  

### Run

```
$ SDE/run_switchd.sh -p chainsketch

```

### Setup 
- Set the switch (port and forward table).
```
$SDE/run_p4_tests.sh -t /PATH/TO/control/ -p chainsketch --target hw
```
### Read Registers
- Read the buckets in **Chainsketch**.
```
$SDE/tools/run_pd_rpc.py -p chainsketch /PATH/TO/measurepercent.py
```



