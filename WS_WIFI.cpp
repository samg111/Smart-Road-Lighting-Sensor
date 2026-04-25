#include "WS_WIFI.h"

// The name and password of the WiFi access point
const char *ssid = APSSID;                
const char *password = APPSK;               
IPAddress apIP(10, 10, 10, 1);    // Set the IP address of the AP

char ipStr[16];
WebServer server(80);                               

uint8_t sliderValues[64] = {0};   
void handleRoot() {
  String myhtmlPage = "<html>"
    "<head>"
    "    <meta charset=\"utf-8\">"
    "    <title>ESP32-C6-DALI</title>"
    "    <style>"
    "        body {"
    "            font-family: Arial, sans-serif;"
    "            background-color: #f0f0f0;"
    "            margin: 0;"
    "            padding: 0;"
    "        }"
    "        .header {"
    "            text-align: center;"
    "            padding: 20px 0;"
    "            background-color: #333;"
    "            color: #fff;"
    "            margin-bottom: 20px;"
    "        }"
    "        .container {"
    "            max-width: 600px;"
    "            margin: 0 auto;"
    "            padding: 20px;"
    "            background-color: #fff;"
    "            border-radius: 5px;"
    "            box-shadow: 0 0 5px rgba(0, 0, 0, 0.3);"
    "        }"
    "        .slider-container {"
    "            margin-top: 20px;"
    "            text-align: center;"
    "        }"
    "        .slider-container label {"
    "            display: block;"
    "            margin-bottom: 10px;"
    "        }"
    "        .slider-container input[type=\"range\"] {"
    "            width: 80%;"
    "            height: 20px;"
    "            -webkit-appearance: none;"
    "            background: #ddd;"
    "            border-radius: 10px;"
    "            outline: none;"
    "        }"
    "        .slider-container input[type=\"range\"]::-webkit-slider-thumb {"
    "            -webkit-appearance: none;"
    "            width: 30px;"
    "            height: 30px;"
    "            background: #333;"
    "            border-radius: 50%;"
    "        }"
    "        .slider-container input[type=\"range\"]::-moz-range-thumb {"
    "            width: 30px;"
    "            height: 30px;"
    "            background: #333;"
    "            border-radius: 50%;"
    "        }"
    "        .button-container {"
    "            margin-top: 20px;"
    "            text-align: center;"
    "        }"
    "        .button-container button {"
    "            margin: 0 5px;"
    "            padding: 10px 15px;"
    "            background-color: #333;"
    "            color: #fff;"
    "            font-size: 14px;"
    "            font-weight: bold;"
    "            border: none;"
    "            border-radius: 3px;"
    "            text-transform: uppercase;"
    "            cursor: pointer;"
    "        }"
    "        .button-container button:hover {"
    "            background-color: #555;"
    "        }"
    "    </style>"
    "</head>"
    "<body>"
    "    <script defer>"
    "        function updateSlider(sliderId, value) {"
    "            var xhr = new XMLHttpRequest();"
    "            xhr.open('GET', '/SetSlider?sliderId=' + sliderId + '&value=' + value, true);"
    "            xhr.send();"
    "        }"
    "        function createSliders() {"
    "            let container = document.querySelector('.slider-container');"
    "            container.innerHTML = '';"
    "            for (let i = 1; i <= " + String(DALI_NUM) + "; i++) {"
    "                let label = document.createElement('label');"
    "                label.setAttribute('for', 'slider' + i);"
    "                label.textContent = 'Slider ' + i;"
    "                container.appendChild(label);"
    "                let slider = document.createElement('input');"
    "                slider.type = 'range';"
    "                slider.id = 'slider' + i;"
    "                slider.min = '0';"
    "                slider.max = '100';"
    "                slider.value = '0';"
    "                slider.addEventListener('input', function() {"
    "                    updateSlider(i, this.value);"
    "                });"
    "                container.appendChild(slider);"
    "            }"
    "        }"
    "        function ledSwitch(ledNumber) {"
    "            var xhttp = new XMLHttpRequest();"
    "            xhttp.onreadystatechange = function() {"
    "                if (this.readyState == 4 && this.status == 200) {"
    "                    console.log('LED ' + ledNumber + ' state changed');"
    "                }"
    "            };"
    "            if (ledNumber == 2) {"
    "                xhttp.open('GET', '/ALLOn', true);"
    "            } else if (ledNumber == 3) {"
    "                xhttp.open('GET', '/ALLOff', true);"
    "            } else if (ledNumber == 4) {"
    "                xhttp.open('GET', '/Loop', true);"
    "            }"
    "            xhttp.send();"
    "        }"
    "        document.addEventListener('DOMContentLoaded', function() {"
    "            createSliders();"
    "            document.getElementById('btn2').addEventListener('click', function() {"
    "                ledSwitch(2);"
    "            });"
    "            document.getElementById('btn3').addEventListener('click', function() {"
    "                ledSwitch(3);"
    "            });"
    "            document.getElementById('btn4').addEventListener('click', function() {"
    "                ledSwitch(4);"
    "            });"
    "        });"
    "    </script>"
    "    <div class=\"header\">"
    "        <h1>ESP32-C6-DALI</h1>"
    "    </div>"
    "    <div class=\"container\">"
    "        <div class=\"slider-container\">"
    "            <!-- Sliders will be dynamically inserted here -->"
    "        </div>"
    "        <div class=\"button-container\">"
    "            <button id=\"btn2\">ALL On</button>"
    "            <button id=\"btn3\">ALL Off</button>"
    "            <button id=\"btn4\">Loop</button>"
    "        </div>"
    "    </div>"
    "</body>"
    "</html>";

  server.send(200, "text/html", myhtmlPage); 
  printf("The user visited the home page\r\n");
}


