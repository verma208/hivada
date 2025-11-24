import React, { useEffect, useState } from "react";
import { Line } from "react-chartjs-2";
import { useWebSocket } from "./hooks/useWebSocket";
import {
    Chart as ChartJS,
    CategoryScale,
    LinearScale,
    PointElement,
    LineElement,
    Title,
    Tooltip,
    Legend,
} from "chart.js";

// Register Chart.js modules
ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend);

function App() {
    const ecgValue = useWebSocket("insert websocket server URL");

    // Keep last N ECG values for plotting
    const [dataPoints, setDataPoints] = useState([]);
    const maxPoints = 100; // how many points to show on chart

    useEffect(() => {
        if (ecgValue !== null) {
            setDataPoints(prev => {
                const updated = [...prev, ecgValue];
                if (updated.length > maxPoints) updated.shift(); // keep array at maxPoints
                return updated;
            });
        }
    }, [ecgValue]);

    const data = {
        labels: dataPoints.map((_, i) => i), // simple index as x-axis
        datasets: [
            {
                label: "ECG Value",
                data: dataPoints,
                borderColor: "rgb(75, 192, 192)",
                backgroundColor: "rgba(75, 192, 192, 0.2)",
                tension: 0.3,
            },
        ],
    };

    const options = {
        responsive: true,
        animation: false, // disable animation for live updates
        scales: {
            y: {
                beginAtZero: false,
            },
        },
    };

    return (
        <div style={{ textAlign: "center", marginTop: "50px" }}>
            <h1>ECG Dashboard</h1>
            <h2>{ecgValue !== null ? `Current ECG Value: ${ecgValue}` : "Waiting for data..."}</h2>
            <div style={{ width: "80%", margin: "auto" }}>
                <Line data={data} options={options} />
            </div>
        </div>
    );
}

export default App;
