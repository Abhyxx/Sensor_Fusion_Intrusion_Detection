  #include <math.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

// WiFi Configuration
const char* ssid = "nnn";
const char* password = "987654312";

WebSocketsServer webSocket = WebSocketsServer(81);

// Pin Configuration
#define TRIG_PIN 5
#define ECHO_PIN 18

#define LED_PIN 2
#define BUZZER_PIN 4

#define RADAR_PIN 27

// Dataset Collection Settings
const int RADAR_SAMPLES = 10;

// Dataset phase should be sensitive.
// Use 1 if RCWL range still looks weak. Use 2 for balanced logging.
// For final demo you can increase it to 3, 5, or 7.
const int RADAR_THRESHOLD = 1;

// Distance threshold for alert or proximity zone.
// 150 cm means buzzer and LED will activate within 1.5 m.
const float ALERT_DISTANCE_CM = 150.0;

const float MIN_VALID_DISTANCE_CM = 20.0;
const float DISTANCE_CHANGE_THRESHOLD_CM = 10.0;
const float SPEED_THRESHOLD_CM_S = 20.0;

// If distance becomes invalid for a moment, alert remains ON briefly.
const unsigned long ALERT_HOLD_MS = 3000;

// Dataset sampling interval.
const int LOOP_DELAY_MS = 200;

// Tracking Variables
float previousDistance = 0.0;
unsigned long previousTime = 0;
unsigned long lastAlertTime = 0;

// Inference code meaning:
// 0 = clear / no useful detection
// 1 = radar motion only
// 2 = proximity only, object/person in ultrasonic range but no radar motion
// 3 = radar motion + distance seen, but outside alert zone
// 4 = breach verified / object within alert distance

// ==========================================
// NEW: LOG ANALYSIS SYSTEM VARIABLES
// ==========================================
unsigned long totalSamples = 0;
unsigned long breachCount = 0;
unsigned long radarOnlyCount = 0;
unsigned long staticCloseCount = 0;
float maxSpeedObserved = 0.0;
float cumulativeSpeed = 0.0;
unsigned long movingSamples = 0;
unsigned long lastLogAnalysisTime = 0;
const unsigned long LOG_ANALYSIS_INTERVAL_MS = 10000; // Output stats every 10 seconds

// Broadcast Telemetry
void broadcastTelemetry(
    String radarStr,
    String pirPlaceholder,
    float distVal,
    String infStr,
    float speedMps,
    unsigned long sampleTime,
    int radarBin,
    int ultrasonicActive,
    int inferenceCode,
    float distanceChangeCm,
    float speedCmS,
    int radarCount
) {
    String payload =
        radarStr + "," +
        pirPlaceholder + "," +
        String(distVal, 1) + "," +
        infStr + "," +
        String(speedMps, 3) + "," +
        String(sampleTime) + "," +
        String(radarBin) + "," +
        String(ultrasonicActive) + "," +
        String(inferenceCode) + "," +
        String(distanceChangeCm, 1) + "," +
        String(speedCmS, 2) + "," +
        String(radarCount);

    // Send to dashboard / Python WebSocket logger
    webSocket.broadcastTXT(payload);

    // Backup USB serial stream for CSV logging
    Serial.print("CSV,");
    Serial.println(payload);
}

// WebSocket Event Handler
// Change WSType_t to WStype_t (lowercase 't')
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    
    IPAddress ip = webSocket.remoteIP(num);
    String clientIP = ip.toString();

    switch(type) {
        // Change WSType_CONNECTED to WStype_CONNECTED
        case WStype_CONNECTED: {
            if (clientIP != "10.239.23.186") {
                String securityAlert = "SECURITY_ALERT," + clientIP;
                webSocket.broadcastTXT(securityAlert);
            }
            break;
        }
        
        // Change WSType_TEXT to WStype_TEXT
        case WStype_TEXT: {
            // Handle your standard incoming commands or data here
            break;
        }
        
        // Change WSType_DISCONNECTED to WStype_DISCONNECTED
        case WStype_DISCONNECTED:
            break;
    }
}

// HC-SR04 Distance Function
float getDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // 30000 us timeout gives around 5 m theoretical max range.
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);

    if(duration == 0) {
        return -1.0;
    }

    return duration * 0.0343 / 2.0;
}

// Average Ultrasonic Readings
float getAverageDistance() {
    float total = 0.0;
    int validReadings = 0;

    for(int i = 0; i < 5; i++) {
        float reading = getDistance();

        if(reading > 0 && reading < 500.0) {
            total += reading;
            validReadings++;
        }

        delay(10);
    }

    if(validReadings == 0) {
        return 999.0;
    }

    return total / (float)validReadings;
}

// RCWL Radar Read
int readRadarCount() {
    int radarCount = 0;

    for(int i = 0; i < RADAR_SAMPLES; i++) {
        radarCount += digitalRead(RADAR_PIN);
        delay(5);
    }

    return radarCount;
}

// Setup
void setup() {
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    pinMode(RADAR_PIN, INPUT);

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    Serial.println();
    Serial.println("IDS DATASET COLLECTION VERSION STARTED - PIR REMOVED");
    Serial.println("CSV_SCHEMA,radar_status,pir_placeholder,distance_cm,inference,speed_m_s,esp_millis,radar_bin,ultrasonic_active,inference_code,distance_change_cm,speed_cm_s,radar_count");

    WiFi.begin(ssid, password);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi Connected!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Explicit server mode initialization
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    delay(1000);
    Serial.println("System Monitoring Active.");
} // <--- FIXED: Added missing closing brace for setup() function

