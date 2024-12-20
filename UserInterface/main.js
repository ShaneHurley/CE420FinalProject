const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');

let mainWindow;

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 600,
        height: 400,
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            nodeIntegration: false,
        },
    });

    mainWindow.loadFile('index.html');
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

function gatherData() {
    const status = {
        master: true,
        sensor: parseFloat(localStorage.getItem('moistureMultiplier')) || 0.9,
        systemContainer: localStorage.getItem('selectedSystem') || 'Demo',
        numberOfZones: parseInt(localStorage.getItem('zoneCount')) || 8,
        zones: Array.from({ length: 9 }, (_, i) => {
            const zoneState = localStorage.getItem(`zone${i + 1}`);
            return zoneState === '1' ? 1 : 0;
        }),
    };

    const filePath = path.join(__dirname, 'status.json');
    fs.writeFileSync(filePath, JSON.stringify(status, null, 2));

    const curlCommand = `curl -X POST http://shanehurley.com:8001/ -H "Content-Type: application/json" -d @${filePath}`;
    exec(curlCommand, (error, stdout, stderr) => {
        if (error) console.error(`Error: ${error.message}`);
        if (stdout) console.log(`Output: ${stdout}`);
        if (stderr) console.error(`Stderr: ${stderr}`);
    });
}

// Automatically send data every 5 seconds
setInterval(gatherData, 5000);
