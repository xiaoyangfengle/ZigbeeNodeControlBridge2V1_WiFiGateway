import Image
import math
 
def hsv_to_rgb(h, s, v):
    if s * v == 0:
        r, g, b =  v, v, v
    else:
        hi =  int(math.floor(3 * h / math.pi)) % 6
        f = 3 * h / math.pi - hi;
        p = v * (1.0 - s)
        q = v * (1.0 - f * s)
        t = v * (1.0 - (1.0 - f) * s)
        if hi == 0:
            r, g, b = v, t, p
        elif hi == 1:
            r, g, b = q, v, p
        elif hi == 2:
            r, g, b = p, v, t 
        elif hi == 3:
            r, g, b = p, q, v
        elif hi == 4:
            r, g, b = t, p, v
        else:
            r, g, b = v, p, q
    return r, g, b
 
def draw_circle(v):
    WIDTH = 400
    HEIGHT = 400
    CENTRE = 40.0
    R = 160.0
    FULL = 40.0
    center = (WIDTH / 2, HEIGHT / 2)
    band_r = Image.new('L', (WIDTH, HEIGHT))
    band_g = Image.new('L', (WIDTH, HEIGHT))
    band_b = Image.new('L', (WIDTH, HEIGHT))
    for y in range(HEIGHT):
        for x in range(WIDTH):
            tx, ty = x - center[0], y - center[1]
            d = math.sqrt(tx * tx + ty * ty)
            
            if d < CENTRE:
                band_r.putpixel((x,y), 255)
                band_g.putpixel((x,y), 255)
                band_b.putpixel((x,y), 255)
            else:
            #if 1:
                if d <= R:
                    if 0 <= ty:
                        t = math.acos(tx / d)
                    else:
                        t = 2 * math.pi - math.acos(tx / d)
                    s = (d / (R - CENTRE)) - (CENTRE / ((R - CENTRE)))
                    r, g, b = hsv_to_rgb(t, s, v)
                    band_r.putpixel((x,y), 255 * r)
                    band_g.putpixel((x,y), 255 * g)
                    band_b.putpixel((x,y), 255 * b)
                elif d <= (R + FULL):
                    if 0 <= ty:
                        t = math.acos(tx / d)
                    else:
                        t = 2 * math.pi - math.acos(tx / d)
                    r, g, b = hsv_to_rgb(t, 1.0, v)
                    band_r.putpixel((x,y), 255 * r)
                    band_g.putpixel((x,y), 255 * g)
                    band_b.putpixel((x,y), 255 * b)
                else:
                    band_r.putpixel((x,y), 255)
                    band_g.putpixel((x,y), 255)
                    band_b.putpixel((x,y), 255)
    im = Image.merge('RGB', (band_r, band_g, band_b))
    im.save('../www/img/hsv-shading.png')
 
if __name__ == '__main__':
    draw_circle(1.0)