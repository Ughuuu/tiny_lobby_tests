const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');

// Configuration
const serverUrl = 'ws://localhost:9001/connect'; // Your WebSocket server URL
const numClients = 5000;  // Number of WebSocket clients to simulate
const messagesPerClient = 0; // Number of messages each client will send. Set to 0 for infinite
const messageInterval = 1;  // Interval in milliseconds between messages
const max_time = 10000; // 10 s
let clientCount = 0;
let clientErrors = 0;
let messagesSent = 0;
let messagesReceived = 0;

// CSV File path
const csvFilePath = path.join(__dirname, 'stress_test_results.csv');

// Write CSV header
fs.writeFileSync(csvFilePath, 'timestamp,client_count,client_errors,messages_sent,messages_received\n');

// Function to start a WebSocket client
function startClient(clientId) {
    const ws = new WebSocket(serverUrl, ['blazium', 'echo']);

    ws.on('open', () => {
        clientCount++;

        ws.send(JSON.stringify({
            "command": "quick_join"
        }));

        let messageCount = 0;
        const messageIntervalId = setInterval(() => {
            // Send a message to the server
            const message = `Client ${clientId} - Message ${messageCount + 1}`;
            ws.send(JSON.stringify({
                "command": "lobby_call",
                "data": { "count": messageCount }
            }));

            messagesSent++;

            messageCount++;

            // Stop sending after the specified number of messages
            if (messageCount >= messagesPerClient && messagesPerClient !== 0) {
                clearInterval(messageIntervalId);
                ws.close();
            }
        }, messageInterval);
    });

    ws.on('message', (message) => {
        messagesReceived++;
    });

    ws.on('close', () => {
        clientCount--;
    });

    ws.on('error', (error) => {
        console.log(error)
        clientErrors++;
    });
}

// Start multiple WebSocket clients
function startStressTest() {
    for (let i = 0; i < numClients; i++) {
        startClient(i + 1);
    }
}

// Run the stress test
startStressTest();

let start = Date.now();

// Interval to log stats and write to CSV
const logInterval = setInterval(() => {
    let timestamp = Date.now() - start;
    const line = `${timestamp},${clientCount},${clientErrors},${messagesSent},${messagesReceived}\n`;

    // Append line to CSV
    fs.appendFileSync(csvFilePath, line);

    // Log to console
    console.log(`Timestamp: ${timestamp} | Client Count: ${clientCount} | Client Errors: ${clientErrors} | Messages Sent: ${messagesSent} | Messages Received: ${messagesReceived}`);

    // Stop the test after max_time
    if (timestamp > max_time) {
        clearInterval(logInterval);
        console.log('Stress test completed. Results saved to stress_test_results.csv');
        process.exit();
    }
}, 1000);
