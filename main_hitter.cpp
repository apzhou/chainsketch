#include "chainsketch.hpp"
#include "adaptor.hpp"
#include <unordered_map> 
#include<math.h>  
#include <bits/stdc++.h>       
#include <utility>    
#include <iomanip>     
#include "datatypes.hpp"  
#include "util.h"      
#include <random>      
   
int main(int argc, char* argv[]) {  
   
int memory_size=512;      
double aae=0; 
int bucket_num=memory_size*1024/(12*4);  //8 bytes flow key+4 Bytes count
 
        int sumerror=0;
	const char* filenames = "iptraces.txt";
	unsigned long long buf_size = 5000000000;  
	double thresh = 0.0007;
	int Chain_width =bucket_num; 
	int Chain_depth = 4; 
	std::vector<std::pair<key_tp, val_tp> > results;
	int numfile = 0;
	double precision = 0, recall = 0, error = 0;
	std::ifstream tracefiles(filenames);
	if (!tracefiles.is_open()) {
		std::cout << "Error opening file" << std::endl;
		return -1;
	}  
 
	for (std::string file; getline(tracefiles, file);) {
		Adaptor* adaptor = new Adaptor(file, buf_size);
		std::cout << "[Dataset]: " << file << std::endl;
		std::cout << "[Message] Finish read data." << std::endl;    
		adaptor->Reset(); 
		mymap ground;
		val_tp sum = 0;  
		tuple_t t;
		while (adaptor->GetNext(&t) == 1) {
		 	sum += 1;
			key_tp key;
			memcpy(key.key, &(t.key), LGN);
			if (ground.find(key) != ground.end()) {
				ground[key] += 1;
			}
			else { 
				ground[key] = 1;
			}   
		}
		std::cout << "[Message] Finish Insert hash table" << std::endl;
		val_tp threshold = thresh * sum;
		ChainSketch* Chain = new ChainSketch(Chain_depth, Chain_width, 8 * LGN);
		adaptor->Reset();  
		memset(&t, 0, sizeof(tuple_t));int gg=0; 
		while (adaptor->GetNext(&t) == 1) {
			Chain->Update((unsigned char*)&(t.key), 1);  
		} 
		results.clear();
		Chain->Query(threshold, results);		
		error = 0;int tp = 0, cnt = 0; aae=0;
		for (auto it = ground.begin(); it != ground.end(); it++) {    
		int flag = 0; 
			if (it->second > threshold) {
				cnt++;
				for (auto res = results.begin(); res != results.end(); res++) {
					if (memcmp(it->first.key, res->first.key, sizeof(res->first.key)) == 0) {
						
						double hh = res->second > it->second ? res->second - it->second : it->second - res->second;
						flag = 1;sumerror+=(int)hh;
						error = hh * 1.0 / it->second + error;
						aae+=hh;  
	  					tp++;		
					}
				} 


			}
		}  
			precision = tp * 1.0 / results.size();
			recall = tp * 1.0 / cnt;
			error = error / tp;
			aae=aae*1.0/tp;
			delete Chain;
			delete adaptor;
			numfile++;
			std::cout<<"\nChainSketch"<<std::endl;
 			std::cout<<"\nMemory    "<<memory_size<<std::endl;
        		std::cout<<"\nprecision "<<precision<<std::endl;
			std::cout<<"\nRecall    "<<recall<<std::endl;
			std::cout<<"\nARE       "<<error<<std::endl;

	}


}
