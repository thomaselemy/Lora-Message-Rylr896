#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

const char* index_html = R"rawliteral(
  <!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP8266 Chat</title>
    <style>
      #chat-container {
        display: flex;
        flex-direction: column;
      }
      .message-bubble {
        border-radius: 20px;
        padding: 10px;
        margin: 5px;
        font-size: 18px;
        width: 70%;
        max-width: 500px;
        word-wrap: break-word;
      }
      .message-bubble.right {
        background-color: #007bff;
        color: white;
        align-self: flex-end;
      }
      .message-bubble.left1 {
        background-color: #f1f0f0;
        color: black;
        align-self: flex-start;
      }
      .message-bubble-left {
      max-width: 80%;
      margin: 10px;
      padding: 10px;
      border-radius: 15px;
      background-color: #c3e88d;
      color: #222;
      font-size: 18px;
      text-align: left;
      align-self: flex-start;
      }
    </style>
  </head>
  <body>
    <h1>ESP8266 Chat</h1>
    <div id="chat-container"></div>
    <form>
      <input type="text" id="message-input" placeholder="Enter message...">
      <button type="button" onclick="sendMessage()">Send</button>
    </form>
    <script>
      function updateChat(message, align) {
        var chatContainer = document.getElementById("chat-container");
        var messageElement = document.createElement("div");
        messageElement.textContent = message;
        messageElement.classList.add("message-bubble", align);
        chatContainer.appendChild(messageElement);
      }
      function sendMessage() {
        var messageInput = document.getElementById("message-input");
        var message = messageInput.value;
        messageInput.value = "";
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
          if (xhr.readyState === 4 && xhr.status === 200) {
            updateChat(message, "right");
          }
        };
        xhr.open("POST", "/message", true);
        xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
        xhr.send("message=" + encodeURIComponent(message));
      }
      setInterval(function() {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
          if (xhr.readyState === 4 && xhr.status === 200) {
            var message = xhr.responseText;
            if (message != "") {
              updateChat(message, "left");
            }
          }
        };
        xhr.open("GET", "/request", true);
        xhr.send();
      }, 1000);
    </script>
  </body>
</html>
)rawliteral";

String message = "";

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleMessage() {
  message = server.arg("message");
  server.send(200, "text/plain", "Message received");
}

void handleRequest() {
  server.send(200, "text/plain", message);
}

void setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP8266AP", "password");

  Serial.begin(115200);
  delay(100);

  server.on("/", handleRoot);
  server.on("/message", HTTP_POST, handleMessage);
  server.on("/request", handleRequest);

  server.begin();
}
void leftMessage() {
  static String previousMessage = "";
  String currentMessage = Serial.readStringUntil('\n');
  if (currentMessage != "" && currentMessage != previousMessage) {
    previousMessage = currentMessage;
    String messageHtml = "<div class='message-bubble-left'>" + currentMessage + "</div>";
    String javascriptCode = "var chatContainer = document.getElementById('chat-container'); chatContainer.innerHTML += '" + messageHtml + "';";
    server.sendContent(javascriptCode);
  }
}

void loop() {
  server.handleClient();
  leftMessage();
  if (message != "") {
    Serial.println(message);
    message = "";
  }
}
