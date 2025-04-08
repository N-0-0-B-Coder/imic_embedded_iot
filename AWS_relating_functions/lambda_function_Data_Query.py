import json
import boto3
from decimal import Decimal
from boto3.dynamodb.conditions import Key
from datetime import datetime, timedelta

dynamodb = boto3.resource("dynamodb")
TABLE_NAME = "IoT_Sensor_Data"
table = dynamodb.Table(TABLE_NAME)

def decimal_default(obj):
    if isinstance(obj, Decimal):
        return float(obj)
    raise TypeError

def lambda_handler(event, context):
    print("Event received:", json.dumps(event))
    try:
        params = event.get("queryStringParameters") or {}
        device_id = params.get("device_id")
        timeframe = params.get("timeframe", "1h")  # Options: 1h, 12h, 1d

        if not device_id:
            return {"statusCode": 400, "body": "Missing device_id"}

        # Convert timeframe to epoch
        now = int(datetime.now().timestamp())
        hours_map = {"1h": 1, "12h": 12, "1d": 24}
        hours = hours_map.get(timeframe, 1)
        start_time = int((datetime.now() - timedelta(hours=hours)).timestamp())

        # Query DynamoDB
        response = table.query(
            KeyConditionExpression=Key("device_id").eq(device_id) & Key("timestamp").gte(start_time),
            ScanIndexForward=True  # Oldest to newest
        )

        print("JSON items: ", json.dumps(response["Items"], default=decimal_default))

        return {
            "statusCode": 200,
            "headers": {"Access-Control-Allow-Origin": "*"},
            "body": json.dumps(response["Items"], default=decimal_default)
        }

    except Exception as e:
        print(e)
        return {
            "statusCode": 500,
            "body": json.dumps({"error": str(e)})
        }
