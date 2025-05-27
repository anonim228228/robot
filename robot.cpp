#include <wiringPi.h>
#include <mosquitto.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

std::string last_automatic_command;
std::string last_manual_command;
std::chrono::steady_clock::time_point last_manual_time;
std::chrono::steady_clock::time_point last_command_time;

void on_message(struct mosquitto *client, void *userdata, const struct mosquitto_message *message) {
    std::string topic(message->topic);
    std::string payload(static_cast<char*>(message->payload), message->payloadlen);
    std::cout << "Received command on topic '" << topic << "': " << payload << std::endl;
    if (topic == "robot/automatic_command") {
        last_automatic_command = payload;
        last_command_time = std::chrono::steady_clock::now();
    } else if (topic == "robot/manual_command") {
        last_manual_command = payload;
        last_manual_time = std::chrono::steady_clock::now();
        last_command_time = std::chrono::steady_clock::now();
    }
}

int main() {
    // Initialize Mosquitto
    mosquitto_lib_init();
    struct mosquitto *mqtt_client = mosquitto_new("robot_client", true, nullptr);
    if (!mqtt_client) {
        std::cerr << "Failed to create MQTT client" << std::endl;
        return 1;
    }
    if (mosquitto_connect(mqtt_client, "192.168.1.100", 1883, 60) != MOSQ_ERR_SUCCESS) { // Replace with your internal IP
        std::cerr << "Failed to connect to MQTT broker" << std::endl;
        mosquitto_destroy(mqtt_client);
        return 1;
    }
    mosquitto_subscribe(mqtt_client, nullptr, "robot/automatic_command", 0);
    mosquitto_subscribe(mqtt_client, nullptr, "robot/manual_command", 0);
    mosquitto_message_callback_set(mqtt_client, on_message);
    mosquitto_loop_start(mqtt_client);

    // Initialize GPIO for motors (commented for PC testing)
    /*
    wiringPiSetup();
    pinMode(0, OUTPUT);  // Left motor forward
    pinMode(1, OUTPUT);  // Left motor backward
    pinMode(2, OUTPUT);  // Right motor forward
    pinMode(3, OUTPUT);  // Right motor backward
    */

    while (true) {
        auto now = std::chrono::steady_clock::now();
        std::string current_command;

        // Check if manual command is active (within 1 second)
        if (!last_manual_command.empty() && now - last_manual_time < std::chrono::seconds(1)) {
            current_command = last_manual_command;
        } else if (!last_automatic_command.empty()) {
            current_command = last_automatic_command;
        } else {
            current_command = "stop";
        }

        // Stop if no new command received for 1 second
        if (now - last_command_time > std::chrono::seconds(1)) {
            current_command = "stop";
        }

        // Simulate motor control (uncomment for Raspberry Pi)
        /*
        if (current_command == "move_forward" || current_command == "forward") {
            digitalWrite(0, HIGH); digitalWrite(1, LOW);
            digitalWrite(2, HIGH); digitalWrite(3, LOW);
        } else if (current_command == "stop") {
            digitalWrite(0, LOW); digitalWrite(1, LOW);
            digitalWrite(2, LOW); digitalWrite(3, LOW);
        } else if (current_command == "left") {
            digitalWrite(0, LOW); digitalWrite(1, HIGH);
            digitalWrite(2, HIGH); digitalWrite(3, LOW);
        } else if (current_command == "right") {
            digitalWrite(0, HIGH); digitalWrite(1, LOW);
            digitalWrite(2, LOW); digitalWrite(3, HIGH);
        } else if (current_command == "backward") {
            digitalWrite(0, LOW); digitalWrite(1, HIGH);
            digitalWrite(2, LOW); digitalWrite(3, HIGH);
        }
        */

        // Clear commands after processing
        if (!last_manual_command.empty() && now - last_manual_time < std::chrono::seconds(1)) {
            last_manual_command.clear();
        } else if (!last_automatic_command.empty()) {
            last_automatic_command.clear();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    mosquitto_loop_stop(mqtt_client, false);
    mosquitto_destroy(mqtt_client);
    mosquitto_lib_cleanup();
    return 0;
}