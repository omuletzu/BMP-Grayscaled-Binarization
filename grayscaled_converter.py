from PIL import Image

image_path = 'image.bmp'
image = Image.open(image_path)

grayscale_image = image.convert("L")
grayscale_image.save('imageGray.bmp')
