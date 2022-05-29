#include <wifi.h> //Connect to WiFi Network
    #include<string.h> // Used for some string handling and processing
    #include <tft_espi.h> // Graphics and font library for ST7735 driver chip
    #include <spi.h> // Used in support of TFT Display
    #include <math.h> // Used for mathematical functions
    
    TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
    
    //Some constants and some resources:
    const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
    const int GETTING_PERIOD = 2000; //periodicity of getting a number fact
    const int BUTTON_TIMEOUT = 1000; //button timeout in milliseconds
    const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
    const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
    char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
    char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
    
    char network[] = "MIT";
    char password[] = "";
    
    uint8_t channel = 1; //network channel on 2.4 GHz
    byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; //6 byte MAC address of AP you're targeting.
    
    const int BUTTON = 5; //connect to pin 5
    unsigned long timer; // used for starting millis() readings
    uint8_t draw_state;  //used for remembering state
    uint8_t previous_value;  //used for remembering previous button
    
    // Store timestamp information for math operations
    int hour;
    int minute;
    int second;
    
    // Angles each hand is from the top of the clock
    double theta_hr;
    double theta_min;
    double theta_sec;
    
    // Store timestamp information to be printed on screen
    char* hour_tok;
    char* minute_tok;
    
    int DB_COUNT_THRESHOLD = 300;
    int db_state = 1;
    int db_count = 0;
    
    void setup() {
      tft.init(); //initialize the screen
      tft.setRotation(2); //set rotation for our layout
      tft.setTextSize(1); //default font size
      tft.fillScreen(TFT_BLACK); //fill background
      tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
      Serial.begin(115200); //begin serial comms
    
      timer = millis();
    
      int n = WiFi.scanNetworks();
      Serial.println("scan done");
      if (n == 0) {
        Serial.println("no networks found");
      } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
          Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
          uint8_t* cc = WiFi.BSSID(i);
          for (int k = 0; k < 6; k++) {
            Serial.print(*cc, HEX);
            if (k != 5) Serial.print(":");
            cc++;
          }
          Serial.println("");
        }
      }
      delay(100); //wait a bit (100 ms)
    
      //if using regular connection use line below:
      WiFi.begin(network, password);
      //if using channel/mac specification for crowded bands use the following:
      //WiFi.begin(network, password, channel, bssid);
      uint8_t count = 0; //count used for Wifi check times
      Serial.print("Attempting to connect to ");
      Serial.println(network);
      while (WiFi.status() != WL_CONNECTED && count < 6) {
        delay(500);
        Serial.print(".");
        count++;
      }
      delay(2000);
      if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
        Serial.println("CONNECTED!");
        Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                      WiFi.localIP()[1], WiFi.localIP()[0],
                      WiFi.macAddress().c_str() , WiFi.SSID().c_str());
        delay(500);
      } else { //if we failed to connect just Try again.
        Serial.println("Failed to Connect :/  Going to restart");
        Serial.println(WiFi.status());
        ESP.restart(); // restart the ESP (proper way)
      }
    
      pinMode(BUTTON, INPUT_PULLUP); //sets IO pin 5 as an input which defaults to a 3.3V signal when unconnected and 0V when the switch is pushed
      draw_state = 0; //initialize to 0
      previous_value = 1; //initialize to 1
    }
    
    // Debouncing function to update state after button bouncing has steadied
    uint8_t db_button(uint8_t input) {
      if (input != db_state) {
        db_count++;
        if (db_count > DB_COUNT_THRESHOLD) {
          db_state = !db_state;
        }
      } else {
        db_count = 0;
      }
      return db_state;
    }
    
    void loop() {
    
      // Make GET request to http://iesc-s3.mit.edu/esp32test/currenttime every minute
      if (millis() - timer == 60000) {
        sprintf(request_buffer, "GET http://iesc-s3.mit.edu/esp32test/currenttime\r\n");
        strcat(request_buffer, "Host: iesc-s3.mit.edu\r\n"); //add more to the end
        strcat(request_buffer, "\r\n"); //add blank line!
        //submit to function that performs GET.  It will return output using response_buffer char array
        do_http_GET("iesc-s3.mit.edu", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
        Serial.println(response_buffer); //print to serial monitor
        timer = millis();
      }
    
      // Record seconds passed according to internal timer
      second = (millis() - timer) / 1000;
    
      // Parse response buffer using delimiters and store hours, minutes in variables
      char* token = strtok(response_buffer, " :");
      int count = 1;
      while (token != NULL) {
        count++;
        if (count == 3) {
          hour_tok = token;
          hour = atoi(token);
        }
        else if (count == 4) {
          minute_tok = token;
          minute = atoi(token);
        }
        token = strtok(NULL, " :");
      }
    
      uint8_t value = db_button(digitalRead(BUTTON)); //get reading
      if (value == 0 and previous_value == 1) { // check if button if pressed
        // If state = 0, show digital reading of time on screen
        if (draw_state == 0) {
          draw_state = 1;
          tft.fillScreen(TFT_BLACK);
          tft.setCursor(0, 0, 1);
          tft.print(hour_tok);
          tft.print(":");
          tft.print(minute_tok);
        }
        // If state = 1, show analog reading of time on screen
        else if (draw_state == 1) {
          draw_state = 0;
          tft.fillScreen(TFT_BLACK);
          tft.drawCircle(64, 64, 50, TFT_YELLOW);
    
          // Draw hour hand
          if (hour > 12) {  // Account for pm times
            hour -= 12;
          }
          theta_hr = ((hour + (minute / 60.0))/ 12.0) * (2 * M_PI);  // Calculate angle between hour hand and top of the clock
          tft.drawLine(64, 64, 25 * sin(theta_hr) + 64, 64 - 25 * cos(theta_hr), TFT_BLUE);
    
          // Draw minute hand
          theta_min = (minute / 60.0) * (2 * M_PI); // Calculate angle between minute hand and top of the clock
          tft.drawLine(64, 64, 40 * sin(theta_min) + 64, 64 - 40 * cos(theta_min), TFT_GREEN);
    
          // Draw second hand
          theta_sec = (second / 60.0) * (2 * M_PI); // Calculate angle between second hand and top of the clock
          tft.drawLine(64, 64, 50 * sin(theta_sec) + 64, 64 - 50 * cos(theta_sec), TFT_RED);
        }
    
      }
      previous_value = value; // Set old button value
    }
    
    /*----------------------------------
       char_append Function:
       Arguments:
          char* buff: pointer to character array which we will append a
          char c:
          uint16_t buff_size: size of buffer buff
    
       Return value:
          boolean: True if character appended, False if not appended (indicating buffer full)
    */
    uint8_t char_append(char* buff, char c, uint16_t buff_size) {
      int len = strlen(buff);
      if (len > buff_size) return false;
      buff[len] = c;
      buff[len + 1] = '\0';
      return true;
    }
    
    /*----------------------------------
       do_http_GET Function:
       Arguments:
          char* host: null-terminated char-array containing host to connect to
          char* request: null-terminated char-arry containing properly formatted HTTP GET request
          char* response: char-array used as output for function to contain response
          uint16_t response_size: size of response buffer (in bytes)
          uint16_t response_timeout: duration we'll wait (in ms) for a response from server
          uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
       Return value:
          void (none)
    */
    void do_http_GET(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
      WiFiClient client; //instantiate a client object
      if (client.connect(host, 80)) { //try to connect to host on port 80
        if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
        client.print(request);
        memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
        uint32_t count = millis();
        while (client.connected()) { //while we remain connected read out data coming back
          client.readBytesUntil('\n', response, response_size);
          if (serial) Serial.println(response);
          if (millis() - count > response_timeout) break;
        }
        count = millis();
        if (serial) Serial.println(response);
        client.stop();
        if (serial) Serial.println("-----------");
      } else {
        if (serial) Serial.println("connection failed :/");
        if (serial) Serial.println("wait 0.5 sec...");
        client.stop();
      }
    }
