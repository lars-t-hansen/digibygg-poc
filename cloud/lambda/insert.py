from influxdb import InfluxDBClient
import os

port = '8086'

host = os.environ['HOST_IP']
user = os.environ['USERNAME']
password = os.environ['PASSWORD']
database = os.environ['DATABASE']

client = InfluxDBClient(host, port, user, password, database)

def lambda_handler(event, context):
    client.write_points([event])
    print(event)
    return 'hello'
