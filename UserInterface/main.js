const { app, BrowserWindow } = require('electron');
const path = require('path');

let mainWindow;

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 600, // Increased width for zones.html layout
        height: 400, // Increased height for zones.html layout
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
        },
    });

    mainWindow.loadFile('index.html'); // Ensure the app starts with index.html
}

app.whenReady().then(() => {
    createWindow();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

const fs = require('fs');
const { exec } = require('child_process');

function collectAndSendData() {
    // Mock the variables as examples; replace with actual DOM queries or state.
    const master = true;
    const sensor = parseFloat(localStorage.getItem('moistureMultiplier')) || 0.9; // Example sensor multiplier
    const systemContainer = localStorage.getItem('selectedSystem') || "Demo"; // Example system name
    const numberOfZones = parseInt(localStorage.getItem('zoneCount')) || 8; // Example zone count

    const zones = [];
    for (let i = 1; i <= numberOfZones; i++) {
        // Assume a DOM structure where each zone button's ID is 'zone{number}'.
        const zoneButton = document.getElementById(`zone${i}`);
        if (zoneButton) {
            zones.push(zoneButton.textContent.includes('ON') ? 1 : 0);
        }
    }

    const data = {
        master: master,
        sensor: sensor,
        "system-container": systemContainer,
        numberOfZones: numberOfZones,
        zones: zones
    };

    // Write JSON to a file
    const filePath = './status.json';
    fs.writeFileSync(filePath, JSON.stringify(data, null, 2));

    // Use curl to POST the JSON
    const curlCommand = `curl -X POST http://shanehurley.com:8001/ -H "Content-Type: application/json" -d @${filePath}`;
    exec(curlCommand, (error, stdout, stderr) => {
        if (error) {
            console.error(`Error executing curl: ${error.message}`);
            return;
        }
        console.log(`Curl output: ${stdout}`);
        if (stderr) {
            console.error(`Curl errors: ${stderr}`);
        }
    });
}

// Add a way to trigger the function, e.g., a button click in the application
document.addEventListener('DOMContentLoaded', () => {
    const sendButton = document.createElement('button');
    sendButton.textContent = 'Send Data';
    sendButton.addEventListener('click', collectAndSendData);
    document.body.appendChild(sendButton);
});
