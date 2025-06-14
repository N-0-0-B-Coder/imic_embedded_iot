import json
import boto3
from botocore.exceptions import ClientError
from decimal import Decimal

# AWS Clients
iot_client = boto3.client("iot")
dynamodb = boto3.resource("dynamodb")

# Table and Topic Details
TABLE_NAME = "IoT_Sensor_Data"

# Function to check and create the table
def check_table():
    try:
        table = dynamodb.Table(TABLE_NAME)
        table.load()  # Check if the table exists
        #print(f"Table {TABLE_NAME} already exists.")
    except ClientError as e:
        if e.response["Error"]["Code"] == "ResourceNotFoundException":
            print(f"Table {TABLE_NAME} not found. Creating table...")
            dynamodb.create_table(
                TableName=TABLE_NAME,
                KeySchema=[
                    {"AttributeName": "device_id", "KeyType": "HASH"},  # Partition Key
                    {"AttributeName": "timestamp", "KeyType": "RANGE"},  # Sort Key
                ],
                # Define data type
                AttributeDefinitions=[
                    {"AttributeName": "device_id", "AttributeType": "S"},
                    {"AttributeName": "timestamp", "AttributeType": "N"},
                ],
                # Dynamodb capacity setting, prefer free tier price page
                ProvisionedThroughput={"ReadCapacityUnits": 5, "WriteCapacityUnits": 5}, 
            )
            print(f"Table {TABLE_NAME} created successfully.")
        else:
            raise  # Raise unexpected errors

# Helper to convert float -> Decimal recursively
def replace_floats(obj):
    if isinstance(obj, float):
        return Decimal(str(obj))
    elif isinstance(obj, dict):
        return {k: replace_floats(v) for k, v in obj.items()}
    elif isinstance(obj, list):
        return [replace_floats(elem) for elem in obj]
    else:
        return obj


def lambda_handler(event, context):
    try:
        #print("Incoming event:", json.dumps(event))  # Log incoming event for debugging

        check_table()

        # Extract data from the incoming event
        created_at = event.get("created_at")
        device_info = event.get("device", {})
        data_entries = event.get("data", [])

        # Get device details
        device_id = device_info.get("serial_number", "Unknown_Device")
        firmware_version = device_info.get("firmware_version", "Unknown")

        table = dynamodb.Table(TABLE_NAME)

        # Initialize the main item
        item = {
            "device_id": device_id,
            "timestamp": created_at,
            "firmware_version": firmware_version,
        }

        # Add all sensor data to the item
        for entry in data_entries:
            sensor_name = entry.get("name")
            sensor_value = entry.get("value")
            if sensor_name:
                ts = entry.get("timestamp", created_at)
                item[f"{sensor_name}_timestamp"] = ts
                if isinstance(sensor_value, dict):  # Vector data like x, y, z
                    for axis in ["x", "y", "z"]:
                        item[f"{sensor_name}_{axis}"] = sensor_value.get(axis)
                else:
                    item[f"{sensor_name}_value"] = sensor_value
                item[f"{sensor_name}_unit"] = entry.get("unit", "")
                item[f"{sensor_name}_series"] = entry.get("series", "")
        # Convert from float to decimal since dynamodb don't support float type
        item = replace_floats(item)
        # Store the combined item in DynamoDB
        table.put_item(Item=item)

        return {"statusCode": 200, "body": "Data stored successfully"}

    except Exception as e:
        print(f"Error processing IoT message: {str(e)}")
        return {"statusCode": 500, "body": str(e)}
