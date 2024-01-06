#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

const char* ssid = "101";
const char* password = "YourPassword";

SoftwareSerial mySerial(D4, D3); // RX, TX
String receivedData;
bool newDataAvailable = false;

ESP8266WebServer server(80);

// Store sent and received messages in arrays
const int maxMessages = 5; // Adjust this as needed
String sentMessages[maxMessages];
String receivedMessages[maxMessages];
int numSentMessages = 0;
int numReceivedMessages = 0;

const char* chatPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>101 Chat</title>
    <style>
        .chat-container {
            display: flex;
            flex-direction: column;
            align-items: flex-start;
            padding: 10px;
        }

        .chat-bubble {
            background-color: #f2f2f2;
            border-radius: 5px;
            padding: 10px;
            margin: 5px;
            max-width: 70%;
        }

        .user-bubble {
            background-color: #007bff;
            border-radius: 5px;
            padding: 10px;
            margin: 5px;
            max-width: 70%;
            color: #fff;
            align-self: flex-end;
        }
    </style>
    <script>
        function sendMessage() {
            var messageInput = document.getElementById("message");
            var message = messageInput.value;
            messageInput.value = "";

            var xhr = new XMLHttpRequest();
            xhr.open("POST", "/sendMessage", true);
            xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
            xhr.send("message=" + encodeURIComponent(message));
        }

        function getMessages() {
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function () {
                if (xhr.readyState === 4 && xhr.status === 200) {
                    var messageContainer = document.getElementById("messageContainer");
                    messageContainer.innerHTML = xhr.responseText;
                }
            };
            xhr.open("GET", "/getMessages", true);
            xhr.send();
        }

        setInterval(getMessages, 1000); // Poll for new messages every second
    </script>
</head>
<body>
<h1>101 Chat</h1>
<div class="chat-container" id="messageContainer"></div>
<input type="text" id="message" placeholder="Enter your message">
<button onclick="sendMessage()">Send</button>
</body>
</html>
)rawliteral";

void setup() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    Serial.begin(115200);
    delay(100);
    mySerial.begin(115200);

    setupRYLR896();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/sendMessage", HTTP_POST, handleSendMessage);
    server.on("/getMessages", HTTP_GET, handleGetMessages);

    server.begin();
}

void loop() {
    server.handleClient();
    handleSerialData();
}

void setupRYLR896() {
    // Configure the RYLR896 module here
    // Set network ID (must match on all devices in the same network)
    delay(500);
    clearSerialBuffer();
    delay(1000);
    mySerial.println("AT+NETWORKID=15\r\n");
    delay(1000);
    clearSerialBuffer();
    // Set the module's address (unique for each device)
    mySerial.println("AT+ADDRESS=101\r\n");
    delay(1000);
    clearSerialBuffer();
    // Set the frequency band (e.g., 915MHz)
    mySerial.println("AT+BAND=915000000\r\n");
    delay(1000);
    clearSerialBuffer();
    // Set the RF power (e.g., 15dBm)
    mySerial.println("AT+CRFOP=15\r\n");
    delay(1000);
    clearSerialBuffer();
    delay(500);
    // Set the parameters for long-range mode
    mySerial.println("AT+PARAMETER=12,7,1,4\r\n");

    delay(1000);
    clearSerialBuffer();
    mySerial.println("AT+NETWORKID=15\r\n");

    delay(1000);
    clearSerialBuffer();
}

void handleSerialData() {
    while (mySerial.available()) {
        char c = mySerial.read();
        if (c == '\n') {
            newDataAvailable = true;
            processReceivedData(receivedData);
            receivedData = "";
        } else {
            receivedData += c;
        }
    }
}

void handleRoot() {
    server.send(200, "text/html", chatPage);
}

void handleSendMessage() {
    String message = server.arg("message");

    // Attempt to send the message via RYLR896 module
    if (sendMessage(message)) {
        // Store sent message in the array
        if (numSentMessages < maxMessages) {
            sentMessages[numSentMessages++] = message;
        }
        server.send(200, "text/plain", "Message sent");
    } else {
        server.send(500, "text/plain", "Error sending message");
    }
}

bool sendMessage(const String& message) {
    // Send the message using RYLR896 module
    clearSerialBuffer();
    String sendData = "AT+SEND=102," + String(message.length()) + "," + message + "\r\n";
    mySerial.println(sendData);

    // Wait for a response
    delay(1000);
    while (mySerial.available()) {
        String response = mySerial.readStringUntil('\n');
        if (response.indexOf("OK") != -1) {
            return true; // Message sent successfully
        } else if (response.indexOf("ERROR") != -1) {
            return false; // Error sending message
        }
    }
    return false; // No response received
}

// ... (Previous code)

void processReceivedData(const String& data) {
    if (data.startsWith("+RCV=")) {
        // Split the data in the +RCV message by ","
        //+RCV=50,5,HELLO,-99,40
        String parts[5];
        int numParts = 0;
        int startPos = 5; // Skip "+RCV="

        for (int i = 0; i < 5; i++) {
            int commaPos = data.indexOf(',', startPos);
            if (commaPos != -1) {
                parts[i] = data.substring(startPos, commaPos);
                startPos = commaPos + 1;
                numParts++;
            }
        }

        // Display the arbitrary parts you want (e.g., the first and third parts)
        if (numParts >= 3) {
            String message = "From: " + parts[0] + ", Msg: " + parts[2]+ ", rssi: " + parts[3];
            receivedMessages[numReceivedMessages++] = message;
        }
    }
}

// ... (Rest of the code remains unchanged)


void handleGetMessages() {
    String responseMessage = "";

    // Combine sent and received messages into one array
    String allMessages[maxMessages * 2];
    int numAllMessages = 0;
    for (int i = 0; i < numSentMessages; i++) {
        allMessages[numAllMessages++] = "You: " + sentMessages[i];
    }
    for (int i = 0; i < numReceivedMessages; i++) {
        allMessages[numAllMessages++] = "Received: " + receivedMessages[i];
    }

    // Display the last N messages (adjust as needed)
    int numToDisplay = min(numAllMessages, 5); // Display the last 10 messages
    for (int i = numAllMessages - numToDisplay; i < numAllMessages; i++) {
        responseMessage += "<div class='chat-bubble'>" + allMessages[i] + "</div>";
    }

    server.send(200, "text/html", responseMessage);
}

void clearSerialBuffer() {
    while (mySerial.available()) {
        mySerial.read(); // Read and discard one character from the buffer
    }
}
