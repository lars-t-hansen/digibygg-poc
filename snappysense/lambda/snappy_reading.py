# -*- fill-column: 100 -*-
#
# Respond to a SnappySense "reading" message by recording the reading in the history for the device
# and computing and sending relevant actuator commands for the location of the device.
#
# See ../mqtt-protocol.md for a description of the messages.
# See ../data-model.md for a description of the databases.

import snappy_data
import snappy_mqtt

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

    db = snappy_data.connect()

    sensor_device_entry = record_timestamp_and_reading(db, device, time, factor, reading)
    if sensor_device_entry == None:
        return

    trigger_actuator_if_necessary(db, snappy_data.device_location(sensor_device_entry), time, factor, reading)


# Record the `reading` for the `factor` on the `device` at the `time`. 
#
# If the recording succeeded, return the device entry for the device that recorded the reading,
# otherwise None.  The recording will fail if the device is disabled.
#
# Cost:
#   2 reads
#   1 write

def record_timestamp_and_reading(db, device, time, factor, reading):
    device_entry = snappy_data.get_device(db, device)
    if device_entry == None or snappy_data.device_is_disabled(device_entry):
        # If None, this is a bug, maybe.  The device should be in the database.  But it could have
        # been removed and the message could have been in transit during removal.
        return None

    history_entry = snappy_data.get_history_entry(db, device)
    if history_entry == None:
        # A bug somewhere, there should always be a history record because one is created for the
        # setup message.
        return None

    # Update the history entry with time and reading, and save it.
    snappy_data.set_history_last_contact(history_entry, time)
    snappy_data.history_entry_add_reading(history_entry, factor, time, reading)
    snappy_data.write_history_entry(db, history_entry)

    return device_entry


# Figure out if we need to send a command to an actuator at `location` to adjust the value for
# `factor`, and if so do it, and record this fact in the device record for the appropriate device
# for the given `time`.
#
# A location has a number of actuators, and at most one of those is for the same factor as the
# reading.
#
# Cost with actuator:
#   3 reads
#   1 write
#   1 mqtt publish
#
# Cost without actuator:
#   1 read

def trigger_actuator_if_necessary(db, location, time, factor, last_reading):
    location_entry = snappy_data.get_location_entry(db, location)
    if location_entry == None:
        return

    device_name, ideal_fn = snappy_data.find_actuator(location_entry, factor)
    if device_name == None:
        return
    
    ideal = evaluate_ideal(ideal_fn)
    if ideal == None:
        # Error
        return

    device_entry = snappy_data.get_enabled_device(db, device_name)
    if device_entry == None:
        # Device not found or not enabled
        return

    history_entry = snappy_data.get_history_entry(db, device_name)
    if history_entry == None:
        # Should have been created
        return

    # Record last contact

    snappy_data.set_history_last_contact(history_entry, time)

    # QoS1 to ensure that the message is received.
    # Send a command to the device to update itself if necessary

    outgoing = None
    if ideal != last_reading:
        outgoing = {"factor":factor, "reading":last_reading, "ideal":ideal}
        # TODO: Record last command in history

    if outgoing is not None:
        device_class = snappy_data.device_class(device_entry)
        snappy_mqtt.publish(f"snappy/control/{device_class}/{device_name}", outgoing, 1)

    snappy_data.write_history_entry(history_entry)

# Try to evaluate the idealfn for the actuator with given parameters.  Return
# the ideal value (a number) or None.

def evaluate_ideal(idealfn_string):
    ideal_elements = idealfn_string.split('/')
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
    ideal = None
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

# The IDEAL function "constant(c)" returns c.

def idealfn_constant(sensor_history_entry, c):
    return c

# We hardcode the arity for the functions in the program so we don't need to
# lookup this, DOCUMENTME.

idealfns = { "constant": {"arity": 1, "fn": idealfn_constant } }

