
let stats = {};
let currentChart = null;

document.addEventListener('DOMContentLoaded', async () => {
    try {
        const response = await fetch('health_stats.json');
        stats = await response.json();
        renderMetricList(Object.keys(stats));

        // Event listener for search
        document.getElementById('search').addEventListener('input', (e) => {
            const term = e.target.value.toLowerCase();
            const filtered = Object.keys(stats).filter(k => k.toLowerCase().includes(term));
            renderMetricList(filtered);
        });

        // Auto-select first metric
        const first = Object.keys(stats)[0];
        if (first) loadMetric(first);

    } catch (e) {
        console.error("Failed to load stats:", e);
    }
});

function renderMetricList(keys) {
    const list = document.getElementById('metricList');
    list.innerHTML = '';

    keys.sort().forEach(key => {
        const div = document.createElement('div');
        div.className = 'metric-item';
        div.textContent = key;
        div.onclick = () => loadMetric(key);
        div.dataset.key = key;
        list.appendChild(div);
    });
}

async function loadMetric(key) {
    // 1. Highlight list item
    document.querySelectorAll('.metric-item').forEach(el => el.classList.remove('active'));
    document.querySelector(`.metric-item[dataset-key="${key}"]`)?.classList.add('active');

    // 2. Update Header & Stats
    const info = stats[key];
    document.getElementById('metricTitle').textContent = key;
    document.getElementById('metricDesc').textContent = `Unit: ${info.unit || 'N/A'}`;

    const statsHtml = `
        <div class="card metric-card">
            <h3>Total Count</h3>
            <div class="value">${info.count.toLocaleString()}</div>
        </div>
        ${info.avg ? `
        <div class="card metric-card">
            <h3>Average</h3>
            <div class="value">${info.avg.toFixed(2)}</div>
            <div class="subtext">${info.unit}</div>
        </div>
        <div class="card metric-card">
            <h3>Max</h3>
            <div class="value">${info.max.toFixed(2)}</div>
        </div>
        <div class="card metric-card">
            <h3>Min</h3>
            <div class="value">${info.min.toFixed(2)}</div>
        </div>
        ` : ''}
    `;
    document.getElementById('statsGrid').innerHTML = statsHtml;

    // 3. Load Chart Data
    // Filename: lower + spaces to underscores
    const cleanName = key.toLowerCase().replace(/ /g, '_');
    const jsonPath = `data/${cleanName}.json`;

    try {
        const res = await fetch(jsonPath);
        if (!res.ok) throw new Error('No data file');
        const points = await res.json();

        renderChart(key, points, info.unit);
    } catch (e) {
        console.warn("Could not load chart data:", e);
        if (currentChart) currentChart.destroy();
    }
}

function renderChart(label, data, unit) {
    const ctx = document.getElementById('mainChart').getContext('2d');
    if (currentChart) currentChart.destroy();

    // Data format: [{d: "2023-...", v: 123}]
    const chartData = data.map(pt => ({
        x: pt.d, // string date
        y: pt.v
    }));

    currentChart = new Chart(ctx, {
        type: 'line',
        data: {
            datasets: [{
                label: label,
                data: chartData,
                borderColor: '#6366f1',
                backgroundColor: 'rgba(99, 102, 241, 0.1)',
                borderWidth: 2,
                pointRadius: chartData.length > 500 ? 0 : 2, // Hide points if too many
                fill: true,
                tension: 0.1
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: {
                    type: 'time',
                    time: {
                        unit: 'month'
                    },
                    grid: { color: '#333' },
                    ticks: { color: '#a0a0a0' }
                },
                y: {
                    grid: { color: '#333' },
                    ticks: { color: '#a0a0a0' }
                }
            },
            plugins: {
                legend: { display: false },
                tooltip: {
                    callbacks: {
                        label: (ctx) => `${ctx.raw.y} ${unit || ''}`
                    }
                }
            }
        }
    });
}
