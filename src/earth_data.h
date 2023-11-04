#ifndef EARTH_DATA_H
#define EARTH_DATA_H

#include <stdint.h>

int sampleEarthData(int x, int y);

#define EARTH_DATA_WIDTH 512
#define EARTH_DATA_HEIGHT 256
#define EARTH_DATA_SIZE 2048
extern uint64_t earthData[EARTH_DATA_SIZE];

#endif