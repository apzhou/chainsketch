/******************************

Created by lyj
Created at 03/24/2021
Email: yijunli@csu.edu.cn

******************************/
#include <tofino/intrinsic_metadata.p4>
#include <tofino/constants.p4>
#include <tofino/primitives.p4>
#include "tofino/stateful_alu_blackbox.p4"

#define SKETCH_SIZE 256



/********************** declare header  ***************************************/
header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type udp_t {
    fields {
        srcPort:    16;
        dstPort:    16;
        pkt_length: 16;
        checksum:   16;
    }
}

header_type chain_hdr_t {
    fields{
        minimal_loc   : 32;
        minimal_count : 32;
    }
}


/********************** define header  ***************************************/
header ethernet_t ethernet;
header ipv4_t ipv4;
header udp_t udp;
header chain_hdr_t chain_hdr;


/********************** declare metadata*************************************/

header_type metadata_t {
    fields {
	
		flowkey           : 32; 
        do_sketch         :  1;
		minimal_loc       : 32;
        minimal_count     : 32;
        compare_flag      : 32;	
        key_1             : 32;
        count_1           : 32;
        loc_1             : 10;
        key_2             : 32;
        count_2           : 32;
        loc_2             : 10;
        key_3             : 32;
        count_3           : 32;
        loc_3             : 10;
        key_4             : 32;
        count_4           : 32;
        loc_4             : 10;
        m_result          : 32;
        m_rand          : 32;
        can_replace     : 32;
    }
}
/********************** define metadata **************************************/
metadata metadata_t mdata;


/********************** define calculation **********************************/
field_list ipv4_field_list {
        ipv4.version;
        ipv4.ihl;
        ipv4.diffserv;
        ipv4.totalLen;
        ipv4.identification;
        ipv4.flags;
        ipv4.fragOffset;
        ipv4.ttl;
        ipv4.protocol;
        ipv4.srcAddr;
        ipv4.dstAddr;
}

