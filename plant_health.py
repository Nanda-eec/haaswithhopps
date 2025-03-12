#!/usr/bin/env python
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

def calculate_health_score(image_path, method='modified_ndvi'):
    """
    Calculates a plant health score (1-100) from an image.

    Args:
        image_path: Path to the image file.
        method:  The method to use ('ndvi', 'modified_ndvi', 'red_channel', 'green_red_ratio').
                 'ndvi' (standard NDVI), 'modified_ndvi' (as in the original code).
                 'red_channel' uses just the red channel intensity.
                 'green_red_ratio' uses the ratio of green to red channels.

    Returns:
        A health score between 1 and 100, or None if an error occurs.  Returns
        the NDVI image (or other calculated image) as a numpy array as the second
       
         return value.
    """
    try:
        img = plt.imread(image_path)
        if len(img.shape) != 3 or img.shape[2] < 3:
            print("Error: Image must be a color image (at least 3 channels).")
            return None, None

        # Extract color channels (ensure float for calculations)
        red = img[:, :, 0].astype(float)
        green = img[:, :, 1].astype(float)
        blue = img[:, :, 2].astype(float)

        if method == 'ndvi':
            # Standard NDVI calculation (requires near-infrared channel)
            if img.shape[2] < 4: #Often the 4th is NIR
                print("Error: Standard NDVI requires a near-infrared channel (usually the 4th channel).")
                return None, None
            nir = img[:,:,3].astype(float)
            ndvi = (nir - red) / (nir + red + 1e-8)  # Add small value to avoid division by zero
            ndvi_image = ndvi

        elif method == 'modified_ndvi':
            # Modified NDVI from the original code
            bottom = (blue - green) ** 2
            bottom[bottom == 0] = 1e-8  # Avoid division by zero.  Use a small number.
            vis = (blue + green) ** 2 / bottom
            ndvi = (red - vis) / (red + vis + 1e-8) #ndvi calculation with red as NIR
            ndvi_image = ndvi

        elif method == 'red_channel':
            # Use red channel intensity (lower red = healthier)
            ndvi_image = 1 - (red / 255.0) # Normalize to 0-1, then invert

        elif method == 'green_red_ratio':
            # Ratio of Green to Red (Higher green/red = healthier)
            ndvi_image = green / (red + 1e-8) #prevent division by 0

        else:
            print("Error: Invalid method specified.")
            return None, None
        
        # Clip NDVI/health indicator to -1 to 1 range (for consistent scoring)
        ndvi_image = np.clip(ndvi_image, -1, 1)

        # Calculate the health score (scale and shift to 1-100)
        # We use mean of the image.  This is a reasonable overall health indicator.
        health_score = (np.mean(ndvi_image) + 1) / 2 * 99 + 1  # Map [-1, 1] to [0, 1], then to [1, 100]
        health_score = np.clip(health_score, 1, 100)  # Ensure score is within 1-100

        return int(round(health_score)), ndvi_image


    except FileNotFoundError:
        print(f"Error: File not found: {image_path}")
        return None, None
    except Exception as e:
        print(f"An error occurred: {e}")
        return None, None


def visualize_ndvi(ndvi_image, output_path="ndvi_visualization.png"):
    """Visualizes the NDVI image and saves it."""
    plt.figure(figsize=(8, 6))
    plt.imshow(ndvi_image, cmap='RdYlGn')  # Use a suitable colormap
    plt.colorbar(label='NDVI')
    plt.title('NDVI Visualization')
    plt.axis('off')  # Hide axes
    plt.savefig(output_path)
    plt.close()



def main():
    image_path = input("Enter the path to the image file: ")
    # Get method choice from the user
    print("Choose a method for health calculation:")
    print("1. Standard NDVI (requires NIR channel)")
    print("2. Modified NDVI (as in original code)")
    print("3. Red Channel Intensity")
    print("4. Green/Red Ratio")
    method_choice = input("Enter choice (1-4): ")

    if method_choice == '1':
        method = 'ndvi'
    elif method_choice == '2':
        method = 'modified_ndvi'
    elif method_choice == '3':
        method = 'red_channel'
    elif method_choice == '4':
        method = 'green_red_ratio'
    else:
        print("Invalid method choice.  Using 'modified_ndvi'.")
        method = 'modified_ndvi'


    health_score, ndvi_image = calculate_health_score(image_path, method)

    if health_score is not None:
        print(f"Plant Health Score: {health_score}")

        # Ask if the user wants to visualize the NDVI
        visualize = input("Visualize the NDVI image (y/n)? ").lower()
        if visualize == 'y':
            visualize_ndvi(ndvi_image)
            print("NDVI visualization saved to ndvi_visualization.png")


if __name__ == "__main__":
    main()