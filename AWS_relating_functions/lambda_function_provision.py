import json
import boto3
import requests

iot_client = boto3.client("iot")
dynamodb = boto3.resource("dynamodb")

# DynamoDB Table
TABLE_NAME = "IoT_Provision_Table"
table = dynamodb.Table(TABLE_NAME)

# Define policy name
POLICY_NAME = "IoT_Policy"

POLICY_DOCUMENT = {
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "iot:Connect"
            ],
            "Resource": "arn:aws:iot:ap-southeast-1:343218212042:client/${iot:Connection.Thing.ThingName}"
        },
        {
            "Effect": "Allow",
            "Action": [
                "iot:Publish",
                "iot:Subscribe",
                "iot:Receive"
            ],
            "Resource": "arn:aws:iot:ap-southeast-1:343218212042:topic/*"
        }
    ]
}


def lambda_handler(event, context):
    try:
        data = json.loads(event["body"])
        device_id = data.get("device_id")

        if not device_id:
            return {
                "statusCode": 400, 
                "body": json.dumps({
                    "error": "Missing device ID"
                })
            }

        http_path = event["requestContext"]["http"]["path"]
        http_method = event["requestContext"]["http"]["method"]

        if http_method == "POST":
            if http_path == "/provisioning":
                return provision_device(device_id)
            
            elif http_path == "/unprovisioning":
                return unprovision_device(device_id)

            return {
                "statusCode": 400, 
                "body": json.dumps({
                    "error": "Invalid API path", 
                    "received_path": http_path
                })
            }
        else:
            return {
                "statusCode": 400, 
                "body": json.dumps({
                    "error": "Invalid HTTP method", 
                    "received_method": http_method
                })
            }

    except Exception as e:
        print("Error:", str(e))
        return {
            "statusCode": 500, 
            "body": json.dumps({
                "message": "Internal Server Error", 
                "error": str(e)
            })
        }


def check_or_create_policy():
    """Check if the IoT policy exists. If not, create it."""
    try:
        # Check if policy exists
        iot_client.get_policy(policyName=POLICY_NAME)
        print(f"Policy '{POLICY_NAME}' already exists.")
    except iot_client.exceptions.ResourceNotFoundException:
        # Policy does not exist, create it
        print(f"Policy '{POLICY_NAME}' not found. Creating policy...")
        iot_client.create_policy(
            policyName=POLICY_NAME,
            policyDocument=json.dumps(POLICY_DOCUMENT)
        )
        print(f"Policy '{POLICY_NAME}' created successfully.")

def provision_device(device_id):
    """Provision a new IoT device."""
    try:
        # Check if device already exists in DynamoDB
        response = table.get_item(Key={"device_id": device_id})
        if "Item" in response:
            existing_data = response["Item"]
            return {
                "statusCode": 200,
                "body": json.dumps({
                    "message": "Device already provisioned",
                    "root_ca": existing_data["root_ca"],
                    "device_cert": existing_data["certificate_pem"],
                    "private_key": existing_data["private_key"],
                    "public_key": existing_data["public_key"]
                })
            }

        check_or_create_policy()

        root_ca_url = "https://www.amazontrust.com/repository/AmazonRootCA1.pem"
        root_ca_pem = requests.get(root_ca_url).text


        # Create a new certificate
        cert_response = iot_client.create_keys_and_certificate(setAsActive=True)
        certificate_arn = cert_response["certificateArn"]
        certificate_id = cert_response["certificateId"]
        private_key = cert_response["keyPair"]["PrivateKey"]
        public_key = cert_response["keyPair"]["PublicKey"]
        certificate_pem = cert_response["certificatePem"]

        # Attach policy to the certificate
        iot_client.attach_policy(policyName=POLICY_NAME, target=certificate_arn)

        # Create IoT Thing
        iot_client.create_thing(thingName=device_id)
        iot_client.attach_thing_principal(thingName=device_id, principal=certificate_arn)

        # Store in DynamoDB
        table.put_item(Item={
            "device_id": device_id,
            "certificate_id": certificate_id,
            "certificate_arn": certificate_arn,
            "root_ca": root_ca_pem,
            "device_cert": certificate_pem,
            "private_key": private_key,
            "public_key": public_key
        })

        return {
            "statusCode": 200,
            "body": json.dumps({
                "message": "Device provisioned successfully",
                "root_ca": root_ca_pem,
                "device_cert": certificate_pem,
                "private_key": private_key,
                "public_key": public_key
            })
        }

    except Exception as e:
        print("Error during provisioning:", str(e))
        return {
            "statusCode": 500, 
            "body": json.dumps({
                "error": str(e)
            })
        }


def unprovision_device(device_id):
    """Unprovision a exist IoT device."""
    # Check if device exists in DynamoDB
    response = table.get_item(Key={"device_id": device_id})
    if "Item" not in response:
        return {
            "statusCode": 404, 
            "body": json.dumps({
                "error": "Device not found"
            })
        }

    item = response["Item"]
    certificate_id = item["certificate_id"]
    certificate_arn = item["certificate_arn"]

    try:
        # Detach the policy from the certificate
        try:
            iot_client.detach_policy(policyName=POLICY_NAME, target=certificate_arn)
        except iot_client.exceptions.ResourceNotFoundException:
            print("Policy already detached.")

        # Detach the certificate from the Thing (only if it exists)
        try:
            iot_client.detach_thing_principal(thingName=device_id, principal=certificate_arn)
        except iot_client.exceptions.ResourceNotFoundException:
            print(f"Principal {certificate_arn} does not exist, skipping detachment.")

        # Deactivate and delete the certificate if it exists
        try:
            iot_client.update_certificate(certificateId=certificate_id, newStatus="INACTIVE")
            iot_client.delete_certificate(certificateId=certificate_id, forceDelete=True)
        except iot_client.exceptions.ResourceNotFoundException:
            print(f"Certificate {certificate_id} does not exist, skipping deletion.")


        # Finally, delete the Thing
        iot_client.delete_thing(thingName=device_id)

        # Remove the device from DynamoDB
        table.delete_item(Key={"device_id": device_id})

        return {
            "statusCode": 200, 
            "body": json.dumps({
                "message": "Device unprovisioned successfully"
            })
        }

    except Exception as e:
        print("Error during unprovisioning:", str(e))
        return {
            "statusCode": 500, 
            "body": json.dumps({
                "error": str(e)
            })
        }

