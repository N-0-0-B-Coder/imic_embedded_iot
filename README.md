# MQTT Message Format
### XYZ format
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

### Normal value format
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
