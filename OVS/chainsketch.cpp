#include "chainsketch.hpp"
#include <math.h>
int seeds = 0;int ct=0;
ChainSketch::ChainSketch(int depth, int width, int lgn) {

	coin_.depth = depth;
	coin_.width = width;
	coin_.lgn = lgn;
	coin_.sum = 0;
	coin_.counts = new SBucket *[depth*width];
	for (int i = 0; i < depth*width; i++) {
		coin_.counts[i] = (SBucket*)calloc(1, sizeof(SBucket));
		memset(coin_.counts[i], 0, sizeof(SBucket));
		coin_.counts[i]->key[0] = '\0';
	}

	coin_.hash = new unsigned long[depth];
	coin_.scale = new unsigned long[depth];
	coin_.hardner = new unsigned long[depth];
	char name[] = "ChainSketch";
	unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
	for (int i = 0; i < depth; i++) {
		coin_.hash[i] = GenHashSeed(seed++);
	}
	for (int i = 0; i < depth; i++) {
		coin_.scale[i] = GenHashSeed(seed++);
	}
	for (int i = 0; i < depth; i++) {
		coin_.hardner[i] = GenHashSeed(seed++);
	}
}

ChainSketch::~ChainSketch() {
	for (int i = 0; i < coin_.depth*coin_.width; i++) {
		free(coin_.counts[i]);
	}
	delete[] coin_.hash;
	delete[] coin_.scale;
	delete[] coin_.hardner;
	delete[] coin_.counts;
}






void ChainSketch::Update(unsigned char* key, val_tp val) {
	unsigned long bucket = 0;
	unsigned long bucket1 = 0;
	int keylen = coin_.lgn / 8;coin_.sum += 1;
	ChainSketch::SBucket *sbucket;
	int flag = 0;
	long min = 99999999; int loc = -1; int loc1 = -1; int k; int index; int ii = 0;
          
	for (int i = 0; i < coin_.depth; i++) {
		bucket = MurmurHash64A(key, keylen, coin_.hardner[i]) % coin_.width;
		index = i * coin_.width + bucket;
		sbucket = coin_.counts[index];
		if (sbucket->key[0] == '\0'&&sbucket->count==0) {
			memcpy(sbucket->key, key, keylen);
			flag = 1;
			sbucket->count = 1;break;
		}
		else if (memcmp(key, sbucket->key, keylen) == 0) {
			flag = 1;
			sbucket->count += 1;break;
		}
               if (sbucket->count < min)
		{
			min = sbucket->count;
			loc = index; bucket1 = bucket; ii = i;
		}
	}
	if (flag == 0 && loc >= 0)
	{
		sbucket = coin_.counts[loc];

		unsigned int gg = 999999999;
		srand(seeds); seeds = seeds + (time(NULL) + 1) % gg;
                seeds = seeds + 1; 
		k = rand() % (sbucket->count + 1) + 1;
		if (k > sbucket->count)
		{	
                ChainSketch::SBucket *sbucket1;
				min = 99999999; loc1 = -1; flag = 0;
					index = ii * coin_.width + (bucket1+1)% coin_.width;
					sbucket1 = coin_.counts[index];
				if (memcmp(sbucket1->key, sbucket->key, keylen) == 0&&index!=loc) {
						
						sbucket1->count += sbucket->count; 
					}
					else if( memcmp(sbucket1->key, key, keylen) == 0&&index!=loc)
					{
					int p = sbucket1->count;
					sbucket1->count = sbucket->count;
					memcpy(sbucket1->key, sbucket->key, keylen);
					sbucket->count += p; 
					}
					
else if(index!=loc)
					{ 

long int newk,newa,newb;
double round=1;

while(sbucket->count>(sbucket1->count+sbucket->count)&&index!=loc&&round<=3){
//printf("haha round %.2f index %d %d sbucket->count %d sbucket1->count %d\n",round,index,loc,sbucket->count,sbucket1->count);
                                         srand(seeds+8); seeds = seeds + (time(NULL) + 1) % gg;
int po1=pow(sbucket->count*1.0,round);
int po2=pow(sbucket1->count*1.0+sbucket->count*1.0,round);
		newk = rand() % (po1 + po2) + 1;
			if(newk<=sbucket->count){//printf("oops\n");
memcpy(sbucket1->key, sbucket->key, keylen); 
						sbucket1->count = sbucket->count;
round=200;
break;

}
index = ii * coin_.width + (index+1)% coin_.width;
sbucket1 = coin_.counts[index];
round++;	
if( memcmp(sbucket1->key, key, keylen) == 0&&index!=loc)
					{printf("asdad\n");
					int p = sbucket1->count;
					sbucket1->count = 0;
					sbucket->count += p; 
					}		
}
					}
			memcpy(sbucket->key, key, keylen);
			sbucket->count += 1;
		}
	}
}




