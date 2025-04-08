const API_URL = "https://h73hgmob52.execute-api.ap-southeast-1.amazonaws.com/sensor-data";

let velocityChart, frequencyChart;

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

      const timestamps = data.map(d =>
        new Date(d.timestamp * 1000).toLocaleString()
      );
      const velocityValues = data.map(d => parseFloat(d.velocity_value));
      const frequencyValues = data.map(d => parseFloat(d.frequency_value));

      renderChart("velocityChart", "Velocity (km/h)", timestamps, velocityValues, "blue", velocityChart => {
        velocityChart = velocityChart;
      });

      renderChart("frequencyChart", "Frequency (Hz)", timestamps, frequencyValues, "green", frequencyChart => {
        frequencyChart = frequencyChart;
      });
    })
    .catch(err => {
      console.error("Fetch error:", err);
      alert("Failed to fetch sensor data.");
    });
}

function renderChart(canvasId, label, labels, data, color, onInit) {
  const ctx = document.getElementById(canvasId).getContext("2d");

  // Destroy previous chart if exists
  if (canvasId === "velocityChart" && velocityChart) velocityChart.destroy();
  if (canvasId === "frequencyChart" && frequencyChart) frequencyChart.destroy();

  const chart = new Chart(ctx, {
    type: "line",
    data: {
      labels,
      datasets: [
        {
          label,
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
          ticks: {
            maxRotation: 90,
            minRotation: 45,
            autoSkip: true,
            maxTicksLimit: 15
          }
        }
      }
    }
  });

  onInit(chart);
}
