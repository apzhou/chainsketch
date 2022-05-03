#ifndef CHANGER_H
#define CHANGER_H

#include "chainsketch.hpp"
#include "datatypes.hpp"

template <class S>
class HeavyChanger {

public:
    HeavyChanger(int depth, int width, int lgn);

    ~HeavyChanger();

    void Update(unsigned char* key, val_tp val);

    void Query(val_tp thresh, myvector& result);

    void Reset();

    S* GetCurSketch();

    S* GetOldSketch();

private:
    S* old_sk;

    S* cur_sk;

    int lgn_;
};

template <class S>
HeavyChanger<S>::HeavyChanger(int depth, int width, int lgn) {
    old_sk = new S(depth, width, lgn);
    cur_sk = new S(depth, width, lgn);
    lgn_ = lgn;
}


template <class S>
HeavyChanger<S>::~HeavyChanger() {
    delete old_sk;
    delete cur_sk;
}

template <class S>
void HeavyChanger<S>::Update(unsigned char* key, val_tp val) {
    cur_sk->Update(key, val);
}

template <class S>
void HeavyChanger<S>::Query(val_tp thresh, myvector& results) {
mymap gr1=cur_sk->Query2(thresh);
mymap gr2=old_sk->Query2(thresh);
key_tp key;	
for (auto it = gr1.begin(); it != gr1.end();it++ )
	{
		memcpy(key.key, it->first.key, 8);
		gr2[key]=gr2[key]>it->second?gr2[key]-it->second:it->second-gr2[key];
	}
for (auto it = gr2.begin(); it != gr2.end();it++ )
	{               
		if(it->second > thresh)
			{
				memcpy(key.key, it->first.key, lgn_/8);
				std::pair<key_tp, val_tp> node;
				node.first = key;
				node.second = it->second;
				results.push_back(node);

			}

	}

}


template <class S>
void HeavyChanger<S>::Reset() {
    old_sk->Reset();
    S* temp = old_sk;
    old_sk = cur_sk;
    cur_sk = temp;
}

template <class S>
S* HeavyChanger<S>::GetCurSketch() {
    return cur_sk;
}

template <class S>
S* HeavyChanger<S>::GetOldSketch() {
    return old_sk;
}

#endif
