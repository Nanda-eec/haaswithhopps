import requests
import time
import os
import re
from datetime import datetime

# --- Configuration ---
ESP32_CAM_IP = "192.168.1.123"  # Replace with your ESP32-CAM's IP address
IMAGE_URL = f"http://{ESP32_CAM_IP}/jpg"
ROOT_URL = f"http://{ESP32_CAM_IP}/"  # URL for getting GPS data (root URL)
SAVE_DIR_BASE = "captured_images"
REQUEST_INTERVAL = 5  # Seconds

def create_save_directory():
    """Creates a new directory with a timestamp."""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    save_dir = os.path.join(SAVE_DIR_BASE, timestamp)
    os.makedirs(save_dir, exist_ok=True)  # Create the directory if it doesn't exist
    return save_dir

def get_gps_data():
    """Fetches GPS data from the ESP32-CAM's root URL."""
    try:
        response = requests.get(ROOT_URL, timeout=10)  # Add a timeout
        response.raise_for_status()  # Raise an exception for bad status codes (4xx or 5xx)

        # Extract GPS data using regular expressions (more robust than string splitting)
        html_content = response.text
        match = re.search(r"Latitude:\s*(-?\d+\.\d+),\s*Longitude:\s*(-?\d+\.\d+),\s*Satellites:\s*(\d+),\s*Altitude:\s*(-?\d+\.?\d*)\s*([a-zA-Z]*)", html_content)

        if match:
            latitude, longitude, satellites, altitude, units = match.groups()
            return f"Lat{latitude}_Lon{longitude}_Sat{satellites}_Alt{altitude}{units}"
        else:
            print("Warning: Could not parse GPS data from HTML.")
            return "NoGPSData"  # Return a placeholder if GPS data can't be parsed

    except requests.exceptions.RequestException as e:
        print(f"Error fetching GPS data: {e}")
        return "RequestFailed" # Return error message

def capture_and_save_image(save_dir):
    """Captures an image from the ESP32-CAM and saves it with GPS data."""
    try:
        response = requests.get(IMAGE_URL, stream=True, timeout=10)
        response.raise_for_status()  # Check for HTTP errors

        gps_data_string = get_gps_data() # Get gps data
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"{timestamp}_{gps_data_string}.jpg"
        filepath = os.path.join(save_dir, filename)

        with open(filepath, 'wb') as f:
            for chunk in response.iter_content(chunk_size=8192):
                f.write(chunk)
        print(f"Image saved to: {filepath}")

    except requests.exceptions.RequestException as e:
        print(f"Error capturing image: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        

def main():
    """Main function to capture images at intervals."""

    save_dir = create_save_directory()
    print(f"Saving images to: {save_dir}")

    while True:
        capture_and_save_image(save_dir)
        time.sleep(REQUEST_INTERVAL)

if __name__ == "__main__":
    main()