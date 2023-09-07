################################################################################
 #  INTEL CONFIDENTIAL
 #
 #  Copyright (c) 2021 Intel Corporation
 #  All Rights Reserved.
 #
 #  This software and the related documents are Intel copyrighted materials,
 #  and your use of them is governed by the express license under which they
 #  were provided to you ("License"). Unless the License provides otherwise,
 #  you may not use, modify, copy, publish, distribute, disclose or transmit this
 #  software or the related documents without Intel's prior written permission.
 #
 #  This software and the related documents are provided as is, with no express or
 #  implied warranties, other than those that are expressly stated in the License.
 #################################################################################

"""
Helper functions for PTF testcases
"""

import bf_switcht_api_thrift

import time
from time import sleep
from os import getenv
import sys
import logging

import unittest
import random

import ptf.dataplane as dataplane
import api_base_tests
import pd_base_tests
import model_utils as u

from ptf import config
from ptf.testutils import *
from ptf.packet import *
from ptf.thriftutils import *

import os
import ptf.mask
import six

from bf_switcht_api_thrift.ttypes import *
from bf_switcht_api_thrift.model_headers import *
from bf_switcht_api_thrift.api_adapter import ApiAdapter

device = 0

swports = []
'''
config["interfaces"] is [ <device>, <dev_port> ]
For model
tofino  - (0,0), (0,1), (0,2) etc
tofino2/tofino3 - (0,8), (0,9), (0,10) etc
So for tofino2/tofino3, we will use (dev_port - 8) as port_id
For asic
just use what is given in the config
'''
for device, port, ifname in config["interfaces"]:
    test_params = ptf.testutils.test_params_get()
    arch = test_params['arch']
    if arch == 'tofino2':
        if test_params['target'] == 'hw':
            swports.append(port)
        else:
            if port >= 8:
                swports.append(port - 8)
    elif arch == 'tofino3':
        if test_params['target'] == 'hw':
            swports.append(port)
        else:
            if port >= 8:
                swports.append(int((port - 8)/2))
    else:
        swports.append(port)
swports.sort()

if swports == []:
    swports = [x for x in range(65)]

port_speed_map = {
    "10g": SWITCH_PORT_ATTR_SPEED_10G,
    "25g": SWITCH_PORT_ATTR_SPEED_25G,
    "40g": SWITCH_PORT_ATTR_SPEED_40G,
    "40g_r2": SWITCH_PORT_ATTR_SPEED_40G_R2,
    "50g": SWITCH_PORT_ATTR_SPEED_50G,
    "50g_r1": SWITCH_PORT_ATTR_SPEED_50G_R1,
    "100g": SWITCH_PORT_ATTR_SPEED_100G,
    "100g_r2": SWITCH_PORT_ATTR_SPEED_100G_R2,
    "200g": SWITCH_PORT_ATTR_SPEED_200G,
    "200g_r8": SWITCH_PORT_ATTR_SPEED_200G_R8,
    "400g": SWITCH_PORT_ATTR_SPEED_400G,
    "default": SWITCH_PORT_ATTR_SPEED_25G
}


