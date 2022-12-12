# -*- fill-column: 100 -*-
#
# Respond to a SnappySense "reading" MQTT package by recording the reading in the history for the
# device and computing and sending relevant actuator commands for the location of the device.
#
# See data-model.md for a description of the databases.

import boto3
import json

# Number of readings to keep per factor per device.
MAX_FACTOR_HISTORY = 10

# Number of actuator commands to keep per factor per device.
MAX_ACTUATOR_HISTORY = 5

# Input fields
#   device - string
#   class - string
#   time - integer
#   factor - string
#   reading - number

def lambda_handler(event, context):
    device = event["device"]
    device_class = event["class"]
    time = event["time"]
    factor = event["factor"]
    reading = event["reading"]

    db = boto3.client('dynamodb', region_name='eu-central-1')

    readings = record_timestamp_and_reading(db, device, time, factor, reading)
    if readings is None:
        return

    trigger_actuator_if_necessary(db, device_entry["location"]["S"], time, factor, readings)


# Record the `reading` for the `factor` on the `device` at the `time`. 
#
# Return the updated list of readings for the factor on the device if the
# recording succeeded, otherwise None.  The recording will fail if the
# device is disabled.

def record_timestamp_and_reading(db, device, time, factor, reading):
    device_entry = get_enabled_device(db, device)
    if device_entry is None:
        return False

    history_entry = get_history_entry(db, device)
    if history_entry is None:
        return False

    timestamp = {"N": str(time)}

    # Update the history entry with time and reading, and save it.

    history_entry["last_contact"] = timestamp
    
    factor_entry = None
    for f in history_entry["readings"]:
        if f["factor"] == factor:
            factor_entry = f
            break
    if factor_entry == None:
        factor_entry = {"factor": {"S": factor}, "last": []}
        history_entry["readings"].append(factor_entry)
        
    # Push the new one onto the front and retire old ones from the end
    factor_entry.insert(0, {"time": timestamp, "value": {"N": str(reading)}})
    while factor_entry.len() > MAX_FACTOR_HISTORY:
        factor_entry.pop()

    # TODO: Can this fail?  Do we care?
    db.put_item(TableName='snappy_history', Item=history_entry)
    return True


# Figure out if we need to send a command to an actuator at `location` to adjust the
# value for `factor`, and if so do it, and record this fact in the device record
# for the appropriate device for the given `time`.
#
# A location has a number of actuators, and at most one of those is for the same factor
# as the reading.

def trigger_actuator_if_necessary(db, location, time, factor, readings):

    location_entry = get_location_entry(db, location)
    if location_entry is None:
        return

    actuator_entry = None
    for a in location_entry["actuators"]["O"]:
        if a["factor"]["S"] == factor:
            actuator_entry = a
            break
    if actuator_entry == None:
        return
    
    ideal = evaluate_ideal(actuator_entry)
    if ideal is None:
        # Error
        return

    device_name = actuator_entry["device"]["S"]

    device_entry = get_enabled_device(db, device_name)
    if device_entry is None:
        # Device not found or not enabled
        return

    history_entry = get_history_entry(db, device_name)
    if history_entry is None:
        # Should have been created
        return

    # Record last contact

    history_entry["last_contact"] = {"N": str(time)}
    db.put_item(TableName='snappy_history', Item=history_entry)

    outgoing = None
    if ideal != readings[0]:
        outgoing = {"factor":factor, "reading":readings[0], "ideal":ideal}

    # QoS1 to ensure that the message is received.

    if outgoing is not None:
        iot_client = boto3.client('iot-data', region_name='eu-central-1')
        iot_client.publish(
            topic=f"snappy/control/{device_class}/{device}",
            qos=1,
            payload=json.dumps(outgoing)
        )


    history_entry = None
    time_attrib = {"N": str(time)}
    hist_resp = db.get_item(TableName='snappy_history', Key=device_key)
    if hist_resp is not None and "Item" in hist_resp:
        history_entry = hist_resp["Item"]
        history_entry["last_contact"] = time_attrib
    else:
        history_entry = {**key, "last_contact": time_attrib, "readings": {"O": []}, "actions": {"O":[]}}
    db.put_item(TableName='snappy_history', Item=history_entry)

    # Update the device if necessary

    outgoing = None
    if "enabled" in device_entry and device_entry["enabled"]["N"] == "0":
        outgoing = {"enabled": 0}
    elif reading_interval != 0 and "reading_interval" in device_entry:
        device_interval = int(device_entry["reading_interval"]["N"])
        if reading_interval != device_interval:
            outgoing = {"reading_interval": device_interval}


# Try to evaluate the idealfn for the actuator with given parameters.  Return
# the ideal value (a number) or None.

def evaluate_ideal(actuator_entry):
    ideal = None

    ideal_elements = actuator_entry["idealfn"]["S"].split('/')
    if ideal_elements.len() == 0: 
        # Error
        return

    fname = ideal_elements[0]
    args = ideal_elements[1:]
    if fname not in idealfns:
        # Error
        return

    idealfn = idealfns[fname]
    if idealfns["arity"] != args.len():
        # Error
        return

    # Sigh, we want apply and map
    alen = args.len()
    if alen == 0:
        ideal = idealfns["fn"]()
    elif alen == 1:
        ideal = idealfns["fn"](float(args[0]))
    elif alen == 2:
        ideal = idealfns["fn"](float(args[0]), float(args[1]))
    else:
        # Error
        return

    return ideal

# Lookup a device record and return it if found and the device is enabled, otherwise
# return None.

def get_enabled_device(db, device):
    dev_resp = db.get_item(TableName='snappy_device', Key={"device": {"S": device}))
    if dev_resp is None or "Item" not in dev_resp:
        # Device does not exist, this is an error.  TODO: Logging?
        return None

    device_entry = dev_resp["Item"]
    if "enabled" in device_entry and device_entry["enabled"]["N"] == "0":
        # Device has been disabled, this is an error or a delayed message.
        # Discard the reading.
        return None

    return device_entry

# Lookup a history record and return it if found, otherwise return None.

def get_history_entry(db, device):
    hist_resp = db.get_item(TableName='snappy_history', Key={"device": {"S": device}})
    if hist_resp is None or "Item" not in hist_resp:
        # History entry does not exist, this is an error, because the startup lambda
        # should have created it.  TODO: Logging?
        return None

    return hist_resp["Item"]

# Lookup a location record and return it if found, otherwise return None.

def get_location_entry(db, location):
    loc_resp = db.get_item(TableName='snappy_location', Key={"location": {"S": location}})
    if loc_resp is None or "Item" not in loc_resp:
        # Location entry does not exist, this is an error.  TODO: Logging?
        return None

    return loc_resp["Item"]

# The IDEAL function "work_temperature(temp_during_work, temp_during_off_hours)"

def idealfn_work_temperature(during_work, during_off):
    # TODO
    # If the current time *at the location of the device* (ouch!) is work hours
    # then choose during_work, otherwise during_off
    return during_work

# The IDEAL function "constant(c)"

def idealfn_constant(c):
    return c

# We hardcode the arity for the functions in the program so we don't need to
# lookup this, DOCUMENTME.

idealfns = {
    "work_temperature":{"arity":2, "fn":idealfn_work_temperature},
    "constant":{"arity":1, "fn":idealfn_constant}
    }

