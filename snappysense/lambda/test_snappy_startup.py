import boto3
from moto import mock_dynamodb

# Moto appears basically broken - every attempt to compartementalize
# any of this logic has failed, whether with fixtures or with
# annotated subroutines.  So for now it's all in one function.

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
    dynamodb.put_item(TableName='snappy_location', Item={"location": {"S": "lars-at-home"}})


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
                            "location":{"S":"lars-at-home"}})
    # "sensors":[], "actuators":["temperature"]
    dynamodb.put_item(TableName='snappy_device',
                      Item={"device":{"S":"2"},
                            "class":{"S":"Thermostat"},
                            "location":{"S":"lars-at-home"}})

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
    import snappy_data

    # The reading interval is different from the default so as to provoke a sent message
    assert 4 != snappy_data.DEFAULT_READING_INTERVAL
    responses = snappy_startup.handle_startup_event(dynamodb, {"device":"1", "class":"RPi2B+", "time":12345, "reading_interval":4}, {})

    # There should be a control message that changes the reading interval for this device
    assert len(responses) == 1
    r0 = responses[0]
    assert len(r0) == 3
    assert r0[0] == "snappy/control/RPi2B+/1"
    payload = r0[1]
    assert "reading_interval" in payload
    print(payload)
    assert payload["reading_interval"] == snappy_data.DEFAULT_READING_INTERVAL
    assert r0[2] == 1
    