class ApiHelper(ApiAdapter):
    dev_id = device
    pipe1_internal_ports = range(128, 144)
    pipe3_internal_ports = range(384, 400)

    def setUp(self):
        super(ApiAdapter, self).setUp()

    def tearDown(self):
        test_params = ptf.testutils.test_params_get()
        if 'reboot' in list(test_params.keys()):
            if test_params['reboot'] == 'hitless':
                if hasattr(self, 'port_list'):
                    for port_oid in self.port_list:
                        self.client.object_delete(port_oid)
        self.port_list = []
        super(ApiAdapter, self).tearDown()

    def _push(self, keyword, status, object_id, *args, **kwargs):
        if not hasattr(self, 'stack'):
            self.stack = []
        if (status == 0):
            self.stack.append((keyword, status, object_id, args, kwargs))
        else:
            self.stack.append((keyword, status, 0, args, kwargs))

    def _pop(self, keyword, *args, **kwargs):
        match_this = (keyword, args, kwargs)
        for idx, pack in enumerate(self.stack):
            if pack == match_this:
                del self.stack[idx]
                break

    def _clean(self, pack):
        keyword, status, object_id, args, kwargs = pack
        try:
            func = None
            if keyword == 'object_create':
                func = self.client.object_delete

            if func:
                if (status == 0):
                    func(object_id, *args, **kwargs)
            else:
                print('Clean up method not found')
        except:
            msg = ('Calling %r with params\n  object_id : %s\n  args  : %s\n  kwargs: %s'
                   '') % (func.__name__, object_id, args, kwargs)
            raise ValueError(msg)

    def cleanlast(self, count = 1):
        if hasattr(self, 'stack'):
            for i in range(count):
                if len(self.stack) > 0:
                    self._clean(self.stack.pop())
                else:
                    break

    def clean_to_object(self, obj):
        if hasattr(self, 'stack'):
            while len(self.stack) > 0:
                pack = self.stack.pop()
                self._clean(pack)
                keyword, status, object_id, args, kwargs = pack
                if object_id == obj:
                    break

    # hitless tests run together clears out thrift state
    # Remove ports for each test when running hitless tests
    def cleanup(self):
        if hasattr(self, 'stack'):
            while len(self.stack) > 0:
                self._clean(self.stack.pop())
        test_params = ptf.testutils.test_params_get()
        if 'reboot' in list(test_params.keys()):
          if test_params['reboot'] == 'hitless' or test_params['reboot'] == 'fastreconfig':
                if hasattr(self, 'port_list'):
                    for port_oid in self.port_list:
                        self.client.object_delete(port_oid)
        self.port_list = []
        super(ApiAdapter, self).tearDown()

    def status(self):
        if hasattr(self, 'stack'):
            keyword, ret, object_id, args, kwargs = self.stack[len(self.stack) - 1]
            return ret

    def attribute_set(self, object, attribute, value):
        attr = u.attr_make(attribute, value)
        return self.client.attribute_set(object, attr)

    def object_counters_get(self, object):
        if self.test_params['target'] == 'hw':
            print("HW: Sleeping for 3 sec before fetching stats")
            time.sleep(3)
        return self.client.object_counters_get(object)

    def attr_get(self, val):
        t = val.type
        if t == switcht_value_type.BOOL:
            return val.BOOL
        elif t == switcht_value_type.UINT8:
            return val.UINT8
        elif t == switcht_value_type.UINT16:
            return val.UINT16
        elif t == switcht_value_type.UINT32:
            return val.UINT32
        elif t == switcht_value_type.UINT64:
            return val.UINT64
        elif t == switcht_value_type.INT64:
            return val.INT64
        elif t == switcht_value_type.ENUM:
            return val.ENUM
        elif t == switcht_value_type.MAC:
            return val.MAC
        elif t == switcht_value_type.STRING:
            return val.STRING
        elif t == switcht_value_type.OBJECT_ID:
            return val.OBJECT_ID
        elif t == switcht_value_type.LIST:
            return val.LIST
        elif t == switcht_value_type.RANGE:
            return val.RANGE

    def attribute_get(self, object, attribute):
        ret = self.client.attribute_get(object, attribute)
        return self.attr_get(ret.attr.value)

    def get_device_handle(self, dev_id):
        attrs = list()
        value = switcht_value_t(type=switcht_value_type.UINT16, UINT16=dev_id)
        attr = switcht_attribute_t(id=SWITCH_DEVICE_ATTR_DEV_ID, value=value)
        attrs.append(attr)
        ret = self.client.object_get(SWITCH_OBJECT_TYPE_DEVICE, attrs)
        return ret.object_id

    def get_port_ifindex(self, port_handle, port_type=0):
        return (port_type << 9 | (port_handle & 0x1FF))

    def wait_for_interface_up(self, port_list):
        self.test_params = ptf.testutils.test_params_get()
        if self.test_params['target'] == 'hw':
            print("Waiting for ports UP...")
            for num_of_tries in range(20):
                time.sleep(1)
                all_ports_are_up = True
                # wait till the port are up
                for port in port_list:
                    state = self.attribute_get(port, SWITCH_PORT_ATTR_OPER_STATE)
                    if state != SWITCH_PORT_ATTR_OPER_STATE_UP:
                        all_ports_are_up = False
                if all_ports_are_up:
                    break
            if not all_ports_are_up:
                raise RuntimeError('Not all of the ports are up')


    def startFastReconfig(self):
        time.sleep(2)
        print(" -- Start FastReconfig -- ")
        self.fast_reconfig_begin(self.dev_id)
        self.fast_reconfig_end(self.dev_id)
        print(" -- Finish FastReconfig -- ")
        print(" -- FastReconfig sequence complete -- ")

    def startWarmReboot(self):
        time.sleep(2)
        print(" -- Start WarmReboot --")
        self.warm_init_begin(self.dev_id)
        self.warm_init_end(self.dev_id)
        print(" -- Finish WarmReboot --")
        print(" -- WarmReboot sequence complete -- ")

    # Run test case with the below param option
    # --wr will run WarmReboot sequence
    # --fr will run FastReconfig sequence
    # Function param:
    #   mode - if set to 0 [or if no param options]
    #     then neither sequence will run
    #   cb - if appropriate, invoke cb function after FR/WR replay
    # Function returns true if FR/WR seq was run, else False
    def startFrWrReplay(self, mode, cb=None):
        if (mode == 0):
            return False

        reboot_type = None
        test_params = ptf.testutils.test_params_get()
        if 'reboot' in list(test_params.keys()):
            reboot_type = test_params['reboot']

        if (reboot_type == 'hitless'):
            self.startWarmReboot()
            if cb:
                cb()
            return True
        elif (reboot_type == 'fastreconfig'):
            self.startFastReconfig()
            if cb:
                cb()
            return True

        return False
