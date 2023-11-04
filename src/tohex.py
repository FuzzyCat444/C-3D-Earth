
rawImage = open('image.data', 'rb')
data = rawImage.read()
rawImage.close()

hexList = open('data.txt', 'w')

comma = ''
numBits = 0
bits = 0
numPixels = len(data) // 3
for i in range(numPixels):
    value = 1 if data[3 * i] == 255 else 0
    bits |= value << numBits
    numBits += 1
    if numBits == 64 or i == numPixels - 1:
        hexStr = comma + '0x' + '{:016X}'.format(bits)
        hexList.write(hexStr)
        bits = 0
        numBits = 0
        comma = ', '

hexList.close()