void ChainSketch::Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> >&results) {
	int qqq = 0;
	myset res;
	for (int i = 0; i < coin_.width*coin_.depth; i++) {
                     
		if (coin_.counts[i]->count > (int)thresh) {
			key_tp reskey; qqq++;
			memcpy(reskey.key, coin_.counts[i]->key, coin_.lgn / 8);
			res.insert(reskey);

		}

	}int r=0;
	for (auto it = res.begin(); it != res.end(); it++,r++) {//printf("%d \n",r);
		val_tp resval = 99999999; val_tp max = 0;
		for (int j = 0; j < coin_.depth; j++) {
			unsigned long bucket = MurmurHash64A((*it).key, coin_.lgn / 8, coin_.hardner[j]) % coin_.width;
			unsigned long index = j * coin_.width + bucket;
			if (memcmp(coin_.counts[index]->key, (*it).key, coin_.lgn / 8) == 0) {
				max += coin_.counts[index]->count;			
			}
index = j * coin_.width + (bucket + 1) % coin_.width;
for(int tt=0;tt<3;tt++){

			if (memcmp(coin_.counts[index]->key, (*it).key, coin_.lgn / 8) == 0)
			{
				max += coin_.counts[index]->count;
			}

index = j * coin_.width + (index + 1) % coin_.width;
}
			
		}
		if (max != 0)resval = max;

			key_tp key;
			memcpy(key.key, (*it).key, coin_.lgn / 8);
			std::pair<key_tp, val_tp> node;
			node.first = key;
			node.second = resval;
			results.push_back(node);
	}
	std::cout << "results.size = " << results.size() << std::endl;
}



val_tp ChainSketch::PointQuery(unsigned char* key) {
	return Low_estimate(key);
}

val_tp ChainSketch::Low_estimate(unsigned char* key) {


val_tp ret = 0, max = 0, min = 999999999;
	for (int i = 0; i < coin_.depth; i++) {
		unsigned long bucket = MurmurHash64A(key, coin_.lgn / 8, coin_.hardner[i]) % coin_.width;

		unsigned long index = i * coin_.width + bucket;
		if (memcmp(coin_.counts[index]->key, key, coin_.lgn / 8) == 0)
		{
			max += coin_.counts[index]->count;
		}
		index = i * coin_.width + (bucket + 1) % coin_.width;
		if (memcmp(key, coin_.counts[i]->key, coin_.lgn / 8) == 0)
		{
			max += coin_.counts[index]->count;
		}

	}
	return max;


}






val_tp ChainSketch::Up_estimate(unsigned char* key) {

val_tp ret = 0, max = 0, min = 999999999;
	for (int i = 0; i < coin_.depth; i++) {
		unsigned long bucket = MurmurHash64A(key, coin_.lgn / 8, coin_.hardner[i]) % coin_.width;

		unsigned long index = i * coin_.width + bucket;
		if (memcmp(coin_.counts[index]->key, key, coin_.lgn / 8) == 0)
		{
			max += coin_.counts[index]->count;
		}
		if (coin_.counts[index]->count < min)min = coin_.counts[index]->count;
		index = i * coin_.width + (bucket + 1) % coin_.width;
		if (memcmp(key, coin_.counts[i]->key, coin_.lgn / 8) == 0)
		{
			max += coin_.counts[index]->count;
		}

	}
	if (max)return max;
	return min;

}





val_tp ChainSketch::GetCount() {
	return coin_.sum;
}



void ChainSketch::Reset() {
	coin_.sum = 0;
	for (int i = 0; i < coin_.depth*coin_.width; i++) {
		coin_.counts[i]->count = 0;
		memset(coin_.counts[i]->key, 0, coin_.lgn / 8);
	}
}

void ChainSketch::SetBucket(int row, int column, val_tp sum, long count, unsigned char* key) {
	int index = row * coin_.width + column;
	coin_.counts[index]->count = count;
	memcpy(coin_.counts[index]->key, key, coin_.lgn / 8);
}

ChainSketch::SBucket** ChainSketch::GetTable() {
	return coin_.counts;
}

/*void ChainSketch::MergeAll(ChainSketch** coin_arr, int size) {
  long countarr[size];
  val_tp sumarr[size];
  unsigned char* keyarr[size];
  long est[size];

  for (int d = 0; d < coin_.depth; d++) {
	for (int w = 0; w < coin_.width; w++) {
	  val_tp total = 0;
	  for (int i = 0; i < size; i++) {
		//find the majority and its estimate
		ChainSketch* cursk = coin_arr[i];
		ChainSketch::SBucket** table = (cursk)->GetTable();
		int index = d*coin_.width+w;
		countarr[i] = table[index]->count;
		keyarr[i] = table[index]->key;
		total += sumarr[i];
	  }

	  int pointer = 0;
	  long counttmp = 0;
	  for (int i = 0; i  < size; i++) {
		est[i] = 0;
		for (int j = 0; j < size; j++) {
		  if (memcmp(keyarr[i], keyarr[j], coin_.lgn/8) == 0) {
			est[i] += (sumarr[j] + countarr[j])/2;
		  } else {
			est[i] += (sumarr[j] - countarr[j])/2;
		  }
		}
		if (est[i] > est[pointer]) pointer = i;
	  }
	  counttmp = 2 * est[pointer] - total;
	  counttmp = counttmp > 0 ? counttmp : 0;
	  if (counttmp == 0) {
		  SetBucket(d, w, 2*est[pointer], counttmp, keyarr[pointer]);
	  } else {
		  SetBucket(d, w, total, counttmp, keyarr[pointer]);
	  }
	}
  }
}
*/





