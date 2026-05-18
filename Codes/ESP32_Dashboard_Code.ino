#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "xyz";
const char* password = "12345678";

WebServer server(80);

// UART2
HardwareSerial TeensySerial(2);

// ================= VARIABLES =================

String stage = "IDLE";
String weight = "0";

String sand = "OFF";
String cement = "OFF";
String water = "OFF";

String hopper = "OFF";

String emergency = "SAFE";

String logs = "";

// ======================================================
// HTML PAGE
// ======================================================

String webpage = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>Industrial Batching Dashboard</title>

<style>

body{
margin:0;
padding:0;
background:#0f172a;
font-family:Arial;
color:white;
}

.header{
background:#111827;
padding:20px;
text-align:center;
font-size:38px;
font-weight:bold;
box-shadow:0px 3px 10px rgba(0,0,0,0.5);
}

.container{
padding:20px;
}

.grid{
display:grid;
grid-template-columns:repeat(auto-fit,minmax(250px,1fr));
gap:20px;
}

.card{
background:#1e293b;
padding:25px;
border-radius:18px;
box-shadow:0px 5px 15px rgba(0,0,0,0.4);
transition:0.3s;
}

.card:hover{
transform:translateY(-5px);
}

.title{
font-size:18px;
color:#94a3b8;
margin-bottom:10px;
}

.value{
font-size:32px;
font-weight:bold;
}

.buttons{
margin-top:30px;
display:flex;
gap:20px;
flex-wrap:wrap;
}

button{
padding:18px 35px;
border:none;
border-radius:12px;
font-size:20px;
font-weight:bold;
cursor:pointer;
transition:0.3s;
}

.start{
background:#16a34a;
color:white;
}

.stop{
background:#dc2626;
color:white;
}

button:hover{
opacity:0.85;
transform:scale(1.03);
}

.logbox{
margin-top:30px;
background:#111827;
padding:20px;
border-radius:18px;
height:250px;
overflow-y:scroll;
font-size:15px;
}

</style>

<script>

function sendCommand(cmd){

fetch('/'+cmd);

}

setInterval(function(){

location.reload();

},2000);

</script>

</head>

<body>

<div class="header">

Automatic Batching System Dashboard

</div>

<div class="container">

<div class="grid">

<div class="card">
<div class="title">Current Stage</div>
<div class="value">%STAGE%</div>
</div>

<div class="card">
<div class="title">Weight</div>
<div class="value">%WEIGHT% Kg</div>
</div>

<div class="card">
<div class="title">Sand Relay</div>
<div class="value">%SAND%</div>
</div>

<div class="card">
<div class="title">Cement Relay</div>
<div class="value">%CEMENT%</div>
</div>

<div class="card">
<div class="title">Water Pump</div>
<div class="value">%WATER%</div>
</div>

<div class="card">
<div class="title">Hopper</div>
<div class="value">%HOPPER%</div>
</div>

<div class="card">
<div class="title">Emergency</div>
<div class="value">%EMERGENCY%</div>
</div>

</div>

<div class="buttons">

<button class="start" onclick="sendCommand('start')">
START SYSTEM
</button>

<button class="stop" onclick="sendCommand('stop')">
EMERGENCY STOP
</button>

</div>

<div class="logbox">

%LOGS%

</div>

</div>

</body>

</html>

)rawliteral";

// ======================================================
// ADD LOG
// ======================================================

void addLog(String msg){

logs += msg + "<br>";

if(logs.length() > 6000){

logs = logs.substring(logs.length() - 6000);

}

}

// ======================================================
// HANDLE ROOT
// ======================================================

void handleRoot(){

String page = webpage;

page.replace("%STAGE%", stage);

page.replace("%WEIGHT%", weight);

page.replace("%SAND%", sand);

page.replace("%CEMENT%", cement);

page.replace("%WATER%", water);

page.replace("%HOPPER%", hopper);

page.replace("%EMERGENCY%", emergency);

page.replace("%LOGS%", logs);

server.send(200, "text/html", page);

}

// ======================================================
// START COMMAND
// ======================================================

void handleStart(){

TeensySerial.println("START");

Serial.println("START SENT");

addLog("START command sent");

server.send(200, "text/plain", "START SENT");

}

// ======================================================
// STOP COMMAND
// ======================================================

void handleStop(){

TeensySerial.println("STOP");

Serial.println("STOP SENT");

addLog("STOP command sent");

server.send(200, "text/plain", "STOP SENT");

}

// ======================================================
// SETUP
// ======================================================

void setup(){

Serial.begin(115200);

// RX=16 TX=17
TeensySerial.begin(115200, SERIAL_8N1, 16, 17);

WiFi.begin(ssid, password);

while(WiFi.status() != WL_CONNECTED){

delay(500);

Serial.print(".");

}

Serial.println("");

Serial.println("WiFi Connected");

Serial.print("IP Address: ");

Serial.println(WiFi.localIP());

server.on("/", handleRoot);

server.on("/start", handleStart);

server.on("/stop", handleStop);

server.begin();

addLog("Dashboard Started");

}

// ======================================================
// LOOP
// ======================================================

void loop(){

server.handleClient();

if(TeensySerial.available()){

String data = TeensySerial.readStringUntil('\n');

data.trim();

Serial.println(data);

addLog(data);

int index;

// ================= STAGE =================

index = data.indexOf("STAGE=");

if(index >= 0){

stage = data.substring(index + 6,
data.indexOf(",", index));

}

// ================= WEIGHT =================

index = data.indexOf("WEIGHT=");

if(index >= 0){

weight = data.substring(index + 7,
data.indexOf(",", index));

}

// ================= SAND =================

index = data.indexOf("SAND=");

if(index >= 0){

String val = data.substring(index + 5,
data.indexOf(",", index));

sand = (val == "1") ? "ON" : "OFF";

}

// ================= CEMENT =================

index = data.indexOf("CEMENT=");

if(index >= 0){

String val = data.substring(index + 7,
data.indexOf(",", index));

cement = (val == "1") ? "ON" : "OFF";

}

// ================= WATER =================

index = data.indexOf("WATER=");

if(index >= 0){

String val = data.substring(index + 6,
data.indexOf(",", index));

water = (val == "1") ? "ON" : "OFF";

}

// ================= HOPPER =================

index = data.indexOf("HOPPER=");

if(index >= 0){

String val = data.substring(index + 7,
data.indexOf(",", index));

hopper = (val == "1") ? "ON" : "OFF";

}

// ================= EMERGENCY =================

index = data.indexOf("EMERGENCY=");

if(index >= 0){

String val = data.substring(index + 10);

emergency = (val == "1") ? "ACTIVE" : "SAFE";

}

}

}