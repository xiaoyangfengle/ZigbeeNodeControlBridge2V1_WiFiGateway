import Image
import math

def k_to_rgb(kelvin):
    temp = kelvin / 100
    
    if (temp <= 66):
        red = 255
        
        green = 99.4708025861 * math.log(temp) - 161.1195681661
        
        if( temp <= 19):
            blue = 0
        else:
            blue = 138.5177312231 * math.log(temp-10) - 305.0447927307

    else:
        red = 329.698727446 * math.pow(temp - 60, -0.1332047592)

        green = 288.1221695283 * math.pow(temp - 60, -0.0755148492 )

        blue = 255
    
    if (red > 255):     red = 255
    if (green > 255):   gren = 255
    if (blue > 255):    blue = 255
    
    return (int(red), int(green), int(blue))


def draw_scale(v):
    MIN_TEMP = 1000
    MAX_TEMP = 10000
    
    WIDTH = 1
    HEIGHT = 400
    
    im = Image.new('RGB', (WIDTH, HEIGHT))
    
    for y in range(HEIGHT):
        kelvin = MIN_TEMP + (MAX_TEMP-MIN_TEMP) * (float(y) / HEIGHT)
        
        y = HEIGHT-y-1
        
        print "Y: %d Kelvin: %d" % (y, kelvin)
        
        pixel = k_to_rgb(kelvin)
        print pixel
        
        for x in range(WIDTH):
            im.putpixel((x,y), pixel)
        
    im.save('../www/img/colour-temperature.png')
 
if __name__ == '__main__':
    draw_scale(1.0)