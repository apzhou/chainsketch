import pd_base_tests
import pdb
import time
import sys

from collections import OrderedDict
from ptf import config
from ptf.testutils import *
from ptf.thriftutils import *

import os

from pal_rpc.ttypes import *

from chainsketch.p4_pd_rpc.ttypes import *
from mirror_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *

from tm_api_rpc.ttypes import *

from pkt_pd_rpc.ttypes import *

CSFQ_PORT = 8888
UDP_DSTPORT = 8888

port_ip_dic = {188: 0x0a010001 , 184: 0x0a010002 , 180: 0x0a010003 , 176: 0x0a010004 ,
               172: 0x0a010005 , 168: 0x0a010006 , 164: 0x0a010007 , 160: 0x0a010008 ,
               156: 0x0a010009 , 152: 0x0a01000a , 148: 0x0a01000b , 144: 0x0a01000c}

dev_id = 0
if test_param_get("arch") == "Tofino":
  print "TYPE Tofino"
  sys.stdout.flush()
  MIR_SESS_COUNT = 1024
  MAX_SID_NORM = 1015
  MAX_SID_COAL = 1023
  BASE_SID_NORM = 1
  BASE_SID_COAL = 1016
elif test_param_get("arch") == "Tofino2":
  print "TYPE Tofino2"
  sys.stdout.flush()
  MIR_SESS_COUNT = 256
  MAX_SID_NORM = 255
  MAX_SID_COAL = 255
  BASE_SID_NORM = 0
  BASE_SID_COAL = 0
else:
  print "TYPE NONE"
  print test_param_get("arch")
  sys.stdout.flush()

ports = [188]

mirror_ids = []

dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))

def setup_random(seed_val=0):
    if 0 == seed_val:
        seed_val = int(time.time())
    print
    print "Seed is:", seed_val
    sys.stdout.flush()
    random.seed(seed_val)

def make_port(pipe, local_port):
    assert(pipe >= 0 and pipe < 4)
    assert(local_port >= 0 and local_port < 72)
    return (pipe << 7) | local_port

def port_to_pipe(port):
    local_port = port & 0x7F
    assert(local_port < 72)
    pipe = (port >> 7) & 0x3
    assert(port == ((pipe << 7) | local_port))
    return pipe

def port_to_pipe_local_port(port):
    return port & 0x7F

swports = []
swports_by_pipe = {}
for device, port, ifname in config["interfaces"]:
    if port == 0: continue
    if port == 64: continue
    pipe = port_to_pipe(port)
    print device, port, pipe, ifname
    print int(test_param_get('num_pipes'))
    if pipe not in swports_by_pipe:
        swports_by_pipe[pipe] = []
    if pipe in range(int(test_param_get('num_pipes'))):
        swports.append(port)
        swports.sort()
        swports_by_pipe[pipe].append(port)
        swports_by_pipe[pipe].sort()

if swports == []:
    for pipe in range(int(test_param_get('num_pipes'))):
        for port in range(1):
            swports.append( make_port(pipe,port) )
cpu_port = 64
#cpu_port = 192
print "Using ports:", swports
sys.stdout.flush()

def mirror_session(mir_type, mir_dir, sid, egr_port=0, egr_port_v=False,
                   egr_port_queue=0, packet_color=0, mcast_grp_a=0,
                   mcast_grp_a_v=False, mcast_grp_b=0, mcast_grp_b_v=False,
                   max_pkt_len=1024, level1_mcast_hash=0, level2_mcast_hash=0,
                   mcast_l1_xid=0, mcast_l2_xid=0, mcast_rid=0, cos=0, c2c=0, extract_len=0, timeout=0,
                   int_hdr=[], hdr_len=0):
    return MirrorSessionInfo_t(mir_type,
                             mir_dir,
                             sid,
                             egr_port,
                             egr_port_v,
                             egr_port_queue,
                             packet_color,
                             mcast_grp_a,
                             mcast_grp_a_v,
                             mcast_grp_b,
                             mcast_grp_b_v,
                             max_pkt_len,
                             level1_mcast_hash,
                             level2_mcast_hash,
                             mcast_l1_xid,
                             mcast_l2_xid,
                             mcast_rid,
                             cos,
                             c2c,
                             extract_len,
                             timeout,
                             int_hdr,
                             hdr_len)

class CSFQ_HDR(Packet):
    name = "CSFQ_HDR"
    fields_desc = [
        XByteField("recirc_flag", 0),
        XIntField("flow_id", 0),
        XIntField("label", 0)
    ]

def csfq_packet(pktlen=0,
            eth_dst='00:11:11:11:11:11',
            eth_src='00:22:22:22:22:22',
            ip_src='0.0.0.2',
            ip_dst='0.0.0.1',
            udp_sport=8000,
            udp_dport=CSFQ_PORT,
            recirc_flag=0,
            flow_id=0,
            label=0):
    udp_pkt = simple_udp_packet(pktlen=0,
                                eth_dst=eth_dst,
                                eth_src=eth_src,
                                ip_dst=ip_dst,
                                ip_src=ip_src,
                                udp_sport=udp_sport,
                                udp_dport=udp_dport)

    return udp_pkt / CSFQ_HDR(recirc_flag=recirc_flag, flow_id=flow_id, label=label)

