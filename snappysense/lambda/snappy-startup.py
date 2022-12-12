# -*- fill-column: 100 -*-
#
# Respond to a SnappySense "startup" MQTT package by recording the time in the history for the
# device and computing and sending relevant configuration commands for the device.
#
# See data-model.md for a description of the databases.

import boto3
import json

# Input fields
#   device - string
#   class - string
#   time - integer
#   reading_interval - integer(may eventually be absent but for now assume it's 0 if irrelevant)

def lambda_handler(event, context):
    device = event["device"]
    device_class = event["class"]
    time = event["time"]
    reading_interval = event["reading_interval"]

    db = boto3.client('dynamodb', region_name='eu-central-1')
    device_key = {"device": {"S": device}}

    # Get the device, bail if not found.

    dev_resp = db.get_item(TableName='snappy_device', Key=device_key)
    if dev_resp is None or "Item" not in dev_resp:
        # Device does not exist, this is an error.  TODO: Logging?
        return
    device_entry = dev_resp["Item"]

    # Create or update the history entry.

    history_entry = None
    time_attrib = {"N": str(time)}
    hist_resp = db.get_item(TableName='snappy_history', Key=device_key)
    if hist_resp is not None and "Item" in hist_resp:
        history_entry = hist_resp["Item"]
        history_entry["last_contact"] = time_attrib
    else:
        history_entry = {**key, "last_contact": time_attrib, "readings": {"O": []}, "actions": {"O":[]}}
    db.put_item(TableName='snappy_history', Item=history_entry)

    # Configure the device if necessary.

    outgoing = None
    if "enabled" in device_entry and device_entry["enabled"]["N"] == "0":
        outgoing = {"enabled": 0}
    elif reading_interval != 0 and "reading_interval" in device_entry:
        device_interval = int(device_entry["reading_interval"]["N"])
        if reading_interval != device_interval:
            outgoing = {"reading_interval": device_interval}

    # Send the configuration with QoS1 to ensure that the message is received.  The device needs to
    # be prepared to receive several config commands after it connects, as a lingering command may
    # be delivered first followed by a new command.

    if outgoing is not None:
        iot_client = boto3.client('iot-data', region_name='eu-central-1')
        iot_client.publish(
            topic=f"snappy/control/{device_class}/{device}",
            qos=1,
            payload=json.dumps(outgoing)
        )
