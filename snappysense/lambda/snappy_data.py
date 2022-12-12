# -*- fill-column: 100 -*-
#
# Database operations for snappysense.
#
# See ../mqtt-protocol.md for a description of the messages.
# See ../data-model.md for a description of the databases.

import boto3

# Default reading interval, in seconds.
# TODO: Make this sensible, 5s is much too often.
DEFAULT_READING_INTERVAL = 5

# Number of readings to keep per factor per device.
MAX_FACTOR_HISTORY = 10

# Number of actuator commands to keep per factor per device.
MAX_ACTION_HISTORY = 5

# Connect to the database and return the object representing it

def connect():
    return boto3.client('dynamodb', region_name='eu-central-1')

################################################################################
#
# LOCATION
                        
# Lookup a location record and return it if found, otherwise return None.

def get_location_entry(db, location):
    probe = db.get_item(TableName='snappy_location', Key={"location": {"S": location}})
    if probe == None or "Item" not in probe:
        return None
    return probe["Item"]

# If there is an actuator for the factor at the location, return the device name
# and the ideal function.  Otherwise return None, None

def find_actuator_device(location_entry, factor):
    actuator_entry = None
    for a in location_entry["actuators"]["L"]:
        if a["factor"]["S"] == factor:
            return a["device"]["S"], a["idealfn"]["S"]
    return None, None

################################################################################
#
# DEVICE

# Lookup a device record and return it if found, otherwise return None.

def get_device(db, device_name):
    probe = db.get_item(TableName='snappy_device', Key={"device":{"S": device_name}})
    if probe == None or "Item" not in probe:
        return None
    return probe["Item"]

def device_class(device_entry):
    return device_entry["class"]["S"]

def device_location(device_entry):
    return device_entry["location"]["S"]

def device_is_disabled(device_entry):
    return "enabled" in device_entry and device_entry["enabled"]["N"] == "0"

def device_reading_interval(device_entry):
    if "reading_interval" in device_entry:
        return int(device_entry["reading_interval"]["N"])
    return DEFAULT_READING_INTERVAL

################################################################################
#
# HISTORY

# Lookup a history record and return it if found, otherwise return None.

def get_history_entry(db, device):
    probe = db.get_item(TableName='snappy_history', Key={"device":{"S": device}})
    if probe == None or "Item" not in probe:
        return None
    return probe["Item"]

# Write the history entry to disk.
# TODO: Can this fail?  Do we care?

def write_history_entry(db, history_entry):
    db.put_item(TableName='snappy_history', Item=history_entry)

# This will get the history entry if it exists, otherwise it will create a new one, but it will *not*
# persist that entry in the database.

def get_history_entry_or_create(db, device):
    history_entry = get_history_entry(db, device)
    if history_entry == None:
        history_entry = {"device":{"S": device}, "last_contact": {"N":"0"}, "readings": {"L": []}, "actions": {"L":[]}}
    return history_entry

def set_history_last_contact(history_entry, time):
    history_entry["last_contact"]["N"] = str(time)

def history_entry_add_reading(history_entry, factor, time, reading):
    factor_entry = find_for_factor(history_entry, "readings", factor)

    # Push the new one onto the front and retire old ones from the end
    factor_entry.insert(0, {"time": {"N":str(time), "value": {"N": str(reading)}}})
    while factor_entry.len() > MAX_FACTOR_HISTORY:
        factor_entry.pop()

def history_entry_add_action(history_entry, factor, time, reading, ideal):
    factor_entry = find_for_factor(history_entry, "actions", factor)

    # Push the new one onto the front and retire old ones from the end
    factor_entry.insert(0, {"time": {"N":str(time), "reading": {"N": str(reading)}, "ideal": {"N": str(ideal)}}})
    while factor_entry.len() > MAX_ACTION_HISTORY:
        factor_entry.pop()

def find_for_factor(history_entry, which, factor):
    factor_entry = None
    for f in history_entry[which]:
        if f["factor"] == factor:
            factor_entry = f
            break

    if factor_entry == None:
        factor_entry = {"factor": {"S": factor}, "last": []}
        history_entry[which].append(factor_entry)

    return factor_entry



                        
