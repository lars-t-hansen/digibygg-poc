# -*- fill-column: 100 -*-
#
# MQTT stuff for SnappySense.
#
# See ../mqtt-protocol.md for a description of the messages.
# See ../data-model.md for a description of the databases.

import boto3
import json

def publish(topic, payload, qos):
    iot_client = boto3.client('iot-data', region_name='eu-central-1')
    iot_client.publish(
        topic=topic,
        qos=qos,
        payload=json.dumps(payload)
    )
