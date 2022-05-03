#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      /* opeXnffccf */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <errno.h>
#include "util.h"
#include "tuple.h"
#include "ringbuffer.h"
#include <string>


#include "chainsketch.hpp"
#include <unordered_map>
#include <utility>
#include <iomanip>
#include "datatypes.hpp"
#include "util.h"
#include <cstring>
int tjw;

void coin_sketch_update(tuple_t1 t,ChainSketch* mv){
   
         mv->Update((unsigned char*)&(t.key), 1);

}

// The number of ringbuffer
// **Must** be (# pmd threads + 1)
#define MAX_RINGBUFFER_NUM 3

static inline char* ip2a(uint32_t ip, char* addr) {
    sprintf(addr, "%d.%d.%d.%d", (ip & 0xff), ((ip >> 8) & 0xff), ((ip >> 16) &
            0xff), ((ip >> 24) & 0xff));
    return addr;
}

void print_tuple(FILE* f, tuple_t1* t) {
    char ip1[30], ip2[30];

    fprintf(f, "%s(%u) <-> %s(%u) %u %ld\n",
            ip2a(t->key.src_ip, ip1), t->key.src_port,
            ip2a(t->key.dst_ip, ip2), t->key.dst_port,
            t->key.proto, t->size
            );
}

int counter = 0;

void handler(int sig) {
	printf("%d\n", counter);
	counter = 0;
	alarm(1);
}

int main(int argc, char *argv[]) {
    std::cin>>tjw;
    tuple_t1 t;
    double throughput=0;
    long long tot_cnt = 0;
    //HC_TYPE * hc = new HC_TYPE();
    int turn = 0;

        int qqq=tjw*1024/(16*4);
         int mv_width = qqq;
         int mv_depth = 4;
         ChainSketch* mv1 = new ChainSketch(mv_depth, mv_width, 8*LGN);
         uint64_t t1=0, t2=0;t1 = now_us();





    LOG_MSG("Initialize the ringbuffer.\n");

	ringbuffer_t * rbs[MAX_RINGBUFFER_NUM];

	for (int i = 0; i < MAX_RINGBUFFER_NUM; ++i) {
		char name[30];
		sprintf(name, "/rb_%d", i);
		rbs[i] = connect_ringbuffer_shm(name, sizeof(tuple_t1));
		printf("%x\n", rbs[i]);
	}
 
	printf("connected.\n");	fflush(stdout);

	int idx = 0;

    // print number of pkts received per 5 sec
	signal(SIGALRM, handler);
	alarm(5);
        int clock=0;
	while (1) {

        if (t.flag == TUPLE_TERM) {
            break;
        } 
        else {
		    while (read_ringbuffer(rbs[(idx) % MAX_RINGBUFFER_NUM], &t) < 0) {clock++;
                idx = (idx + 1) % MAX_RINGBUFFER_NUM;
            }
            counter++;
            // Insert to sketch here
                coin_sketch_update(t,mv1);


        







        }
    } 

        t2 = now_us();
        throughput = clock/(double)(t2-t1)*1000000000;
        std::cout << "time = " << (double)(t2-t1)*1000000000 << std::endl;


	return 0;
}
