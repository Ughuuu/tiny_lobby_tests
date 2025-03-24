const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');
const { error } = require('console');

// Configuration
const serverUrl = 'ws://localhost:8080/connect'; // Your WebSocket server URL
const numClients = 5000;  // Number of WebSocket clients to simulate
let stopAfter = 0; // Number of messages each client will send. Set to 0 for infinite
let messageInterval = 1;  // Interval in milliseconds between messages
const max_time = 15000; // 15 s
//let usecase = "max_sent"
let usecase = "max_echo"
switch (usecase) {
    case "max_echo":
        // send as many messages as possible
        stopAfter = 15000
        messageInterval = 1
        break;
    case "max_single_chat":
        // send as many messages as possible
        stopAfter = 15000
        messageInterval = 1
        break;
    case "jrpg":
        // simulate a JRPG game
        stopAfter = 6000
        messageInterval = 1000 / 6
        break;
}
let clientCount = 0;
let clientErrors = 0;
let batchesReceived = 0;
let messagesSent = 0;
let messagesReceived = 0;

// CSV File path
const csvFilePath = path.join(__dirname, 'stress_test_results.csv');

// Write CSV header
fs.writeFileSync(csvFilePath, 'timestamp,client_count,client_errors,messages_sent,messages_received,batches_received\n');

// Function to start a WebSocket client
function startClient(clientId) {
    const ws = new WebSocket(serverUrl, ['blazium', 'echo']);
    ws.on('open', () => {
        clientCount++;
    });

    ws.on('message', (message) => {
        var message = JSON.parse(message.toString())
        if (Array.isArray(message)) {
            for (var i = 0; i < message.length; i++) {
                if (message[i]["command"] == "logical_error" || message[i]["command"] == "error") {
                    clientErrors++;
                    console.log(message[i])
                }
            }
            batchesReceived += message.length;
        } else {
            if (message["command"] == "logical_error" || message["command"] == "error") {
                clientErrors++;
                console.log(message)
            }
            batchesReceived++;
        }
        messagesReceived++;
        if (numClients == 1) {
            console.log(JSON.parse(message.toString()))
        }
    });

    ws.on('close', (code, reason) => {
        console.log(code, " ", reason)
        clientCount--;
    });

    ws.on('error', (error) => {
        console.log(JSON.parse(error.message))
        clientErrors++;
    });
    return ws
}

let websockets = []

for (let i = 0; i < numClients; i++) {
    websockets.push(startClient(i + 1));
}

let start = -1;
function startStresTest() {
    start = Date.now();
    console.log("Starting stress test...");
    for (let i = 0; i < numClients; i++) {
        let ws = websockets[i];
        messagesSent++;
        ws.send(JSON.stringify({
            "command": "create_lobby",
            "data": { "max_players": 2 }
        }));
        
        setTimeout(() => {
            let messageCount = 0;
            const messageIntervalId = setInterval(() => {
                // Send a message to the server
                switch (usecase) {
                    case "max_echo":
                        ws.send(JSON.stringify({
                            "command": "lobby_call",
                            "data": { "function": "echo", "inputs": ["abc"], "id": "123" }
                        }));
                    break;
                    case "max_single_chat":
                        ws.send(JSON.stringify({
                            "command": "chat_lobby",
                            "data": { "chat": "test", "id": "123" }
                        }));
                    break;
                }
        
                messagesSent++;
        
                messageCount++;
        
                let timestamp = Date.now() - start;
                // stop sending after specified time
                if (stopAfter < timestamp) {
                    clearInterval(messageIntervalId);
                    //ws.close();
                }
            }, messageInterval);
        }, messageInterval);
    }
}

// Interval to log stats and write to CSV
const logInterval = setInterval(() => {
    if (start == -1) {
        console.log("Waiting for all clients to connect..." + clientCount + " " + numClients);
        if (clientCount == numClients) {
            startStresTest();
        }
        return;
    }
    let timestamp = Date.now() - start;
    const line = `${timestamp},${clientCount},${clientErrors},${messagesSent},${messagesReceived},${batchesReceived}\n`;

    // Append line to CSV
    fs.appendFileSync(csvFilePath, line);

    // Log to console
    console.log(`Timestamp: ${timestamp} | Client Count: ${clientCount} | Client Errors: ${clientErrors} | Messages Sent: ${messagesSent} | Messages Received: ${messagesReceived} | Batches Received: ${batchesReceived}`);

    // Stop the test after max_time
    if (timestamp > max_time) {
        clearInterval(logInterval);
        console.log('Stress test completed. Results saved to stress_test_results.csv');
        process.exit();
    }
}, 1000);
