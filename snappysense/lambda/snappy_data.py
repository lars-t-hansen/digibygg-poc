# -*- fill-column: 100 -*-
#
# Database operations for snappysense.
#
# See ../mqtt-protocol.md for a description of the messages.
# See ../data-model.md for a description of the databases.

import boto3

# TODO: Store parameters in a separate file?

# Default reading interval, in seconds.
# TODO: Make this sensible
DEFAULT_READING_INTERVAL = 5

# Number of readings to keep per factor per device.
MAX_FACTOR_HISTORY = 10

# Number of actuator commands to keep per factor per device.
MAX_ACTUATOR_HISTORY = 5

# Connect to the database and return the object representing it

def connect():
    return boto3.client('dynamodb', region_name='eu-central-1')

################################################################################
#
# LOCATION
                        
# Lookup a location record and return it if found, otherwise return None.

def get_location_entry(db, location):
    loc_resp = db.get_item(TableName='snappy_location', Key={"location": {"S": location}})
    if loc_resp is None or "Item" not in loc_resp:
        return None

    return loc_resp["Item"]

# If there is an actuator for the factor at the location, return the device name
# and the ideal function.  Otherwise return None, None

def find_actuator_device(location_entry, factor):
    actuator_entry = None
    for a in location_entry["actuators"]["O"]:
        if a["factor"]["S"] == factor:
            return a["device"]["S"], a["idealfn"]["S"]
    return None, None

################################################################################
#
# DEVICE

# Lookup a device record and return it if found, otherwise return None.

def get_device(db, device_name):
    dev_resp = db.get_item(TableName='snappy_device', Key={"device":{"S": device_name}})
    if dev_resp is None or "Item" not in dev_resp:
        return None
    return dev_resp["Item"]

# Lookup a device record and return it if found and the device is enabled, otherwise
# return None.

def get_enabled_device(db, device):
    device_entry = get_device(db, device)
    if device_entry is None:
        return None
    if device_is_disabled(device):
        return None
    return device_entry

def device_class(device_entry):
    return device_entry["class"]["S"]

def device_location(device_entry):
    return device_entry["location"]["S"]

# Get the "enabled" field (which may be absent); default True

def device_is_disabled(device_entry):
    return "enabled" in device_entry and device_entry["enabled"]["N"] == "0"

# Get the "reading_interval" field (which may be absent); default DEFAULT_READING_INTERVAL.
# The value is an integer.

def device_reading_interval(device_entry):
    if "reading_interval" in device_entry:
        return int(device_entry["reading_interval"]["N"])
    return DEFAULT_READING_INTERVAL
    return None

################################################################################
#
# HISTORY

# Lookup a history record and return it if found, otherwise return None.

def get_history_entry(db, device):
    hist_resp = db.get_item(TableName='snappy_history', Key={"device": {"S": device}})
    if hist_resp is None or "Item" not in hist_resp:
        return None

    return hist_resp["Item"]

# This will get the history entry if it exists, otherwise it will create a new one, but it will *not*
# persist that entry in the database.

def get_history_entry_or_create(db, device):
    history_entry = get_history_entry(db, device)
    if history_entry is None:
        history_entry = {"device":{"S": device}, "last_contact": {"N":"0"}, "readings": {"O": []}, "actions": {"O":[]}}

    return history_entry

def set_history_last_contact(history_entry, time):
    history_entry["last_contact"]["N"] = str(time)

def history_entry_add_reading(history_entry, factor, time, reading):
    factor_entry = None
    for f in history_entry["readings"]:
        if f["factor"] == factor:
            factor_entry = f
            break

    if factor_entry == None:
        factor_entry = {"factor": {"S": factor}, "last": []}
        history_entry["readings"].append(factor_entry)
        
    # Push the new one onto the front and retire old ones from the end
    factor_entry.insert(0, {"time": {"N":str(time), "value": {"N": str(reading)}})
    while factor_entry.len() > MAX_FACTOR_HISTORY:
        factor_entry.pop()

def write_history_entry(db, history_entry):
    # TODO: Can this fail?  Do we care?
    db.put_item(TableName='snappy_history', Item=history_entry)

