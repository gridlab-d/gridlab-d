"""

    AI Incubator Prompt:

    I need a Python script that will follow all the links from a specified page, go to each page in turn, save each image off the page, and save those images into a single folder, preserving the original file names of the images.


    """

import os
import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin
from tqdm import tqdm  # Optional: For progress indication (install via `pip install tqdm`)

def download_image(url, folder):
    """
    Download an image from the given URL and save it in the specified folder.
    """
    try:
        response = requests.get(url, stream=True)
        response.raise_for_status()  # Check if the request was successful
        filename = os.path.basename(url)  # Extract the file name from the image URL
        filepath = os.path.join(folder, filename)
        with open(filepath, 'wb') as f:
            for chunk in response.iter_content(1024):
                f.write(chunk)
        print(f"Saved: {filename}")
    except Exception as e:
        print(f"Failed to download {url}: {e}")

def get_all_links(url):
    """
    Retrieve all links from a specified webpage.
    """
    try:
        response = requests.get(url)
        response.raise_for_status()
        soup = BeautifulSoup(response.text, 'html.parser')
        all_links = [urljoin(url, a['href']) for a in soup.find_all('a', href=True)]
        return all_links
    except Exception as e:
        print(f"Failed to retrieve links from {url}: {e}")
        return []

def download_images_from_page(url, folder):
    """
    Download all images from a specific webpage.
    """
    try:
        response = requests.get(url)
        response.raise_for_status()
        soup = BeautifulSoup(response.text, 'html.parser')
        img_tags = soup.find_all('img')
        for img_tag in img_tags:
            img_url = urljoin(url, img_tag['src'])
            download_image(img_url, folder)
    except Exception as e:
        print(f"Failed to download images from {url}: {e}")

def main(start_url, folder):
    """
    Main function to crawl links, download images, and save them.
    """
    if not os.path.exists(folder):
        os.makedirs(folder)

    # Step 1: Get all links from the starting page
    links = get_all_links(start_url)

    # Step 2: Download images from each link
    print(f"Found {len(links)} links. Processing...")
    for link in tqdm(links, desc="Processing links"):
        download_images_from_page(link, folder)

if __name__ == "__main__":
    # URL of the webpage to start from
    start_url = "https://GridLAB-D™.shoutwiki.com/wiki/Index"

    # Folder to save images
    folder = "docs\images"

    main(start_url, folder)