field_list_calculation ipv4_checksum {
    input {
        ipv4_field_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4.hdrChecksum  {
    update ipv4_checksum;
}


field_list hash_fields {
    ipv4.srcAddr;
	ipv4.dstAddr;
    ipv4.protocol;
	udp.srcPort;
	udp.dstPort;
}

field_list_calculation sketch_hash_r1 {
    input {
        hash_fields;
    }
    algorithm : crc_16;
    output_width : 16;
}

field_list_calculation sketch_hash_r2 {
    input {
        hash_fields;
    }
    algorithm : crc_16_buypass;
    output_width : 16;
}

field_list_calculation sketch_hash_r3 {
    input {
        hash_fields;
    }
    algorithm : crc_16_dds_110;
    output_width : 16;
}

field_list_calculation sketch_hash_r4 {
    input {
        hash_fields;
    }
    algorithm : crc_16_dect;
    output_width : 16;
}

/********************** parser **********************************/
parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x0800 : parse_ipv4;
        default: ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return select(ipv4.protocol) {
        17  : parse_udp;
        default : ingress;
    }
}

parser parse_chain_hdr {
    extract (chain_hdr);
    return ingress;
}

parser parse_udp {
    extract(udp);
    set_metadata(mdata.do_sketch, 1);
    return parse_chain_hdr;
}



/********************** Registers/Actions for sketch ***********************/

/**** array_1 register and table/action ****/
register bucket_array_1{
    width : 64; // <key,value>
    instance_count : SKETCH_SIZE;
    attributes : saturating;
}

blackbox stateful_alu update_bucket_array_1{
    reg : bucket_array_1;
    condition_lo : register_lo == 0; // Is this bucket empty?
    condition_hi : register_hi == mdata.flowkey; //Does this bucket store the flow?
    update_lo_1_predicate : condition_lo or condition_hi; // it is empty or hit
    update_lo_1_value : register_lo + 1;
    update_lo_2_predicate : not condition_lo and not condition_hi; // else read
    update_lo_2_value : register_lo;

    update_hi_1_predicate : condition_lo or condition_hi; // it is empty or hit
    update_hi_1_value : mdata.flowkey;
    update_hi_2_predicate : not condition_lo and not condition_hi; // else read 
    update_hi_2_value : register_hi;

    output_value : alu_hi; //read key to check whether successful update 
    output_dst : mdata.key_1;

    initial_register_lo_value : 0;
    initial_register_hi_value : 0;
}

action do_update_bucket_array_1(){
    update_bucket_array_1.execute_stateful_alu(mdata.loc_1);
}

@pragma stage 2
table array_1_table {
    actions { do_update_bucket_array_1; }
    default_action : do_update_bucket_array_1();
}


blackbox stateful_alu replace_bucket_array_1{
    reg : bucket_array_1;
    condition_lo : mdata.can_replace >= 0; // can replace?
    update_lo_1_predicate : condition_lo; // it is empty or hit
    update_lo_1_value : register_lo + 1;
    update_lo_2_predicate : not condition_lo; // else read
    update_lo_2_value : register_lo;

    update_hi_1_predicate : condition_lo; // it is empty or hit
    update_hi_1_value : mdata.flowkey;
    update_hi_2_predicate : not condition_lo; // else read 
    update_hi_2_value : register_hi;

    output_value : alu_hi; //read key to check whether successful update 
    output_dst : mdata.key_1;

    initial_register_lo_value : 0;
    initial_register_hi_value : 0;
}

action do_replace_bucket_array_1(){
    replace_bucket_array_1.execute_stateful_alu(mdata.loc_1);
}

table replace_array_1_table {
    actions { do_replace_bucket_array_1; }
    default_action : do_replace_bucket_array_1();
}
//this register is for record flow count,
//the value is the same as the lower 32 bit of register bucket_array_1
//it is only used to get count value
register bucket_array_1_data{
    width : 32; // value
    instance_count : SKETCH_SIZE;
    attributes : saturating;
}

blackbox stateful_alu read_bucket_array_1_data{
    reg : bucket_array_1_data;

    output_value : register_lo; //read counter vlaue
    output_dst : mdata.count_1;

    initial_register_lo_value : 0;
}

action do_read_bucket_array_1_data(){
    read_bucket_array_1_data.execute_stateful_alu(mdata.loc_1);
}

table array_1_data_table {
    actions { do_read_bucket_array_1_data; }
    default_action : do_read_bucket_array_1_data();
}

blackbox stateful_alu add_bucket_array_1_data{
    reg : bucket_array_1_data;

    update_lo_1_value : register_lo + 1;
}

action do_add_bucket_array_1_data(){
    add_bucket_array_1_data.execute_stateful_alu(mdata.loc_1);
}

table array_1_add_data_table {
    actions { do_add_bucket_array_1_data; }
    
    default_action : do_add_bucket_array_1_data();
}


blackbox stateful_alu replace_bucket_array_1_data{
    reg : bucket_array_1_data;

    update_lo_1_value : register_lo + 1;
}

action do_replace_bucket_array_1_data(){
    replace_bucket_array_1_data.execute_stateful_alu(mdata.loc_1);
}

@pragma stage 3
table array_1_replace_data_table {
    actions { do_replace_bucket_array_1_data; }
    
    default_action : do_replace_bucket_array_1_data();
}

/**** array_2 register and table/action ****/
register bucket_array_2{
    width : 64; // <key,value>
    instance_count : SKETCH_SIZE;
    attributes : saturating;
}

blackbox stateful_alu update_bucket_array_2{
    reg : bucket_array_2;
    condition_lo : register_lo == 0; // Is this bucket empty?
    condition_hi : register_hi == mdata.flowkey; //Does this bucket store the flow?
    update_lo_1_predicate : condition_lo or condition_hi; // it is empty or hit
    update_lo_1_value : register_lo + 1;
    update_lo_2_predicate : not condition_lo and not condition_hi; // else read
    update_lo_2_value : register_lo;

    update_hi_1_predicate : condition_lo or condition_hi; // it is empty or hit
    update_hi_1_value : mdata.flowkey;
    update_hi_2_predicate : not condition_lo and not condition_hi; // else read 
    update_hi_2_value : register_hi;

    output_value : alu_hi; //read key to check whether successful update 
    output_dst : mdata.key_2;

    initial_register_lo_value : 0;
    initial_register_hi_value : 0;
}

action do_update_bucket_array_2(){
    update_bucket_array_2.execute_stateful_alu(mdata.loc_2);
}

table array_2_table {
    actions { do_update_bucket_array_2; }
    
    default_action : do_update_bucket_array_2();
}


blackbox stateful_alu replace_bucket_array_2{
    reg : bucket_array_2;
    condition_lo : mdata.can_replace >= 0; // can replace?
    update_lo_1_predicate : condition_lo; // it is empty or hit
    update_lo_1_value : register_lo + 1;
    update_lo_2_predicate : not condition_lo; // else read
    update_lo_2_value : register_lo;

    update_hi_1_predicate : condition_lo; // it is empty or hit
    update_hi_1_value : mdata.flowkey;
    update_hi_2_predicate : not condition_lo; // else read 
    update_hi_2_value : register_hi;

    output_value : alu_hi; //read key to check whether successful update 
    output_dst : mdata.key_2;

    initial_register_lo_value : 0;
    initial_register_hi_value : 0;
}

action do_replace_bucket_array_2(){
    replace_bucket_array_2.execute_stateful_alu(mdata.loc_2);
}

@pragma stage 3
table replace_array_2_table {
    actions { do_replace_bucket_array_2; }
    default_action : do_replace_bucket_array_2();
}
//this register is for record flow count,
//the value is the same as the lower 32 bit of register bucket_array_2
//it is only used to get count value
register bucket_array_2_data{
    width : 32; // value
    instance_count : SKETCH_SIZE;
    attributes : saturating;
}

blackbox stateful_alu read_bucket_array_2_data{
    reg : bucket_array_2_data;

    output_value : register_lo; //read counter vlaue
    output_dst : mdata.count_2;

    initial_register_lo_value : 0;
}

action do_read_bucket_array_2_data(){
    read_bucket_array_2_data.execute_stateful_alu(mdata.loc_2);
}

table array_2_data_table {
    actions { do_read_bucket_array_2_data; }
    
    default_action : do_read_bucket_array_2_data();
}

blackbox stateful_alu add_bucket_array_2_data{
    reg : bucket_array_2_data;

    update_lo_1_value : register_lo + 1;
}

action do_add_bucket_array_2_data(){
    add_bucket_array_2_data.execute_stateful_alu(mdata.loc_2);
}

table array_2_add_data_table {
    actions { do_add_bucket_array_2_data; }
    
    default_action : do_add_bucket_array_2_data();
}

blackbox stateful_alu replace_bucket_array_2_data{
    reg : bucket_array_2_data;

    update_lo_1_value : register_lo + 1;
}

action do_replace_bucket_array_2_data(){
    replace_bucket_array_2_data.execute_stateful_alu(mdata.loc_2);
}

@pragma stage 4
table array_2_replace_data_table {
    actions { do_replace_bucket_array_2_data; }
    
    default_action : do_replace_bucket_array_2_data();
}

/**** array_3 register and table/action ****/
register bucket_array_3{
    width : 64; // <key,value>
    instance_count : SKETCH_SIZE;
    attributes : saturating;
}

blackbox stateful_alu update_bucket_array_3{
    reg : bucket_array_3;
    condition_lo : register_lo == 0; // Is this bucket empty?
    condition_hi : register_hi == mdata.flowkey; //Does this bucket store the flow?
    update_lo_1_predicate : condition_lo or condition_hi; // it is empty or hit
    update_lo_1_value : register_lo + 1;
    update_lo_2_predicate : not condition_lo and not condition_hi; // else read
    update_lo_2_value : register_lo;

    update_hi_1_predicate : condition_lo or condition_hi; // it is empty or hit
    update_hi_1_value : mdata.flowkey;
    update_hi_2_predicate : not condition_lo and not condition_hi; // else read 
    update_hi_2_value : register_hi;

    output_value : alu_hi; //read key to check whether successful update 
    output_dst : mdata.key_3;

    initial_register_lo_value : 0;
    initial_register_hi_value : 0;
}

action do_update_bucket_array_3(){
    update_bucket_array_3.execute_stateful_alu(mdata.loc_3);
}

table array_3_table {
    actions { do_update_bucket_array_3; }
    
    default_action : do_update_bucket_array_3();
}


blackbox stateful_alu replace_bucket_array_3{
    reg : bucket_array_3;
    condition_lo : mdata.can_replace >= 0; // can replace?
    update_lo_1_predicate : condition_lo; // it is empty or hit
    update_lo_1_value : register_lo + 1;
    update_lo_2_predicate : not condition_lo; // else read
    update_lo_2_value : register_lo;

    update_hi_1_predicate : condition_lo; // it is empty or hit
    update_hi_1_value : mdata.flowkey;
    update_hi_2_predicate : not condition_lo; // else read 
    update_hi_2_value : register_hi;

    output_value : alu_hi; //read key to check whether successful update 
    output_dst : mdata.key_3;

    initial_register_lo_value : 0;
    initial_register_hi_value : 0;
}

action do_replace_bucket_array_3(){
    replace_bucket_array_3.execute_stateful_alu(mdata.loc_3);
}
@pragma stage 4
table replace_array_3_table {
    actions { do_replace_bucket_array_3; }
    default_action : do_replace_bucket_array_3();
}

//this register is for record flow count,
//the value is the same as the lower 32 bit of register bucket_array_3
//it is only used to get count value
register bucket_array_3_data{
    width : 32; // value
    instance_count : SKETCH_SIZE;
    attributes : saturating;
}

blackbox stateful_alu read_bucket_array_3_data{
    reg : bucket_array_3_data;

    output_value : register_lo; //read counter vlaue
    output_dst : mdata.count_3;

    initial_register_lo_value : 0;
}

action do_read_bucket_array_3_data(){
    read_bucket_array_3_data.execute_stateful_alu(mdata.loc_3);
}

table array_3_data_table {
    actions { do_read_bucket_array_3_data; }
    
    default_action : do_read_bucket_array_3_data();
}


blackbox stateful_alu add_bucket_array_3_data{
    reg : bucket_array_3_data;

    update_lo_1_value : register_lo + 1;
}

action do_add_bucket_array_3_data(){
    add_bucket_array_3_data.execute_stateful_alu(mdata.loc_3);
}

table array_3_add_data_table {
    actions { do_add_bucket_array_3_data; }
    
    default_action : do_add_bucket_array_3_data();
}


blackbox stateful_alu replace_bucket_array_3_data{
    reg : bucket_array_3_data;

    update_lo_1_value : register_lo + 1;
}

action do_replace_bucket_array_3_data(){
    replace_bucket_array_3_data.execute_stateful_alu(mdata.loc_1);
}

@pragma stage 5
table array_3_replace_data_table {
    actions { do_replace_bucket_array_3_data; }
    
    default_action : do_replace_bucket_array_3_data();
}
/***********************  Action and Table *********************************/

action get_key(){
    modify_field(mdata.flowkey,ipv4.srcAddr);
}

table get_flow_key{
	actions {
 	    get_key;      
  	}
	default_action : get_key();
}

action assign1(){
    modify_field(mdata.minimal_count,mdata.count_1);
    modify_field(mdata.minimal_loc,1);
}

table assign1_tb{
	actions {
 	    assign1;      
  	}
	default_action : assign1();
}

action assign2(){
    min(mdata.minimal_count,mdata.minimal_count,2);
}

table assign2_tb{
	actions {
 	    assign2;      
  	}
	default_action : assign2();
}

action assign3(){
    min(mdata.minimal_count,mdata.minimal_count,3);
}

table assign3_tb{
	actions {
 	    assign3;      
  	}
	default_action : assign3();
}



action get_locs1(){
    modify_field_with_hash_based_offset(mdata.loc_1, 0, sketch_hash_r1, SKETCH_SIZE);
}

table get_locs1_tb{
    actions {
 	    get_locs1;      
  	}
	default_action : get_locs1();
}



action get_locs2(){
    modify_field_with_hash_based_offset(mdata.loc_2, 0, sketch_hash_r2, SKETCH_SIZE);
}

table get_locs2_tb{
    actions {
 	    get_locs2;      
  	}
	default_action : get_locs2();
}


action get_locs3(){
    modify_field_with_hash_based_offset(mdata.loc_3, 0, sketch_hash_r3, SKETCH_SIZE);
}

table get_locs3_tb{
    actions {
 	    get_locs3;      
  	}
	default_action : get_locs3();
}





action is_2_minimal(){
    subtract(mdata.compare_flag,mdata.minimal_count,mdata.count_2);
}

table is_2_minimal_tb{
    actions {
 	    is_2_minimal;      
  	}
	default_action : is_2_minimal();
}

action is_3_minimal(){
    subtract(mdata.compare_flag,mdata.minimal_count,mdata.count_3);
}

table is_3_minimal_tb{
    actions {
 	    is_3_minimal;      
  	}
	default_action : is_3_minimal();
}




action assignloc2(){
    modify_field(mdata.minimal_loc,mdata.loc_2);
}

table assignloc2_tb{
	actions {
 	    assignloc2;      
  	}
	default_action : assignloc2();
}

action assignloc3(){
    modify_field(mdata.minimal_loc,mdata.loc_3);
}

table assignloc3_tb{
	actions {
 	    assignloc3;      
  	}
	default_action : assignloc3();
}




action do_write_minimal(){
    modify_field(chain_hdr.minimal_loc,mdata.minimal_loc);
    modify_field(chain_hdr.minimal_count,mdata.minimal_count);
}

table do_write_minimal_tb{
    actions{
        do_write_minimal;
    }
    default_action:do_write_minimal();
}





action recirculate_to_68_action() {
    recirculate(68);
}


table recirculate_to_68_action_tb {
    actions {
        recirculate_to_68_action;
    }
    default_action: recirculate_to_68_action();
}

action get_replace(){
    modify_field(mdata.minimal_loc,chain_hdr.minimal_loc);
    modify_field(mdata.minimal_count,chain_hdr.minimal_count);
    modify_field_rng_uniform(mdata.m_rand,0,65535);
}

table get_replace_tb {
    actions {
        get_replace;
    }
    default_action: get_replace();
}

register num32{
    width : 32; // <key,value>
    instance_count : 1;
    attributes : saturating;
}

blackbox stateful_alu prog_64k_div_x {
    reg: num32;

    update_lo_2_value: math_unit;
    output_value: alu_lo;
    output_dst:   mdata.m_result;

    math_unit_input : register_lo;
    math_unit_exponent_shift : 0;
    math_unit_exponent_invert : True;
    math_unit_output_scale : 9;
    math_unit_lookup_table : 68 73 78 85 93 102 113 128 0  0  0  0  0   0   0   0;
}

action do_calculate(){
    prog_64k_div_x.execute_stateful_alu(0);
}

table do_calculate_tb{
    actions {
        do_calculate;
    }
    default_action: do_calculate();
}

action can_replace_action(){
    subtract(mdata.can_replace,mdata.m_rand,mdata.m_result);
}

table can_replace_action_tb{
    actions {
        can_replace_action;
    }
}

action set_egress(egress_spec) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_spec);
    add_to_field(ipv4.ttl, -1);
}

