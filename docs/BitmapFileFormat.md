
## BMP File Format 

### Bitmap File Header
1. Signature (2bytes) | 2bytes blank
2. File Size (4bytes)
3. Reserved1, 2 (4bytes in total)
4. File Offset to PixelArray (4bytes)

### Read DIB Header (BITMAPV5HEADER)
1. DIB Header Size
2. Image Width
3. Image Height
4. Planes, Bits per Pixel
5. Compression
6. Image Size
7. X Pixels Per Meter
8. Y Pixels Per Meter
9. Colors in Color Table
10. Important Color Count
11. Red channel bitmask 
12. Green channel bitmask
13. Blue channel bitmask
14. Alpha channel bitmask
15. Color Space Type
16. Color Space Endpoints (36bytes)
17. Gamma for Red channel
18. Gamma for Green channel
19. Gamma for Blue channel
20. Intent
21. ICC Profile Data
22. ICC Profile Size
23. Reserved

### Color Table (semi-optional)
- The attribute "Colors in Color Table" of the DIB Header bytes describes it. 
- **The presence of the Color Table is mandatory when Bits per Pixel <= 8**
- **The size of Color Table Entries is 3 Bytes if BITMAPCOREHEADER is substituted for BITMAPV5HEADER**
1. Color Definition (index 0)
2. Color Definition (index 1)
3. Color Definition (index 2)
n. Color Definition (index n)

### GAP1 (optional)

### Image Data (PixelArray[x, y])
- Each pixels will follow the pixel format by some attributes of DIB Header. (Bits per Pixel, Compression each RGB and Alpha channels)
- Pad row size to a multiple of 4 Bytes

| Pixel [0, h- 1] | Pixel [1, h-1] | Pixel [w-1, h-1] | Padding (4bytes) |

### GAP2 (optional)

### ICC Color Profile (optional)
- **The ICC Color Profile may be present only when the BITMAPV5HEADER is used.**
- Embedded, variable size ICC Color Profile Data (or path to a linked file containing ICC Color Profile Data)

## References
- [wikipedia](https://en.wikipedia.org/wiki/BMP_file_format)