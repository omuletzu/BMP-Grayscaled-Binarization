# BMP Binarization for grayscaled image

### The main problem to be solved within this project is the acceleration of processing a BMP image stored on an SD card by binarizing the image using an FPGA. Binarization involves converting each pixel of the image into a single bit (black or white) based on a predefined threshold, which can be useful for shape recognition applications or image processing for artificial vision.

- The main objectives of the project are as follows:

- Reading a BMP file from an SD card: Initially, the image must be read from a file stored on the SD card.

- Processing the image in FPGA: After reading the image, it will be sent to the FPGA for binarization, in accordance with a specified threshold.

- Writing the processed image back to the SD card: After processing, the binarized image will be saved to the SD card for later use.

- Using DMA technology: The transfer of data between PS and PL will be done via DMA, allowing for fast and efficient data transfer between the two sections.
