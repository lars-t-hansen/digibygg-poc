# -*- fill-column: 100 -*-

import boto3
from moto import mock_dynamodb

# Cloned from test_snappy_startup, q.v.

@mock_dynamodb
def test_startup():
    dynamodb = boto3.client('dynamodb', region_name='eu-central-1')

    # Create and initialize LOCATION

    dynamodb.create_table(
        AttributeDefinitions=[
            {"AttributeName":"location", "AttributeType":"S"}
        ],
        TableName = 'snappy_location',
        KeySchema=[
            {"AttributeName":"location", "KeyType":"HASH"}
        ],
        BillingMode="PAY_PER_REQUEST"
    )
    dynamodb.put_item(TableName='snappy_location',
                      Item={"location": {"S": "lars-at-home"},
                            "sensors": {"L": [{"S": "1"}, {"S": "3"}]},
                            "actuators": {"L": [{"M": {"factor": {"S":"temperature"},
                                                       "device": {"S":"2"},
                                                       "idealfn": {"S":"constant/21"}}}]}})

    # Create and initialize DEVICE

    dynamodb.create_table(
        AttributeDefinitions=[
            {"AttributeName":"device", "AttributeType":"S"},
        ],
        TableName = 'snappy_device',
        KeySchema=[
            {"AttributeName":"device", "KeyType":"HASH"}
        ],
        BillingMode="PAY_PER_REQUEST"
    )
    dynamodb.put_item(TableName='snappy_device',
                      Item={"device":{"S":"1",},
                            "class":{"S":"RPi2B+"},
                            "location":{"S":"lars-at-home"},
                            "sensors":{"L":[{"S":"temperature"}]},
                            "actuators":{"L":[]}})
    # "sensors":[], "actuators":["temperature"]
    dynamodb.put_item(TableName='snappy_device',
                      Item={"device":{"S":"2"},
                            "class":{"S":"Thermostat"},
                            "location":{"S":"lars-at-home"},
                            "sensors":{"L":[]},
                            "actuators":{"L":[{"S":"temperature"}]}})
    dynamodb.put_item(TableName='snappy_device',
                      Item={"device":{"S":"3",},
                            "class":{"S":"MBPM1Max"},
                            "location":{"S":"lars-at-home"},
                            "enabled":{"N":"0"},
                            "sensors":{"L":[{"S":"temperature"}]},
                            "actuators":{"L":[]}})

    # Create and initialize HISTORY

    dynamodb.create_table(
        AttributeDefinitions=[
            {"AttributeName":"device", "AttributeType":"S"},
        ],
        TableName='snappy_history',
        KeySchema=[
            {"AttributeName":"device", "KeyType":"HASH"}
        ],
        BillingMode="PAY_PER_REQUEST"
    )

    # I don't know if this needs to be here but to be conservative, it is.

    import snappy_startup
    import snappy_reading
    import snappy_data

    # We need a setup message to ensure that there's a history element, this is part of the
    # protocol.  The code is resilient in the face of a failure in this, but the tests below are not
    # (and should not be).

    responses = snappy_startup.handle_startup_event(dynamodb, {"device":"1",
                                                               "class":"RPi2B+",
                                                               "time":1,
                                                               "reading_interval":0},
                                                    {})

    #
    # This should work but there should be no response, as the reading is already at
    # the ideal for the location.
    #

    h1 = snappy_data.get_history_entry(dynamodb, "1")
    assert snappy_data.history_last_contact(h1) == 1
    r1 = snappy_data.history_readings(h1)
    assert len(r1) == 0

    responses = snappy_reading.handle_reading_event(dynamodb, {"device":"1",
                                                               "class":"RPi2B+",
                                                               "time":12345,
                                                               "factor":"temperature",
                                                               "reading":21},
                                                    {})
    assert len(responses) == 0

    h1 = snappy_data.get_history_entry(dynamodb, "1")
    assert snappy_data.history_last_contact(h1) == 12345
    r1 = snappy_data.history_readings(h1)
    assert len(r1) == 1
    assert r1[0]["temperature"] == 21

    # TODO: Here we can assert that:
    # - the list of actions for the temperature actuator at the location is still empty
