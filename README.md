# MQTT Message Format

## Hardware
- ESP32 Devkit
- MPU6500

## SDK
- ESP32-IDF required for application building

## Language
- C
- Python with **boto3** and **json** required

## AWS
### Required services
- IAM (set up user/role with right permissions)
- IoT Core (MQTT)
- Lambda (BE handlers fpr provision, mqtt data)
- Dynamodb (Data storage)
- API Gateway (API handler for Lambda functions)
- Cloudwatch (Logging)

### Data example format for MQTT Data Lambda Function of XYZ value case
```json
{
  "created_at": 1703865660,
  "device": {
    "serial_number": "ESP32-001",
    "firmware_version": "1.0.0"
  },
  "data": [
    {
      "name": "acceleration",
      "value": { "x": 0.123, "y": -0.057, "z": 9.815 },
      "unit": "m/s^2",
      "timestamp": 1703865601
    },
    {
      "name": "angular_velocity",
      "value": { "x": 0.015, "y": -0.026, "z": 0.035 },
      "unit": "rad/s",
      "timestamp": 1703865602
    }
  ]
}
```

### Data example format for MQTT Data Lambda Function of normal value case
```json
{
  "created_at": 1703865660,
  "device": {
    "serial_number": "ESP32-001",
    "firmware_version": "1.0.0"
  },
  "data": [
    {
      "name": "temperature",
      "value": "25.3",
      "unit": "celsius",
      "series": "C",
      "timestamp": 1703865600
    },
    {
      "name": "humidity",
      "value": "54",
      "unit": "%",
      "timestamp": 1703865650
    }
  ]
}
```