action _drop() {
    drop();
}

table ipv4_route {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
        // resubmit_action;
        set_egress;
        _drop;
    }
    size : 8192;
}
/********************************* Ingress *********************************/
control ingress{
    apply(get_flow_key);
    apply(get_locs1_tb);
    apply(get_locs2_tb);
    apply(get_locs3_tb);
    apply(ipv4_route);
    if (ig_intr_md.ingress_port != 68 ){ //new pkt
        if(mdata.do_sketch == 1){
            apply(array_1_table); // hash in row 1
            if(mdata.key_1 != mdata.flowkey){ //miss, go to next
                apply(array_1_data_table); //get count
                apply(array_2_table);
                if(mdata.key_2 != mdata.flowkey){//miss, go to next
                    apply(array_2_data_table); //get count
                    apply(array_3_table);
                    if(mdata.key_3 != mdata.flowkey){//miss, go to next
                        apply(array_3_data_table); //get count
                        apply(assign1_tb);
                        apply(assign2_tb);
                        apply(is_2_minimal_tb);
                        if(mdata.compare_flag==0){
                            apply(assignloc2_tb);
                        }
                        apply(assign3_tb);
                        apply(is_3_minimal_tb);
                        if(mdata.compare_flag==0){
                            apply(assignloc3_tb);
                        }
                        apply(do_write_minimal_tb);
                        apply(recirculate_to_68_action_tb);
                    }else{
                        apply(array_3_add_data_table);
                    }
                }else{
                    apply(array_2_add_data_table);
                }
            }else{
                apply(array_1_add_data_table);
            }
        }
    }else{ //second access
        apply(get_replace_tb);
        apply(do_calculate_tb);  
        apply(can_replace_action_tb);
        if(mdata.minimal_loc == 1){
            apply(replace_array_1_table);
            apply(array_1_replace_data_table);
        }else if (mdata.minimal_loc == 2){
            apply(replace_array_2_table);
            apply(array_2_replace_data_table);
        }else{
            apply(replace_array_3_table);
            apply(array_3_replace_data_table);
        }
    }
    
}

/********************************* Egress *********************************/
control egress {
}