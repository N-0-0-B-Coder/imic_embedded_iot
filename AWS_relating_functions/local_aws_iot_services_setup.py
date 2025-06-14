import boto3
from botocore.exceptions import ClientError

# Configuration
AWS_IDENTITY = "343218212042"
AWS_REGION = "ap-southeast-1"
LAMBDA_FUNCTION_NAME1 = "IoT_MQTT_Sensor_Data"
LAMBDA_FUNCTION_NAME2 = "IoT_MQTT_OTA"
IOT_RULE_NAME1 = "IoT_MQTT_Data_To_DynamoDB"
IOT_RULE_NAME2 = "IoT_MQTT_OTA"
IOT_TOPIC1 = "/topic/data"
IOT_TOPIC2 = "/topic/ota"
IOT_TOPIC = f"{IOT_TOPIC1} || {IOT_TOPIC2}"  # Combine topics for the rule
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
        rule_names = [IOT_RULE_NAME1, IOT_RULE_NAME2]
        lambda_functions = [LAMBDA_FUNCTION_NAME1, LAMBDA_FUNCTION_NAME2]
        
        for rule_name, lambda_function in zip(rule_names, lambda_functions):
            lambda_client.add_permission(
                FunctionName=lambda_function,
                StatementId=f"AllowIoTInvoke-{rule_name}",
                Action="lambda:InvokeFunction",
                Principal="iot.amazonaws.com",
                SourceArn=f"arn:aws:iot:{AWS_REGION}:{AWS_IDENTITY}:rule/{rule_name}"
            )
            print(f"Lambda invoke permission added for IoT Core rule '{rule_name}' and Lambda function '{lambda_function}'.")
    except ClientError as e:
        if e.response["Error"]["Code"] == "ResourceConflictException":
            print("Permission already exists.")
        else:
            raise

def create_iot_rules():
    try:
        # Use list_topic_rules() to check if the rules exist
        response = iot.list_topic_rules()
        existing_rules = [rule['ruleName'] for rule in response['rules']]

        rules = [
            {"name": IOT_RULE_NAME1, "topic": IOT_TOPIC1, "lambda_function": LAMBDA_FUNCTION_NAME1},
            {"name": IOT_RULE_NAME2, "topic": IOT_TOPIC2, "lambda_function": LAMBDA_FUNCTION_NAME2}
        ]

        for rule in rules:
            if rule["name"] in existing_rules:
                print(f"IoT rule {rule['name']} already exists.")
            else:
                print(f"Creating IoT rule {rule['name']}...")
                lambda_arn = lambda_client.get_function(FunctionName=rule["lambda_function"])["Configuration"]["FunctionArn"]
                topic_rule_payload = {
                    "sql": f"SELECT * FROM '{rule['topic']}'",
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
                    ruleName=rule["name"],
                    topicRulePayload=topic_rule_payload
                )
                print(f"IoT rule {rule['name']} created successfully.")

    except ClientError as e:
        print(f"An error occurred: {e.response['Error']['Message']}")
        raise

def deploy():
    create_dynamodb_table()
    create_iot_rules()
    add_lambda_permission()

if __name__ == "__main__":
    deploy()
