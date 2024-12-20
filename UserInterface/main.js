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
            contextIsolation: true, // Ensure security
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

// Listen for zone data from the renderer process
ipcMain.on('send-data', (event, data) => {
    const filePath = path.join(__dirname, 'status.json');
    fs.writeFileSync(filePath, JSON.stringify(data, null, 2));

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
});

// IPC listener for data from renderer process
ipcMain.on('send-data', (event, data) => {
    const filePath = path.join(__dirname, 'status.json');
    fs.writeFileSync(filePath, JSON.stringify(data, null, 2));

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
});