def scapy_sppifo_bindings():
    bind_layers(UDP, CSFQ_HDR, dport=CSFQ_PORT)

def receive_packet(test, port_id, template):
    dev, port = port_to_tuple(port_id)
    (rcv_device, rcv_port, rcv_pkt, pkt_time) = dp_poll(test, dev, port, timeout=2)
    nrcv = template.__class__(rcv_pkt)
    return nrcv

def print_packet(test, port_id, template):
    receive_packet(test, port_id, template).show2()

def addPorts(self):
    fp_ports = ["24/0","23/0","21/0","16/0"]
    loopback_ports = ["13/0","22/0"]
# fp_ports = ["13/0","14/0", "11/0"]
    pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
    self.sess_hdl = self.conn_mgr.client_init()
    self.dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))
    self.devPorts = []
    self.LPPorts = []
    self.dev = 0
    self.platform_type = "mavericks"
    board_type = self.pltfm_pm.pltfm_pm_board_type_get()
    if re.search("0x0234|0x1234|0x4234|0x5234", hex(board_type)):
        self.platform_type = "mavericks"
    elif re.search("0x2234|0x3234", hex(board_type)):
        self.platform_type = "montara"

    # get the device ports from front panel ports
    try:
        for fpPort in fp_ports:
            port, chnl = fpPort.split("/")
            devPort = \
                self.pal.pal_port_front_panel_port_to_dev_port_get(0,
                                                                int(port),
                                                                int(chnl))
            self.devPorts.append(devPort)

        if test_param_get('setup') == True or (test_param_get('setup') != True
            and test_param_get('cleanup') != True):

            # add and enable the platform ports
            for i in self.devPorts:
                self.pal.pal_port_add(0, i,
                                    pal_port_speed_t.BF_SPEED_100G,
                                    pal_fec_type_t.BF_FEC_TYP_NONE)
                self.pal.pal_port_an_set(0, i, 2);
                self.pal.pal_port_enable(0, i)
        
        for lbPort in loopback_ports:
                port, chnl = lbPort.split("/")
                devPort = \
                    self.pal.pal_port_front_panel_port_to_dev_port_get(0,
                                                                    int(port),
                                                                    int(chnl))
                self.LPPorts.append(devPort)

                # add and enable the platform ports
        for i in self.LPPorts:
            self.pal.pal_port_add(0, i,
                                pal_port_speed_t.BF_SPEED_100G, pal_fec_type_t.BF_FEC_TYP_NONE) #pal_fec_type_t.BF_FEC_TYP_REED_SOLOMON

            self.pal.pal_port_loopback_mode_set(0, i,
                                pal_loopback_mod_t.BF_LPBK_MAC_NEAR)
            self.pal.pal_port_an_set(0, i, 2);
            self.pal.pal_port_enable(0, i)
            
        self.conn_mgr.complete_operations(self.sess_hdl)

    except Exception as e:
		print "Some Error in port init"
    # test.pal.pal_port_add_all(dev_id, pal_port_speed_t.BF_SPEED_100G, pal_fec_type_t.BF_FEC_TYP_NONE)
    # test.pal.pal_port_enable_all(dev_id)
    # ports_not_up = True
    # print("Waiting for ports to come up...")
    # sys.stdout.flush()
    # num_tries = 12
    # i = 0
    # while ports_not_up:
    #     ports_not_up = False
    #     for p in swports:
    #         x = test.pal.pal_port_oper_status_get(dev_id, p)
    #         if x == pal_oper_status_t.BF_PORT_DOWN:
    #             ports_not_up = True
    #             print("  port", p, "is down")
    #             sys.stdout.flush()
    #             time.sleep(3)
    #             break
    #     i = i + 1
    #     if i >= num_tries:
    #         break
    # assert ports_not_up == False
    # print("All ports up.")
    # sys.stdout.flush()
    return



