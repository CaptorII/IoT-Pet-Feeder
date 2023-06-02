#include <LiquidCrystal.h>
#include <stdio.h>
#include <string.h>
#include <Servo.h>

Servo servo;
LiquidCrystal lcd(6, 7, 2, 3, 4, 5);

int prevDisplayButtonState = 1;
int prevDispenseButtonState = 1;
int lowLevel = 20;
int emptyLevel = 1;
unsigned long feedScheduleTimer = 43200000UL; // 12 hours in milliseconds
unsigned long levelCheckTimer = 300000; // 5 minutes in milliseconds
unsigned long lastFeedTime = 0;
unsigned long lastCheckTime = 0;
unsigned long delayTimer = 0;
bool delayRunning = false;
bool levelLow = false;
bool feederEmpty = false;

void setup() {
    // initialise serial and LCD display
    Serial.begin(9600);
    lcd.begin(16, 2);
    // initialise digital pins
    pinMode(12, OUTPUT); // trigger pin on sensor
    pinMode(11, OUTPUT); // servo pin
    pinMode(10, INPUT); // echo pin on sensor
    pinMode(9, INPUT_PULLUP); // display button pin
    pinMode(8, INPUT_PULLUP); // dispense button pin
    servo.attach(11); // attach servo to pin 11
    servo.write(0); // set initial position of servo
}

void loop() {
    // start timer for checking level
    unsigned long timeSinceLevelCheck = millis() - lastCheckTime;
    if (timeSinceLevelCheck >= levelCheckTimer) {
        check_if_level_low();
    }
    // start timer for next auto-feed
    unsigned long timeSinceLastFeed = millis() - lastFeedTime;
    if (timeSinceLastFeed >= feedScheduleTimer) {
        if (check_level() > emptyLevel) {
            dispense_food();
        }
        else {
            feeder_empty();
            lastFeedTime = millis(); // reset last feed timer
            Serial.print("Feeder empty - unable to feed at scheduled time (ms): ");
            Serial.println(lastFeedTime); // print error
            // send error to dashboard feed !-- not implemented --!
        }
    }
    // check if display button is pressed
    int buttonState = digitalRead(9);
    if (buttonState != prevDisplayButtonState) {
        prevDisplayButtonState = buttonState;
        if (buttonState == 0) {            
            display_level(check_level());
            check_if_level_low();
        }
    }
    // check if dispense button pressed
    buttonState = digitalRead(8);
    if (buttonState != prevDispenseButtonState) {
        prevDispenseButtonState = buttonState;
        if(buttonState == 0) {
            if (check_level() > emptyLevel) {
                dispense_food();
            }
            else {
                feeder_empty();
                lastFeedTime = millis(); // reset last feed timer
                Serial.print("Feeder empty - unable to feed when requested at (ms): ");
                Serial.println(lastFeedTime); // print error
                // send error to dashboard feed !-- not implemented --!
            }
        }
    }    
    if (delayRunning && (millis() - delayTimer) >= 5000) {
        // clear top line after 5 seconds
        clear_line(0);
        delayRunning = false;
    }
    delay(100); // check for button presses every 0.1 seconds
}

void dispense_food() {
    clear_line(0);
    lcd.print("Dispensing...");
    servo.write(90); // open feeding gate
    delay(2000); // leave gate open for 2 seconds to dispense 
    servo.write(0); // close gate
    clear_line(0);
    lcd.print("Food dispensed!");
    lastFeedTime = millis(); // reset last feed timer
    Serial.print("Food dispensed at (ms): ");
    Serial.println(lastFeedTime);
    delayTimer = millis(); // start timer to clear screen
    delayRunning = true;
}

int check_level() {
    // send pulse
    digitalWrite(12, 0);
    delay(5);
    digitalWrite(12, 1);
    delay(10);
    digitalWrite(12, 0);    
    int duration = pulseIn(10, 1); // read pulse
    // calculate remaining food level from pulse time
    int foodLevel = 100 - (duration / 200);    
    char foodLvl[3]; // convert number to string
    sprintf(foodLvl, "%d", foodLevel);
    // set up string for output
    char foodLvlStr[16] = "Food level: ";
    strcat(foodLvlStr, foodLvl);
    strcat(foodLvlStr, "%");
    Serial.println(foodLvlStr); // print food level to serial
    lastCheckTime = millis(); // reset last check timer
    return foodLevel;
}

void display_level(int foodLevel) {
    char foodLvl[3]; // convert number to string
    sprintf(foodLvl, "%d", foodLevel);
    // set up string for output
    char foodLvlStr[16] = "Food level: ";
    strcat(foodLvlStr, foodLvl);
    strcat(foodLvlStr, "%");
    clear_line(0);
    lcd.print(foodLvlStr); // print food level to LCD    
    delayTimer = millis(); // start timer to clear screen
    delayRunning = true;
}

void clear_line(int line) {
    lcd.setCursor(0, line);
    lcd.print("                ");  // 16 spaces to clear the line
    lcd.setCursor(0, line);    
}

void feeder_empty() {
    clear_line(1);
    lcd.write("Feeder empty!");
    feederEmpty = true;
}

void check_if_level_low() {
    int foodLevel = check_level();
    // send food level to ada dashboard via wifi !-- not implemented --!
    if (foodLevel <= lowLevel && !feederEmpty) {
        if (!levelLow) { // on first detecting low food level
            // set bottom line to display warning
            clear_line(1);
            lcd.write("Food level low!");
            // send alert to refill feeder !-- not implemented --!
        }
        levelLow = true;
    }
    else if (foodLevel <= emptyLevel) {
        feeder_empty();
    }
    else { // when feeder is refilled
        if (levelLow) {
            clear_line(1);
            levelLow = false;
        }
    }
}