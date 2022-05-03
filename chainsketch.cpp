#include "chainsketch.hpp"
#include <math.h> 
#include <random>
int seeds = 0;int ct=0;
int MAXINT=1000000000;
int chainlength=2; 
int tim=0;
int wr;
int k;
unsigned long bucket = 0;
unsigned long bucket1 = 0;
int keylen = 8;
long int newk;
int loc = -1; int ii = 0;
unsigned int gg = 999999999;
static std::random_device rd;
ChainSketch::ChainSketch(int depth, int width, int lgn) {
        
	Chain_.depth = depth;
	Chain_.width = width;
	Chain_.lgn = lgn;
	Chain_.counts = new SBucket *[depth*width];
	for (int i = 0; i < depth*width; i++) {
		Chain_.counts[i] = (SBucket*)calloc(1, sizeof(SBucket));
		memset(Chain_.counts[i], 0, sizeof(SBucket));
		Chain_.counts[i]->key[0] = '\0';
	}
        wr=chainlength;
	Chain_.hash = new unsigned long[depth];
	Chain_.scale = new unsigned long[depth];
	Chain_.hardner = new unsigned long[depth];
	char name[] = "ChainSketch";
	unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
	for (int i = 0; i < depth; i++) {
		Chain_.hash[i] = GenHashSeed(seed++);
	}
	for (int i = 0; i < depth; i++) {
		Chain_.scale[i] = GenHashSeed(seed++);
	}
	for (int i = 0; i < depth; i++) {
		Chain_.hardner[i] = GenHashSeed(seed++);
	}
}

ChainSketch::~ChainSketch() {
	for (int i = 0; i < Chain_.depth*Chain_.width; i++) {
		free(Chain_.counts[i]);
	}
	delete[] Chain_.hash;
	delete[] Chain_.scale;
	delete[] Chain_.hardner;
	delete[] Chain_.counts;
}



void ChainSketch::Update(unsigned char* key, val_tp val) {
	long min = 99999999,index; 
	ChainSketch::SBucket* sbucket;
	ChainSketch::SBucket* sbucket1;
       if(!chainlength)return;
	for (int i = 0; i < Chain_.depth; i++) {
		bucket = MurmurHash64A(key, keylen, Chain_.hardner[i]) % Chain_.width;
		index = i * Chain_.width + bucket;
		sbucket = Chain_.counts[index];
		if (sbucket->count == 0) {
			memcpy(sbucket->key, key, keylen);
			sbucket->count = val; 
			return;
		}
		if (memcmp(key, sbucket->key, keylen) == 0) {
			sbucket->count += val; 
			return;
		}
		if (sbucket->count < min)
		{
			min = sbucket->count;
			loc = index; bucket1 = bucket; ii = i;
		}
	
	}
	if(wr)sbucket = Chain_.counts[loc];
	k = rd() % (sbucket->count + 1) + 1;
	if (k <= val && chainlength > 0)
	{
		index = ii * Chain_.width + (bucket1 + 1) % Chain_.width;
		sbucket1 = Chain_.counts[index];
		if (memcmp(sbucket1->key, sbucket->key, keylen) == 0 && index != loc) {
			sbucket1->count += sbucket->count;
		}
		else if (memcmp(sbucket1->key, key, keylen) == 0 && index != loc)
		{
			int p = sbucket1->count;
			sbucket1->count = sbucket->count;
			memcpy(sbucket1->key, sbucket->key, keylen);
			sbucket->count += p;
		}
		else if (index != loc)
		{
			double round = 1;
			while ( round <= chainlength&&sbucket->count>sbucket1->count)
			{
				
				double va = sbucket->count * 1.0 / (sbucket1->count + sbucket->count);
				double po = pow(va, round);
				int ro = 1;
				while (ro * po < 10 && ro < MAXINT) {
					ro *= 10;
				}
				
				newk = rd() % ro + 1;
				if (newk <= int(ro * po)) {
					memcpy(sbucket1->key, sbucket->key, keylen);
					sbucket1->count = sbucket->count;
					break;

				}
				index = ii * Chain_.width + (index + 1) % Chain_.width;
				sbucket1 = Chain_.counts[index];
				round = round + 1;
				if (memcmp(sbucket1->key, key, keylen) == 0)
				{
					int p = sbucket1->count;
					sbucket1->count = 0;
					sbucket->count += p;
				}



			}

		}
		memcpy(sbucket->key, key, keylen);
		sbucket->count += val;


	}



}




mymap ChainSketch::Query2(val_tp thresh) 
{mymap ground;key_tp key;
	for (int i = 0; i < Chain_.width*Chain_.depth; i++) {
			memcpy(key.key, Chain_.counts[i]->key, Chain_.lgn / 8);
              	        ground[key] += Chain_.counts[i]->count;                     
	}

return ground;
}




void ChainSketch::Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> >&results) {
        mymap ground;key_tp key;

	for (int i = 0; i < Chain_.width*Chain_.depth; i++) {
			memcpy(key.key, Chain_.counts[i]->key, Chain_.lgn / 8);
              	        ground[key] += Chain_.counts[i]->count;                     
	}
for (auto it = ground.begin(); it != ground.end(); ){
	if (it->second < (int)thresh) 
	{
		it = ground.erase(it);
       	}
	else it++;
}

for (auto it = ground.begin(); it != ground.end();it++ ){
	                key_tp key;
			memcpy(key.key, it->first.key, Chain_.lgn / 8);
			std::pair<key_tp, val_tp> node;
			node.first = key;
			node.second = it->second;
			results.push_back(node);
}



	std::cout << "results.size = " << results.size() << std::endl;
}




void ChainSketch::Reset() {
	for (int i = 0; i < Chain_.depth*Chain_.width; i++) {
		Chain_.counts[i]->count = 0;
		memset(Chain_.counts[i]->key, 0, Chain_.lgn / 8);
	}
}

void ChainSketch::SetBucket(int row, int column, val_tp sum, long count, unsigned char* key) {
	int index = row * Chain_.width + column;
	Chain_.counts[index]->count = count;
	memcpy(Chain_.counts[index]->key, key, Chain_.lgn / 8);
}

ChainSketch::SBucket** ChainSketch::GetTable() {
	return Chain_.counts;
}







