# -*- fill-column: 100 -*-
#
# Respond to a SnappySense "startup" MQTT package by recording the time in the history for the
# device and computing and sending relevant configuration commands for the device.
#
# See ../mqtt-protocol.md for a description of the messages.
# See ../data-model.md for a description of the databases.

import snappy_data
import snappy_mqtt

# Input fields
#   device - string
#   class - string
#   time - integer
#   reading_interval - integer(may eventually be absent but for now assume it's 0 if irrelevant)

def startup_event(event, context):
    device = event["device"]
    device_class = event["class"]
    time = event["time"]
    reading_interval = event["reading_interval"]

    db = snappy_data.connect()

    device_entry = snappy_data.get_device(db, device)
    if device_entry is None:
        # Device does not exist, this is an error.  TODO: Logging?
        return

    # Update the history entry for the device, creating it if necessary.

    history_entry = snappy_data.get_history_entry_or_create(db, device)
    snappy_data.set_history_last_contact(time)
    snappy_data.write_history(db, history_entry)

    # Configure the device if necessary.
    #
    # Send the configuration with QoS1 to ensure that the message is received.  The device needs to
    # be prepared to receive several config commands after it connects, as a lingering command may
    # be delivered first followed by a new command.

    outgoing = None
    if snappy_data.device_is_disabled(device_entry):
        outgoing = {"enabled": 0}
    else:
        ri = snappy_data.device_reading_interval(device_entry)
        if reading_interval != 0 and reading_interval != ri:
            outgoing = {"reading_interval": ri}

    if outgoing is not None:
        snappy_mqtt.publish(f"snappy/control/{device_class}/{device}", outgoing, 1)