// ==========================================
// NEW: LOG ANALYSIS FUNCTION
// ==========================================
void performLogAnalysis(int infCode, float speedCmS, unsigned long currentMillis) {
    totalSamples++;
    
    // Track counts based on your active Inference Codes
    if (infCode == 4) breachCount++;
    else if (infCode == 1) radarOnlyCount++;
    else if (infCode == 2) staticCloseCount++;

    // Track kinematics analytics
    float absSpeed = fabs(speedCmS);
    if (absSpeed > 0.0) {
        cumulativeSpeed += absSpeed;
        movingSamples++;
        if (absSpeed > maxSpeedObserved) {
            maxSpeedObserved = absSpeed;
        }
    }

    // Periodically print the structured Log Analysis report
    if (currentMillis - lastLogAnalysisTime >= LOG_ANALYSIS_INTERVAL_MS) {
        float avgSpeed = (movingSamples > 0) ? (cumulativeSpeed / movingSamples) : 0.0;
        float breachRatio = (totalSamples > 0) ? ((float)breachCount / totalSamples) * 100.0 : 0.0;
        
        Serial.println("\n--- [LOG ANALYSIS REPORT] ---");
        Serial.printf("Uptime Checked      : %lu ms\n", currentMillis);
        Serial.printf("Total Data Samples  : %lu\n", totalSamples);
        Serial.printf("Verified Breaches   : %lu (%.1f%% of logs)\n", breachCount, breachRatio);
        Serial.printf("Radar-Only Anomalies: %lu\n", radarOnlyCount);
        Serial.printf("Static Near Objects : %lu\n", staticCloseCount);
        Serial.printf("Max Approaching Spd : %.2f cm/s\n", maxSpeedObserved);
        Serial.printf("Avg Motion Speed    : %.2f cm/s\n", avgSpeed);
        Serial.println("-----------------------------\n");
        
        lastLogAnalysisTime = currentMillis;
    }
}

// Main Loop
void loop() {
    webSocket.loop();

    unsigned long currentTime = millis();

    // Read both remaining sensors continuously.
    int radarCount = readRadarCount();
    int radarBin = (radarCount >= RADAR_THRESHOLD) ? 1 : 0;

    float distance = getAverageDistance();
    int ultrasonicActive = 1;

    // Distance change and speed
    float distanceChangeCm = 0.0;
    float speedCmS = 0.0;

    if(previousTime != 0 &&
       distance > 0 &&
       distance < 900.0 &&
       previousDistance > 0 &&
       previousDistance < 900.0) {

        distanceChangeCm = previousDistance - distance;
        float deltaTime = (currentTime - previousTime) / 1000.0;

        if(fabs(distanceChangeCm) < 2.0) {
            distanceChangeCm = 0.0;
        }

        if(deltaTime > 0) {
            speedCmS = distanceChangeCm / deltaTime;
        }

        if(fabs(speedCmS) < 5.0) {
            speedCmS = 0.0;
        }
    }

    float speedMps = speedCmS / 100.0;

    // Inference State Logic
    bool validDistance = (distance >= MIN_VALID_DISTANCE_CM && distance < 500.0);
    bool closeObject = (validDistance && distance <= ALERT_DISTANCE_CM);
    bool radarMotion = (radarBin == 1);
    bool distanceChanging = (fabs(distanceChangeCm) >= DISTANCE_CHANGE_THRESHOLD_CM || fabs(speedCmS) >= SPEED_THRESHOLD_CM_S);

    bool breachVerified = closeObject && (radarMotion || distanceChanging);
    bool staticCloseObject = closeObject && !radarMotion && !distanceChanging;

    String radarStatus = radarBin ? "MOTION_DETECTED" : "STILL";
    String pirPlaceholder = "PIR_REMOVED";
    String inference;
    int inferenceCode;

    if(breachVerified) {
        inference = "BREACH_VERIFIED";
        inferenceCode = 4;
    }
    else if(staticCloseObject) {
        inference = "STATIC_OBJECT_NEAR";
        inferenceCode = 2;
    }
    else if(radarMotion && validDistance) {
        inference = "RADAR_DISTANCE_TRACKING";
        inferenceCode = 3;
    }
    else if(radarMotion && !validDistance) {
        inference = "RADAR_ONLY";
        inferenceCode = 1;
    }
    else {
        inference = "SECURE";
        inferenceCode = 0;
    }

    // Send data to dashboard + CSV logger
    broadcastTelemetry(
        radarStatus,
        pirPlaceholder,
        distance,
        inference,
        speedMps,
        currentTime,
        radarBin,
        ultrasonicActive, // FIXED: Changed 'tick' back to your native tracking variable
        inferenceCode,
        distanceChangeCm,
        speedCmS,
        radarCount
    );

    // Debug line
    Serial.print("DEBUG,radarCount=");
    Serial.print(radarCount);
    Serial.print(",distance=");
    Serial.print(distance);
    Serial.print(",distanceChangeCm=");
    Serial.print(distanceChangeCm);
    Serial.print(",speedCmS=");
    Serial.println(speedCmS);

    // Alert Actuation
    if(breachVerified) {
        lastAlertTime = millis();
    }

    if(millis() - lastAlertTime <= ALERT_HOLD_MS) {
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
    } else {
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
    }

    previousDistance = distance;
    previousTime = currentTime;

    // ==========================================
    // NEW: INTEGRATION CALL FOR LOG ANALYSIS
    // ==========================================
    performLogAnalysis(inferenceCode, speedCmS, currentTime);

    delay(LOOP_DELAY_MS);
}
