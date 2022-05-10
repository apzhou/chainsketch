#include "heavy_changer.hpp"
#include "adaptor.hpp"
#include <unordered_map>
#include <utility>
#include "util.h" 
#include "datatypes.hpp"
#include <iomanip>   
 
typedef std::unordered_map<key_tp, val_tp*, key_tp_hash, key_tp_eq> groundmap;
  
int main(int argc, char* argv[]) { 
int memory_size=512;  
int bucket_num=memory_size*1024/(12*4);  //8 bytes flow key+4 Bytes count
    const char* filenames = "iptraces.txt"; 
    unsigned long long buf_size = 5000000000;
    double thresh = 0.0008; 
    int chain_width = bucket_num; 
    int chain_depth = 4; 
    int numfile = 0;
    double precision=0, recall=0, error=0;
    std::string file;
    std::ifstream tracefiles(filenames);
    if (!tracefiles.is_open()) {
        std::cout << "Error opening file" << std::endl;
        return -1;
    }
    groundmap groundtmp;
    mymap ground;
    tuple_t t;
    memset(&t, 0, sizeof(tuple_t));
    val_tp diffsum = 0; 
    HeavyChanger<ChainSketch>* heavychangerchain = new HeavyChanger<ChainSketch>(chain_depth, chain_width, LGN*8);
    for (std::string file; getline(tracefiles, file);) {
        Adaptor* adaptor =  new Adaptor(file, buf_size);
        std::cout << "[Dataset]: " << file << std::endl;
        memset(&t, 0, sizeof(tuple_t));
        adaptor->Reset();
        for (auto it = groundtmp.begin(); it != groundtmp.end(); it++) {
            it->second[1] = it->second[0];
            it->second[0] = 0;
        }
        while(adaptor->GetNext(&t) == 1) {
            key_tp key;
            memcpy(key.key, &(t.key), LGN);
            if (groundtmp.find(key) != groundtmp.end()) {
                groundtmp[key][0] += 1;
            } else {
                val_tp* valtuple = new val_tp[2]();
                groundtmp[key] = valtuple;
                groundtmp[key][0] += 1;
            }
        }
        if (numfile != 0) {
            ground.clear();
            val_tp oldval, newval, diff; 
            diffsum = 0;
            for (auto it = groundtmp.begin(); it != groundtmp.end(); it++) {
                oldval = it->second[0];
                newval = it->second[1];
                diff = newval > oldval ? newval - oldval : oldval - newval;
                diffsum += diff;
            }
            for (auto it = groundtmp.begin(); it != groundtmp.end(); it++) {
                oldval = it->second[0];
                newval = it->second[1];
                diff = newval > oldval ? newval - oldval : oldval - newval;
                if (diff > thresh*(diffsum)){
                    ground[it->first] = diff;
                }
            }
        }
        adaptor->Reset(); 
        heavychangerchain->Reset();
        ChainSketch* cursk = (ChainSketch*)heavychangerchain->GetCurSketch();
        while(adaptor->GetNext(&t) == 1) {
            cursk->Update((unsigned char*)&(t.key), 1);
        }
        if (numfile != 0) {  
            std::vector<std::pair<key_tp, val_tp> > results;
            results.clear();
            val_tp threshold = thresh*diffsum;
            heavychangerchain->Query(threshold, results);
            int tp = 0, cnt = 0; 
            error = 0; 
            for (auto it = ground.begin(); it != ground.end(); it++) {
                if (it->second > threshold) {
                    cnt++;
                    for (auto res = results.begin(); res != results.end(); res++) {
                        if (memcmp(it->first.key, res->first.key, sizeof(res->first.key)) == 0) {
double hh=res->second > it->second?res->second - it->second: it->second-res->second;
error += hh*1.0/it->second;
                            tp++;
                        }
                    }
                }
            }
            precision = tp*1.0/results.size();
            recall = tp*1.0/cnt;
            error = error/tp;
        }
	numfile++;
        delete adaptor;
    }

    for (auto it = groundtmp.begin(); it != groundtmp.end(); it++) {
        delete [] it->second;
    }
    delete heavychangerchain; 
	std::cout<<"\nChainSketch"<<std::endl;
 	std::cout<<"\nMemory    "<<memory_size<<std::endl;
        std::cout<<"\nprecision "<<precision<<std::endl;
	std::cout<<"\nRecall    "<<recall<<std::endl;
	std::cout<<"\nARE       "<<error<<std::endl;

}
