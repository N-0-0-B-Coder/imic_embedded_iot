import json
import boto3
import zlib

s3_client = boto3.client('s3', region_name='ap-southeast-1')
iot_client = boto3.client("iot-data", region_name="ap-southeast-1")


BUCKET_NAME = "esp32-firmware-storage"
FIRMWARE_KEY = "iot_esp32_ota.bin"

def lambda_handler(event, context):
    #print("Incoming event:", json.dumps(event))  # Log incoming event for debugging

    try:
        http_method = event["requestContext"]["http"]["method"]

        #handle CORS preflight (OPTIONS request)
        if http_method == "OPTIONS":
            return {
                "statusCode": 200,
                "headers": {
                    "Access-Control-Allow-Origin": "*",
                    "Access-Control-Allow-Methods": "OPTIONS,POST,GET",
                    "Access-Control-Allow-Headers": "Content-Type"
                },
                "body": json.dumps("CORS preflight response")
            }

        #handle POST request
        body = {}
        if event.get("body"):
            try:
                body = json.loads(event["body"])
            except ValueError:
                # If body is not valid JSON
                return {
                    "statusCode": 400,
                    "headers": {
                        "Access-Control-Allow-Origin": "*",
                        "Access-Control-Allow-Methods": "OPTIONS,POST,GET",
                        "Access-Control-Allow-Headers": "Content-Type"
                    },
                    "body": json.dumps({"error": "Invalid JSON in request body"})
                }
        
        device_id = body.get("device_id")
        command = body.get("command")

        if not device_id or not command:
            return {
                "statusCode": 400,
                "headers": {
                    "Access-Control-Allow-Origin": "*",
                    "Access-Control-Allow-Methods": "OPTIONS,POST,GET",
                    "Access-Control-Allow-Headers": "Content-Type"
                },
                "body": json.dumps({"error": "Missing device id or command"})
            }

        if command.lower() != "ota":
            return {
                "statusCode": 400,
                "headers": {
                    "Access-Control-Allow-Origin": "*",
                    "Access-Control-Allow-Methods": "OPTIONS,POST,GET",
                    "Access-Control-Allow-Headers": "Content-Type"
                },
                "body": json.dumps({"error": "Not a OTA command"})
            }


        
        try:

            # Fetch firmware binary from S3
            fw_response = s3_client.get_object(Bucket=BUCKET_NAME, Key=FIRMWARE_KEY)
            fw_data = fw_response['Body'].read()

            # Calculate CRC32 of the binary
            crc32 = zlib.crc32(fw_data) & 0xFFFFFFFF

            # Generate a presigned URL for the firmware file, valid for 120 seconds
            signed_url = s3_client.generate_presigned_url(
                'get_object',
                Params={'Bucket': BUCKET_NAME, 'Key': FIRMWARE_KEY},
                ExpiresIn=120
            )

        except Exception as e:
            print(f"ERROR generating presigned URL or CRC32: {e}")
            return {
                "statusCode": 500,
                "headers": {"Access-Control-Allow-Origin": "*"},
                "body": json.dumps({"error": "Failed to prepare OTA data"})
            }

        # Prepare MQTT message
        message = {
            "command": "ota",
            "fw_url": signed_url,
            "fw_crc": crc32
        }

        try:
            iot_client.publish(
                topic=f"/topic/command/{device_id}",
                qos=1,
                payload=json.dumps(message)
            )
        except Exception as e:
            print(f"ERROR publishing MQTT message: {e}")
            return {
                "statusCode": 500,
                "headers": {
                    "Access-Control-Allow-Origin": "*",
                    "Access-Control-Allow-Methods": "OPTIONS,POST,GET",
                    "Access-Control-Allow-Headers": "Content-Type"
                    },
                "body": json.dumps({"error": "Failed to publish MQTT message"})
            }

        return {
            "statusCode": 200,
            "headers": {
                "Access-Control-Allow-Origin": "*",
                "Access-Control-Allow-Methods": "OPTIONS,POST,GET",
                "Access-Control-Allow-Headers": "Content-Type"
            },
            "body": json.dumps({
                "statusCode": 200,
                "message": "OTA command sent successfully", 
                "fw_url": signed_url,
                "fw_crc": crc32
            })
        }
    except Exception as e:
        return {
            "statusCode": 500,
            "headers": {
                "Access-Control-Allow-Origin": "*",
                "Access-Control-Allow-Methods": "OPTIONS,POST,GET",
                "Access-Control-Allow-Headers": "Content-Type"
            },
            "body": json.dumps({"error": str(e)})
        }
