import cv2
import numpy as np
import paho.mqtt.client as mqtt
import sys
import time

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Connected to MQTT broker")
    else:
        print(f"Failed to connect, return code: {rc}")

def detect_color_object(frame):
    # Convert to HSV color space
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    
    # Define color ranges
    colors = {
        'green': ([40, 40, 40], [70, 255, 255]),
        'red': ([0, 100, 100], [10, 255, 255]),
        'blue': ([100, 100, 100], [130, 255, 255])
    }
    
    largest_area = 0
    largest_contour = None
    detected_color = None
    
    # Check each color
    for color_name, (lower, upper) in colors.items():
        lower = np.array(lower, dtype=np.uint8)
        upper = np.array(upper, dtype=np.uint8)
        
        # Create mask for the color
        mask = cv2.inRange(hsv, lower, upper)
        
        # Find contours
        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        # Find the largest contour
        for contour in contours:
            area = cv2.contourArea(contour)
            if area > largest_area and area > 500:  # Minimum area threshold
                largest_area = area
                largest_contour = contour
                detected_color = color_name
    
    return largest_contour, detected_color

def main():
    # Initialize MQTT client
    client = mqtt.Client(client_id="server_client", callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    try:
        client.connect("localhost", 1883, 60)
    except Exception as e:
        print(f"Error connecting to MQTT broker: {e}")
        return
    
    client.loop_start()

    # Initialize camera
    cap = cv2.VideoCapture(2)  # USB camera
    if not cap.isOpened():
        print("Error: Could not open camera")
        client.loop_stop()
        client.disconnect()
        return

    print("Press 'w' (forward), 'a' (left), 'd' (right), 's' (backward), 'space' (stop), 'q' (quit)")

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error: Could not read frame")
            break

        # Detect colored object
        contour, color = detect_color_object(frame)
        command = "stop"  # Default command

        if contour is not None:
            # Draw bounding box around the object
            x, y, w, h = cv2.boundingRect(contour)
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
            cv2.putText(frame, color, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 0), 2)
            
            # Automatic command: move toward object
            command = "move_forward"
            client.publish("robot/automatic_command", command)
            print(f"Sent automatic command: {command} (detected {color} object)")
        else:
            client.publish("robot/automatic_command", "stop")
            print("Sent automatic command: stop (no object detected)")

        # Display frame
        cv2.imshow("Camera", frame)

        # Handle manual control
        key = cv2.waitKey(10) & 0xFF
        manual_command = None
        if key == ord('q'):
            break
        elif key == ord('w'):
            manual_command = "forward"
        elif key == ord('a'):
            manual_command = "left"
        elif key == ord('d'):
            manual_command = "right"
        elif key == ord('s'):
            manual_command = "backward"
        elif key == ord(' '):
            manual_command = "stop"

        if manual_command:
            client.publish("robot/manual_command", manual_command)
            print(f"Sent manual command: {manual_command}")

    # Cleanup
    cap.release()
    cv2.destroyAllWindows()
    client.loop_stop()
    client.disconnect()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nExiting...")
        sys.exit(0)