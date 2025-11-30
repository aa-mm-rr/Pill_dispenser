#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"

// Pin definitions
#define MOTOR_PIN1 13
#define MOTOR_PIN2 6
#define MOTOR_PIN3 3
#define MOTOR_PIN4 2
#define SENSOR_PIN 28
#define PIEZO_PIN 27
#define BUTTON1 9
#define BUTTON2 8
#define LED_PIN 22

// Motor sequence
const int motorSteps[8][4] = {
    {1,0,0,0},
    {1,1,0,0},
    {0,1,0,0},
    {0,1,1,0},
    {0,0,1,0},
    {0,0,1,1},
    {0,0,0,1},
    {1,0,0,1}
};

int stepPosition = 0;
int motorStepsPerTurn = 4096;
bool isCalibrated = false;
bool isRunning = false;
int pillsCount = 0;
int totalPills = 7;

// Program states
typedef enum {
    WAITING_FOR_START,
    DOING_CALIBRATION,
    READY_TO_DISPENSE,
    DISPENSING_PILLS,
    CHECKING_PILL,
    PILL_ERROR
} programState;

programState currentState = WAITING_FOR_START;

// Motor functions
void setMotorPins(int step) {
    gpio_put(MOTOR_PIN1, motorSteps[step][0]);
    gpio_put(MOTOR_PIN2, motorSteps[step][1]);
    gpio_put(MOTOR_PIN3, motorSteps[step][2]);
    gpio_put(MOTOR_PIN4, motorSteps[step][3]);
}

void moveMotorForward() {
    stepPosition = (stepPosition + 1) % 8;
    setMotorPins(stepPosition);
    sleep_ms(3);
}

void turnMotor(int steps) {
    for (int i = 0; i < steps; i++) {
        moveMotorForward();
    }
}

// Calibration
void calibrateDispenser() {
    printf("Calibrating...\n");

    bool foundHole = false;
    int searchLimit = motorStepsPerTurn * 2;

    for (int i = 0; i < searchLimit; i++) {
        moveMotorForward();

        if (!gpio_get(SENSOR_PIN)) {
            foundHole = true;
            printf("Found calibration hole\n");
            break;
        }

        sleep_ms(1);
    }

    if (foundHole) {
        isCalibrated = true;
        printf("Calibration done\n");
    } else {
        printf("Calibration failed\n");
    }
}

// Pill detection
bool checkForPill() {
    sleep_ms(200);

    bool pillFound = false;
    uint32_t checkStart = to_ms_since_boot(get_absolute_time());

    while ((to_ms_since_boot(get_absolute_time()) - checkStart) < 500) {
        if (!gpio_get(PIEZO_PIN)) {
            pillFound = true;
            break;
        }
        sleep_ms(10);
    }

    return pillFound;
}

// LED functions
void turnLedOn() {
    gpio_put(LED_PIN, true);
}

void turnLedOff() {
    gpio_put(LED_PIN, false);
}

void blinkLed(int times, int speed) {
    for (int i = 0; i < times; i++) {
        turnLedOn();
        sleep_ms(speed);
        turnLedOff();
        sleep_ms(speed);
    }
}

// Button check
bool checkButton(int pin) {
    if (!gpio_get(pin)) {
        sleep_ms(50);
        if (!gpio_get(pin)) {
            while (!gpio_get(pin)) {
                sleep_ms(10);
            }
            return true;
        }
    }
    return false;
}

// Main program logic
void runProgram() {
    static absolute_time_t lastPillTime = {0};
    static int errorCount = 0;

    switch (currentState) {
        case WAITING_FOR_START:
            blinkLed(1, 500);

            if (checkButton(BUTTON1)) {
                currentState = DOING_CALIBRATION;
                printf("Starting calibration\n");
            }
            break;

        case DOING_CALIBRATION:
            calibrateDispenser();
            if (isCalibrated) {
                currentState = READY_TO_DISPENSE;
                turnLedOn();
            } else {
                currentState = WAITING_FOR_START;
            }
            break;

        case READY_TO_DISPENSE:
            if (checkButton(BUTTON2)) {
                currentState = DISPENSING_PILLS;
                pillsCount = 0;
                isRunning = true;
                lastPillTime = get_absolute_time();
                printf("Starting pill dispensing\n");
            }
            break;

        case DISPENSING_PILLS:
            if (absolute_time_diff_us(lastPillTime, get_absolute_time()) >= 30000000) {
                printf("Dispensing pill %d\n", pillsCount + 1);

                turnMotor(motorStepsPerTurn / 8);

                currentState = CHECKING_PILL;
                lastPillTime = get_absolute_time();
            }
            break;

        case CHECKING_PILL:
            if (checkForPill()) {
                pillsCount++;
                printf("Pill %d OK\n", pillsCount);

                if (pillsCount >= totalPills) {
                    printf("All pills done\n");
                    currentState = WAITING_FOR_START;
                    isRunning = false;
                    isCalibrated = false;
                } else {
                    currentState = DISPENSING_PILLS;
                }
            } else {
                printf("No pill detected\n");
                currentState = PILL_ERROR;
                errorCount = 0;
            }
            break;

        case PILL_ERROR:
            if (errorCount < 5) {
                blinkLed(1, 200);
                errorCount++;
                sleep_ms(300);
            } else {
                pillsCount++;
                if (pillsCount >= totalPills) {
                    printf("Finished with errors\n");
                    currentState = WAITING_FOR_START;
                    isRunning = false;
                    isCalibrated = false;
                } else {
                    currentState = DISPENSING_PILLS;
                }
            }
            break;
    }
}

int main(void) {
    stdio_init_all();
    printf("Pill dispenser starting\n");

    // Setup motor pins
    gpio_init(MOTOR_PIN1); gpio_set_dir(MOTOR_PIN1, GPIO_OUT);
    gpio_init(MOTOR_PIN2); gpio_set_dir(MOTOR_PIN2, GPIO_OUT);
    gpio_init(MOTOR_PIN3); gpio_set_dir(MOTOR_PIN3, GPIO_OUT);
    gpio_init(MOTOR_PIN4); gpio_set_dir(MOTOR_PIN4, GPIO_OUT);

    // Setup sensors
    gpio_init(SENSOR_PIN);
    gpio_set_dir(SENSOR_PIN, GPIO_IN);
    gpio_pull_up(SENSOR_PIN);

    gpio_init(PIEZO_PIN);
    gpio_set_dir(PIEZO_PIN, GPIO_IN);
    gpio_pull_up(PIEZO_PIN);

    // Setup buttons
    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);
    gpio_pull_up(BUTTON1);

    gpio_init(BUTTON2);
    gpio_set_dir(BUTTON2, GPIO_IN);
    gpio_pull_up(BUTTON2);

    // Setup LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    setMotorPins(stepPosition);

    printf("Ready - press button 1 to start\n");

    while (true) {
        runProgram();
        sleep_ms(100);
    }

    return 0;
}