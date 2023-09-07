from switch_helpers import ApiHelper, port_speed_map
from bf_switcht_api_thrift.api_adapter import ApiAdapter
from bf_switcht_api_thrift.model_headers import SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE_TAGGED
from bf_switcht_api_thrift.model_headers import SWITCH_PORT_ATTR_FEC_TYPE_RS, \
    SWITCH_PORT_ATTR_FEC_TYPE_NONE

# This module is copyable between SDE 9.9.0 and SDE 9.11.0

class PortSetup(ApiHelper):
    # setup 1
    VLID_APROJ = 100
    VLID_GEN = 200
    VLID_SPINE1 = 211
    VLID_SPINE2 = 215
    
    MX_MTU = 10240

    def setUp(self):
        print("In setup")
        if not hasattr(self, 'client'):
            super(ApiAdapter, self).setUp()

        self.device = self.get_device_handle(0)

        self._sw_setup2()
        
        while True: 
            pass
    
    def _sw_setup2(self):
        VLID_COM = 215
        vlan = self.add_vlan(self.device, vlan_id=VLID_COM)

        # Port 1-10, 10x100GB LAG, VLAN 215, Tagged, FEC=RS
        self._add_lag(range(1, 11), vlids=[VLID_COM], vls=[vlan], fec=SWITCH_PORT_ATTR_FEC_TYPE_RS)

        # Port 11-16, 100GB, VLAN 215, Untagged
        for n in range(11, 17):
            port = self.add_port(self.device, port_id=self._portid(n), speed=port_speed_map["100g"],
                learning=1, rx_mtu=self.MX_MTU, tx_mtu=self.MX_MTU, port_vlan_id=VLID_COM)
            self.add_vlan_member(self.device, member_handle=port, vlan_handle=vlan)

        # Port 17, 4x25GB, VLAN 215, Untagged
        self._add_4x25_port(17, VLID_COM, vlan)

        # Port 18-30, 100GB, VLAN 215, Untagged
        for n in range(18, 31):
            port = self.add_port(self.device, port_id=self._portid(n), speed=port_speed_map["100g"],
                learning=1, rx_mtu=self.MX_MTU, tx_mtu=self.MX_MTU, port_vlan_id=VLID_COM)
            self.add_vlan_member(self.device, member_handle=port, vlan_handle=vlan)

        # Port 31-32, 4x25GB each, VLAN 215, Untagged
        for n in [31, 32]:
            self._add_4x25_port(n, VLID_COM, vlan)
    
    def _sw_setup1(self):
        # Port 1-4, 4x100GB LACP, VLAN 211, 215
        vlansp1 = self.add_vlan(self.device, vlan_id=self.VLID_SPINE1)
        vlansp2 = self.add_vlan(self.device, vlan_id=self.VLID_SPINE2)
        self._add_lag(range(1, 5), vlids=[self.VLID_SPINE1, self.VLID_SPINE2], vls=[vlansp1, vlansp2], fec=SWITCH_PORT_ATTR_FEC_TYPE_NONE)
        
        # Port 5-10, 6x100GB LACP, VLAN 200
        vlangen = self.add_vlan(self.device, vlan_id=self.VLID_GEN)
        self._lag2_mems = self._add_lag(range(5, 11), vlids=[self.VLID_GEN], vls=[vlangen], fec=SWITCH_PORT_ATTR_FEC_TYPE_RS)

        # Port 11-16, 6x100GB LACP, VLAN 200
        self._lag3_mems = self._add_lag(range(11, 17), vlids=[self.VLID_GEN], vls=[vlangen], fec=SWITCH_PORT_ATTR_FEC_TYPE_RS)

        # Port 17, 4x25GB, VLAN 100
        vlanap = self.add_vlan(self.device, vlan_id=self.VLID_APROJ)
        self._add_4x25_port(17, self.VLID_APROJ, vlanap)

        # Port 18-29, 100GB each, VLAN 200
        for n in range(18, 30):
            port = self.add_port(self.device, port_id=self._portid(n), speed=port_speed_map["100g"],
                learning=1, rx_mtu=self.MX_MTU, tx_mtu=self.MX_MTU, port_vlan_id=self.VLID_GEN)
            self.add_vlan_member(self.device, member_handle=port, vlan_handle=vlangen)

        # Port 30, 100GB Tagged, VLAN 211, 215
        port = self.add_port(self.device, port_id=self._portid(30), speed=port_speed_map["100g"],
                learning=1, rx_mtu=self.MX_MTU, tx_mtu=self.MX_MTU, port_vlan_id=self.VLID_SPINE1)
        for vl in [vlansp1, vlansp2]:
            self.add_vlan_member(self.device, member_handle=port, vlan_handle=vl, 
                tagging_mode=SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE_TAGGED)

        # Port 31-32, 4x25GB each, VLAN 200
        for n in [31, 32]:
            self._add_4x25_port(n, self.VLID_GEN, vlangen)

        self._remove_lag_member()

    def _add_lag(self, pnums: list, vlids: list, vls: list, fec: int) -> list:
        assert len(vlids) == len(vls)

        lag = self.add_lag(self.device, learning=1, port_vlan_id=vlids[0])
        for vl in vls:
            self.add_vlan_member(self.device, member_handle=lag, vlan_handle=vl, tagging_mode=SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE_TAGGED)

        memb_obj_ids = []
        for n in pnums:
            port = self.add_port(self.device, port_id=self._portid(n), speed=port_speed_map["100g"], 
                learning=1, rx_mtu=self.MX_MTU, tx_mtu=self.MX_MTU, port_vlan_id=vlids[0], fec_type=fec)
            for vl in vls:
                self.add_vlan_member(self.device, member_handle=port, vlan_handle=vl, tagging_mode=SWITCH_VLAN_MEMBER_ATTR_TAGGING_MODE_TAGGED)
            oid = self.add_lag_member(self.device, port_handle=port, lag_handle=lag)
            memb_obj_ids.append(oid) 
        
        return memb_obj_ids

    def _add_4x25_port(self, pnum: int, vlid: int, vl):
        for p in range(4):
            port = self.add_port(self.device, port_id=self._portid(pnum, p), speed=port_speed_map["25g"], 
                learning=1, rx_mtu=self.MX_MTU, tx_mtu=self.MX_MTU, port_vlan_id=vlid)
            self.add_vlan_member(self.device, member_handle=port, vlan_handle=vl)

    def _portid(self, pnum: int, part:int = 0):
        return (pnum - 1) * 4 + part 
    
    def _remove_lag_member(self):
        self.client.object_delete(self._lag2_mems[0])
        self.client.object_delete(self._lag3_mems[0])

    def runTest(self):
        pass

    def tearDown(self):
        print("In tearDown")
        super(ApiAdapter, self).tearDown()
        pass