void handleSetSlider() {
  if (server.hasArg("sliderId") && server.hasArg("value")) {
    int sliderId = server.arg("sliderId").toInt();
    int value = server.arg("value").toInt();
    
    // 确保 sliderId 在合法范围内
    if (sliderId >= 1 && sliderId <= DALI_NUM) {
      setBrightness(sliderId, value);
    }
  }
  server.send(200, "text/plain", "OK");
}

// 设置亮度的函数（需要实现具体逻辑）
void setBrightness(int sliderId, int value) {
    // 这里根据 sliderId 设置亮度
    Luminaire_Brightness(value, DALI_Addr[sliderId-1]);
    printf("Slider %d value: %d\n", sliderId, value);
}

void handleSwitch(uint8_t ledNumber) {
  switch(ledNumber){
    case 1:
      printf("RGB On.\r\n");
      Lighten_ALL();                                                       
      break;
    case 2:
      printf("RGB Off.\r\n");
      Extinguish_ALL();                                                    
      break;
  }
  server.send(200, "text/plain", "OK");
}
bool DALI_Loop = 1;
void handleALLOn()     { handleSwitch(1); }
void handleALLOff()    { handleSwitch(2); }
void handleLoop()      { DALI_Loop = ! DALI_Loop;}


void WIFI_Init()
{
  WiFi.mode(WIFI_AP); 
  while(!WiFi.softAP(ssid, password)) {
    printf("Soft AP creation failed.\r\n");
    printf("Try setting up the WIFI again.\r\n");
  } 
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Set the IP address and gateway of the AP
  
  IPAddress myIP = WiFi.softAPIP();
  uint32_t ipAddress = WiFi.softAPIP();
  printf("AP IP address: ");
  sprintf(ipStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  printf("%s\r\n", ipStr);

  printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA:%d\r\n", DALI_NUM);
  server.on("/", handleRoot);
  server.on("/SetSlider", handleSetSlider);
  server.on("/ALLOn"       , handleALLOn);
  server.on("/ALLOff"      , handleALLOff);
  server.on("/Loop"        , handleLoop);

  server.begin(); 
  printf("Web server started\r\n");
}

void WIFI_Loop()
{
  server.handleClient(); // Processing requests from clients
}
















