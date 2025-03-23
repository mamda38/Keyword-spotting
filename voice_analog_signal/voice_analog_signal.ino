#include <I2S.h>

#define WINDOW_SIZE 20  // Tăng kích thước cửa sổ bộ lọc

int sampleBuffer[WINDOW_SIZE];  
int bufferIndex = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  I2S.setAllPins(-1, 42, 41, -1, -1);
  if (!I2S.begin(PDM_MONO_MODE, 16000, 16)) {  // Giảm tần số lấy mẫu
    Serial.println("Failed to initialize I2S!");
    while (1);
  }

  for (int i = 0; i < WINDOW_SIZE; i++) {
    sampleBuffer[i] = 0;
  }
}

void loop() {
  int sample = I2S.read();

  if (sample && sample != -1 && sample != 1) {
    sampleBuffer[bufferIndex] = sample;
    bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;

    long sum = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
      sum += sampleBuffer[i];
    }
    int filteredSample = sum / WINDOW_SIZE;

    Serial.println(filteredSample);
  }
  delay(50);  // Giảm tốc độ gửi Serial
}
