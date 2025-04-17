const API_URL = "https://h73hgmob52.execute-api.ap-southeast-1.amazonaws.com/sensor-data";
const API_OTA = "https://xm3bpiqje4.execute-api.ap-southeast-1.amazonaws.com/ota";

// Chart instances
let velocityChartInstance = null;
let frequencyChartInstance = null;

function renderChart(canvasId, label, labels, data, unit, color, existingChart) {
  const ctx = document.getElementById(canvasId).getContext("2d");

  if (existingChart) {
    existingChart.destroy();
  }

  return new Chart(ctx, {
    type: "line",
    data: {
      labels,
      datasets: [
        {
          label: `${label} (${unit})`,
          data,
          fill: false,
          borderColor: color,
          tension: 0.1,
        },
      ],
    },
    options: {
      responsive: true,
      scales: {
        x: {
          title: {
            display: true,
            text: "Time"
          },
          ticks: {
            maxRotation: 90,
            minRotation: 45,
            autoSkip: true,
            maxTicksLimit: 15
          }
        },
        y: {
          title: {
            display: true,
            text: unit
          }
        }
      }
    }
  });
}

function fetchAndRender() {
  const deviceId = document.getElementById("deviceId").value.trim();
  const timeframe = document.getElementById("timeframe").value;

  if (!deviceId) {
    alert("Please enter a device ID.");
    return;
  }

  const url = `${API_URL}?device_id=${deviceId}&timeframe=${timeframe}`;

  fetch(url)
    .then(res => res.json())
    .then(data => {
      if (!Array.isArray(data)) {
        alert("Error: Invalid data returned from server.");
        return;
      }

      if (data.length === 0) {
        alert("No data available for this device and timeframe.");
        return;
      }

      const timestamps = data.map(d =>
        new Date(d.timestamp * 1000).toLocaleString()
      );

      const velocityValues = data.map(d => parseFloat(d.velocity_value));
      const frequencyValues = data.map(d => parseFloat(d.frequency_value));

      const velocityUnit = data[0].velocity_unit || "km/h";
      const frequencyUnit = data[0].frequency_unit || "Hz";

      // Render both charts with instance tracking
      velocityChartInstance = renderChart("velocityChart", "Velocity", timestamps, velocityValues, velocityUnit, "blue", velocityChartInstance);
      frequencyChartInstance = renderChart("frequencyChart", "Frequency", timestamps, frequencyValues, frequencyUnit, "green", frequencyChartInstance);
    })
    .catch(err => {
      console.error("Fetch error:", err);
      alert("Failed to fetch sensor data.");
    });
}

function ota() {
  const deviceId = document.getElementById("deviceId").value.trim();
  const url = API_OTA;

  if (!deviceId) {
    alert("Please enter a device ID.");
    return;
  }

  fetch(url, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify({ command: "ota", device_id: deviceId })
  })
    .then(res => res.json())
    .then(data => {
      if (data.statusCode === 200) {
        alert("OTA update initiated successfully.");
      } else {
        alert("Failed to initiate OTA update.");
      }
    })
    .catch(err => {
      console.error("OTA error:", err);
      alert("Failed to initiate OTA update.");
    });
}
