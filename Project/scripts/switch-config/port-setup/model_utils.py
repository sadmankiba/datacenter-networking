
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


import json
from ptf import testutils
from bf_switcht_api_thrift.ttypes import *
import ctypes


def twos_comp(val, bits):
    """compute the 2's complement of int value val"""
    if (val & (1 << (bits - 1))) != 0: # if sign bit is set e.g., 8bit: 128-255
        val = val - (1 << bits)        # compute negative value
    return val                         # return positive value as is

def object_info_map_build():
    map = {}
    objects_k = 'objects'
    objects_v = model[objects_k]
    for obj_k  in objects_v:
        obj_v = objects_v[obj_k]
        ot_k = "object_type"
        ot_v = obj_v[ot_k]
        obj_v["object_name"] = obj_k
        map[ot_v] = obj_v
    return map

def attr_md_map_build():
    map = {}
    objects_k = 'objects'
    objects_v = model[objects_k]
    for obj_k in objects_v:
        obj_v = objects_v[obj_k]
        attrs_k = "attributes"
        attrs_v = obj_v[attrs_k]
        for attr_k in attrs_v:
            attr_v = attrs_v[attr_k]
            attr_id_k = "attr_id"
            attr_id_v = attr_v[attr_id_k]
            attr_v["attr_name"] = attr_k
            map[attr_id_v] = attr_v
    return map

def object_type_get( object_name):
    objects_k = 'objects'
    objects_v = model[objects_k]
    obj_k = object_name
    obj_v = objects_v[obj_k]
    ot_k = 'object_type'
    ot_v = obj_v[ot_k]
    assert (ot_v)
    return ot_v

def attr_id_get(object_name, attr_name):
    objects_k = 'objects'
    objects_v = model[objects_k]
    obj_k = object_name
    obj_v = objects_v[obj_k]
    attrs_k = 'attributes'
    attrs_v = obj_v[attrs_k]
    attr_k = attr_name
    attr_v = attrs_v[attr_k]
    attr_id_k = "attr_id"
    attr_id_v = attr_v[attr_id_k]
    assert (attr_id_v)
    return attr_id_v


# Hacky stuff ahead..
# Thrift doesn't have unsigned types, doing conversions here.
def attr_make( attr_id, val_in):

    attr_md = attrs[attr_id]
    type_info_k = "type_info"
    type_info_v = attr_md[type_info_k]
    type_k = "type"
    type_v = type_info_v[type_k]

    if type_v == 'SWITCH_TYPE_BOOL':
        val = switcht_value_t(type=switcht_value_type.BOOL, BOOL=val_in)
    elif type_v == 'SWITCH_TYPE_UINT8':
        val_signed = twos_comp(val_in, 8)
        assert(ctypes.c_uint8(val_signed).value == val_in)
        val = switcht_value_t(type=switcht_value_type.UINT8, UINT8=val_signed)
    elif type_v == 'SWITCH_TYPE_UINT16':
        val_signed = twos_comp(val_in, 16)
        assert(ctypes.c_uint16(val_signed).value == val_in)
        val = switcht_value_t(type=switcht_value_type.UINT16, UINT16=val_signed)
    elif type_v == 'SWITCH_TYPE_UINT32':
        val_signed = twos_comp(val_in, 32)
        assert(ctypes.c_uint32(val_signed).value == val_in)
        val = switcht_value_t(type=switcht_value_type.UINT32, UINT32=val_signed)
    elif type_v == 'SWITCH_TYPE_UINT64':
        val_signed = twos_comp(val_in, 64)
        assert(ctypes.c_uint64(val_signed).value == val_in)
        val = switcht_value_t(type=switcht_value_type.UINT64, UINT64=val_signed)
    elif type_v == 'SWITCH_TYPE_INT64':
        val = switcht_value_t(type=switcht_value_type.INT64, INT64=val_in)
    elif type_v == 'SWITCH_TYPE_ENUM':
        val_signed = twos_comp(val_in, 64)
        assert(ctypes.c_uint64(val_signed).value == val_in)
        val = switcht_value_t(type=switcht_value_type.ENUM, ENUM=val_signed)
    elif type_v == 'SWITCH_TYPE_OBJECT_ID':
        val = switcht_value_t(type=switcht_value_type.OBJECT_ID, OBJECT_ID=val_in)
    elif type_v == 'SWITCH_TYPE_MAC':
        val = switcht_value_t(type=switcht_value_type.MAC, MAC=val_in)
    elif type_v == 'SWITCH_TYPE_STRING':
        val = switcht_value_t(type=switcht_value_type.STRING, STRING=val_in)
    elif type_v == 'SWITCH_TYPE_IP_ADDRESS':
        val = switcht_value_t(type=switcht_value_type.IP_ADDRESS, IP_ADDRESS=val_in)
    elif type_v == 'SWITCH_TYPE_IP_PREFIX':
        val = switcht_value_t(type=switcht_value_type.IP_PREFIX, IP_PREFIX=val_in)
    elif type_v == 'SWITCH_TYPE_LIST':
        val = switcht_value_t(type=switcht_value_type.LIST, LIST=tuple(val_in))
    else:
        assert(0)
    return switcht_attribute_t(id=attr_id, value=val)


model_file_name = testutils.test_param_get('api_model_json')
if model_file_name is None:
    model_file_name = 'install/share/switch/aug_model.json'
print("Model file %s" % model_file_name)
assert(model_file_name)
with open(model_file_name) as model_file:
    model = json.load(model_file)
assert(model)
objects = object_info_map_build()
assert(objects)
attrs = attr_md_map_build()
assert(attrs)
