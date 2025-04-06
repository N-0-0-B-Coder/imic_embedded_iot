import boto3
from botocore.exceptions import ClientError

# Configuration
AWS_IDENTITY = "343218212042"
AWS_REGION = "ap-southeast-1"
LAMBDA_FUNCTION_NAME = "IoT_MQTT_Sensor_Data"
IOT_RULE_NAME = "IoT_MQTT_Data_To_DynamoDB"
IOT_TOPIC = "/topic/data"
DYNAMODB_TABLE_PROVISIONING_NAME = "IoT_Provision_Table"
DYNAMODB_TABLE_DATA_NAME = "IoT_Sensor_Data"

# AWS Clients
iot = boto3.client("iot", region_name=AWS_REGION)
lambda_client = boto3.client("lambda", region_name=AWS_REGION)
dynamodb = boto3.client("dynamodb", region_name=AWS_REGION)

def create_dynamodb_table():
    try:
        dynamodb.describe_table(TableName=DYNAMODB_TABLE_PROVISIONING_NAME)
        print(f"DynamoDB table '{DYNAMODB_TABLE_PROVISIONING_NAME}' already exists.")
    except ClientError as e:
        if e.response["Error"]["Code"] == "ResourceNotFoundException":
            print("Creating DynamoDB table for provisioning...")
            dynamodb.create_table(
                TableName=DYNAMODB_TABLE_PROVISIONING_NAME,
                KeySchema=[
                    {"AttributeName": "device_id", "KeyType": "HASH"},
                ],
                AttributeDefinitions=[
                    {"AttributeName": "device_id", "AttributeType": "S"},
                ],
                ProvisionedThroughput={"ReadCapacityUnits": 1, "WriteCapacityUnits": 1},
            )
            print("Waiting for table to be created...")
            waiter = dynamodb.get_waiter("table_exists")
            waiter.wait(TableName=DYNAMODB_TABLE_PROVISIONING_NAME)
            print(f"Table '{DYNAMODB_TABLE_PROVISIONING_NAME}' created successfully.")
        else:
            raise

    try:
        dynamodb.describe_table(TableName=DYNAMODB_TABLE_DATA_NAME)
        print(f"DynamoDB table '{DYNAMODB_TABLE_DATA_NAME}' already exists.")
    except ClientError as e:
        if e.response["Error"]["Code"] == "ResourceNotFoundException":
            print("Creating DynamoDB table...")
            dynamodb.create_table(
                TableName=DYNAMODB_TABLE_DATA_NAME,
                KeySchema=[
                    {"AttributeName": "device_id", "KeyType": "HASH"},
                    {"AttributeName": "timestamp", "KeyType": "RANGE"},
                ],
                AttributeDefinitions=[
                    {"AttributeName": "device_id", "AttributeType": "S"},
                    {"AttributeName": "timestamp", "AttributeType": "N"},
                ],
                ProvisionedThroughput={"ReadCapacityUnits": 1, "WriteCapacityUnits": 1},
            )
            print("Waiting for table to be created...")
            waiter = dynamodb.get_waiter("table_exists")
            waiter.wait(TableName=DYNAMODB_TABLE_DATA_NAME)
            print(f"Table '{DYNAMODB_TABLE_DATA_NAME}' created successfully.")
        else:
            raise

def add_lambda_permission():
    try:
        lambda_client.add_permission(
            FunctionName=LAMBDA_FUNCTION_NAME,
            StatementId="AllowIoTInvoke",
            Action="lambda:InvokeFunction",
            Principal="iot.amazonaws.com",
            SourceArn=f"arn:aws:iot:{AWS_REGION}:{AWS_IDENTITY}:rule/{IOT_RULE_NAME}"  # Replace YOUR_ACCOUNT_ID
        )
        print("Lambda invoke permission added for IoT Core.")
    except ClientError as e:
        if e.response["Error"]["Code"] == "ResourceConflictException":
            print("Permission already exists.")
        else:
            raise

def create_iot_rule():
    try:
        # Use list_topic_rules() to check if the rule exists
        response = iot.list_topic_rules()
        existing_rules = [rule['ruleName'] for rule in response['rules']]
        if IOT_RULE_NAME in existing_rules:
            print(f"IoT rule '{IOT_RULE_NAME}' already exists.")
            return

        # If the rule does not exist, create it
        print("Creating IoT rule...")
        lambda_arn = lambda_client.get_function(FunctionName=LAMBDA_FUNCTION_NAME)["Configuration"]["FunctionArn"]
        topic_rule_payload = {
            "sql": f"SELECT * FROM '{IOT_TOPIC}'",
            "awsIotSqlVersion": "2016-03-23",
            "actions": [
                {
                    "lambda": {
                        "functionArn": lambda_arn
                    }
                }
            ],
            "ruleDisabled": False
        }
        iot.create_topic_rule(
            ruleName=IOT_RULE_NAME,
            topicRulePayload=topic_rule_payload
        )
        print(f"IoT rule '{IOT_RULE_NAME}' created successfully.")
    except ClientError as e:
        print(f"An error occurred: {e.response['Error']['Message']}")
        raise

def deploy():
    create_dynamodb_table()
    add_lambda_permission()
    create_iot_rule()

if __name__ == "__main__":
    deploy()
