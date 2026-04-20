from PIL import Image
import os

frames = []
for i in range(1, 62):
    filename = os.path.join("imgs", f"{i}.png")
    if not os.path.exists(filename):
        raise FileNotFoundError(f"Not find: {filename}")
    frames.append(Image.open(filename))

frames[0].save(
    "output.gif",
    save_all=True,
    append_images=frames[1:],
    duration=100,
    loop=0 
)