def init_tables(test, sess_hdl, dev_tgt):
    test.entry_hdls_ipv4 = []
    test.entry_flowrate_shl_0_table = []
    test.entry_flowrate_shl_1_table = []
    test.entry_flowrate_shl_2_table = []
    test.entry_flowrate_shl_3_table = []
    test.entry_check_uncongest_state_table = []

    ipv4_table_address_list = [-1062731006,-1062731004,-1062731001,-1062731000]
    ipv4_table_port_list = [0, 36,60,52]
    # 0 --16 dell7, 8 -- 15 dell2, 48 --10, 32 -- 12 dell8
    # sudo arp -s 192.168.2.5 0c:42:a1:5a:53:00
    # sudo arp -s 192.168.2.7 0c:42:a1:5a:5b:d8
    # sudo arp -s 192.168.2.8 b8:59:9f:e2:0c:16 
    # iperf -s -B 192.168.2.8 -p 5202
    # iperf -s -B 192.168.2.8 -p 5203
    ethernet_set_mac_src = ["\xa8\x2b\xb5\xde\x92\x2e", 
                            "\xa8\x2b\xb5\xde\x92\x32",
                            "\xa8\x2b\xb5\xde\x92\x36",
                            "\xa8\x2b\xb5\xde\x92\x3a",
                            "\xa8\x2b\xb5\xde\x92\x3e",
                            "\xa8\x2b\xb5\xde\x92\x42",
                            "\xa8\x2b\xb5\xde\x92\x46",
                            "\xa8\x2b\xb5\xde\x92\x4a",
                            "\xa8\x2b\xb5\xde\x92\x4e",
                            "\xa8\x2b\xb5\xde\x92\x52",
                            "\xa8\x2b\xb5\xde\x92\x56",
                            "\xa8\x2b\xb5\xde\x92\x5a"]
    ethernet_set_mac_dst = ["\x3c\xfd\xfe\xab\xde\xd8",
                            "\x3c\xfd\xfe\xa6\xeb\x10",
                            "\x3c\xfd\xfe\xaa\x5d\x00",
                            "\x3c\xfd\xfe\xaa\x46\x68",
                            "\x3c\xfd\xfe\xab\xde\xf0",
                            "\x3c\xfd\xfe\xab\xdf\x90",
                            "\x3c\xfd\xfe\xab\xe0\x50",
                            "\x3c\xfd\xfe\xab\xd9\xf0",
                            "\xd0\x94\x66\x3b\x12\x37",
                            "\xd0\x94\x66\x84\x9f\x19",
                            "\xd0\x94\x66\x84\x9f\xa9",
                            "\xd0\x94\x66\x84\x54\x81"]
    fix_src_port = []
    for i in range(128):
        fix_src_port.append(9000 + i)
    udp_src_port_list = []
    for i in range(128):
        udp_src_port_list.append(UDP_DSTPORT + i)

    # add entries for ipv4 routing
    # test.client.ipv4_route_set_default_action__drop(sess_hdl, dev_tgt)
    # for i in range(len(ipv4_table_address_list)):
    #     match_spec = sppifo_ipv4_route_match_spec_t(ipv4_table_address_list[i])
    #     action_spec = sppifo_set_egress_action_spec_t(ipv4_table_port_list[i])
    #     entry_hdl = test.client.ipv4_route_table_add_with_set_egress(
    #         sess_hdl, dev_tgt, match_spec, action_spec)
    #     test.entry_hdls_ipv4.append(entry_hdl)

    # ipv4_route
    # send
    # discard

    test.client.ipv4_route_set_default_action_discard(sess_hdl, dev_tgt)
    for i in range(len(ipv4_table_address_list)):
        match_spec = chainsketch_ipv4_route_match_spec_t(ipv4_table_address_list[i])
        print("-----------------------------")
        print(match_spec)
        action_spec = chainsketch_send_action_spec_t(ipv4_table_port_list[i])
        test.client.ipv4_route_table_add_with_send(sess_hdl, dev_tgt, match_spec, action_spec)
    
        
    

def clean_tables(test, sess_hdl, dev_id):
    print "closing session"


class SimpleTest(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["chainsketch"])
        scapy_sppifo_bindings()

    def runTest(self):
        sess_hdl = self.conn_mgr.client_init()
        self.sids = []
        qmap = tm_q_map_t(md_qid0_to_tm_q=0, md_qid1_to_tm_q=1, md_qid2_to_tm_q=2, md_qid3_to_tm_q=3, md_qid4_to_tm_q=4, md_qid5_to_tm_q=5, md_qid6_to_tm_q=6, md_qid7_to_tm_q=7, md_qid8_to_tm_q=0, md_qid9_to_tm_q=1, md_qid10_to_tm_q=2, md_qid11_to_tm_q=3, md_qid12_to_tm_q=4, md_qid13_to_tm_q=5, md_qid14_to_tm_q=6, md_qid15_to_tm_q=7, md_qid16_to_tm_q=0, md_qid17_to_tm_q=1, md_qid18_to_tm_q=2, md_qid19_to_tm_q=3, md_qid20_to_tm_q=4, md_qid21_to_tm_q=5, md_qid22_to_tm_q=6, md_qid23_to_tm_q=7, md_qid24_to_tm_q=0, md_qid25_to_tm_q=1, md_qid26_to_tm_q=2, md_qid27_to_tm_q=3, md_qid28_to_tm_q=4, md_qid29_to_tm_q=5, md_qid30_to_tm_q=6, md_qid31_to_tm_q=7, q_count=8)
        
        for port_number in [44]:
            self.tm.tm_set_port_q_mapping(dev_id, port_number, 8, qmap)
            for pri in range(8):
                self.tm.tm_set_q_sched_priority(dev_id, port_number, pri, pri)
        try:
            if (test_param_get('target') == 'hw'):
                addPorts(self)
            else:
                print "test_param_get(target):", test_param_get('target')
            init_tables(self, sess_hdl, dev_tgt)
            self.conn_mgr.complete_operations(sess_hdl)
            print "INIT Finished."
            sys.stdout.flush()
            while (True):
                time.sleep(1)
            self.conn_mgr.complete_operations(sess_hdl)
        finally:
            clean_tables(self, sess_hdl, dev_id)