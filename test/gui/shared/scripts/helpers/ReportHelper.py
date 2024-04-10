from os import path
import pyscreenshot as ImageGrab


def take_screenshot(dir, filename):
    # screenshot
    screenshot = ImageGrab.grab()
    # save to a file
    screenshot.save(path.join(dir, filename))
