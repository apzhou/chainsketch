#ifndef CHAINSKETCH_H
#define CHAINSKETCH_H
#include <vector>
#include <unordered_set>
#include <utility>
#include <cstring>
#include <cmath>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "datatypes.hpp"
extern "C"
{
#include "hash.h"
#include "util.h"
}



class ChainSketch {

    typedef struct SBUCKET_type {

        int count;

        unsigned char key[LGN];

    } SBucket;

    struct Chain_type {

        //Counter table
        SBucket **counts;

        //Outer sketch depth and width
        int depth;
        int width;

        //# key word bits
        int lgn;

        unsigned long *hash, *scale, *hardner;
    };


    public:
	ChainSketch(int depth, int width, int lgn);

    ~ChainSketch();

    	void Update(unsigned char* key, val_tp value);

    
   	void Query(val_tp thresh, myvector& results);
   
	mymap Query2(val_tp thresh);


    	void Reset();


    private:

    	void SetBucket(int row, int column, val_tp sum, long count, unsigned char* key);

	ChainSketch::SBucket** GetTable();

	Chain_type Chain_;
};

#endif
