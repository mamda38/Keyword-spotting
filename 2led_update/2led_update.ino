/* Edge Impulse Arduino examples
 * Optimized for embedded applications
 */

// If your target is limited in memory remove this macro to save 10K RAM
#define EIDSP_QUANTIZE_FILTERBANK 0

/* Includes ---------------------------------------------------------------- */
#include <test9_inferencing.h>
#include <I2S.h>

// Constants
#define SAMPLE_RATE 16000U
#define SAMPLE_BITS 16
#define LED_1 2
#define LED_2 3
#define CONFIDENCE_THRESHOLD 0.77f
#define INFERENCE_DELAY 100  // ms between inferences
#define SERIAL_TIMEOUT 5000  // ms to wait for serial

// Pin definitions
const int8_t I2S_MIC_SERIAL_CLOCK = 42;
const int8_t I2S_MIC_SERIAL_DATA = 41;

/** Audio buffers, pointers and selectors */
typedef struct {
  int16_t *buffer;
  uint8_t buf_ready;
  uint32_t buf_count;
  uint32_t n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 4096;
static signed short sampleBuffer[sample_buffer_size];
static bool debug_nn = false;
static bool record_status = true;
static TaskHandle_t sampling_task_handle = NULL;

// Function prototypes
static bool microphone_inference_start(uint32_t n_samples);
static bool microphone_inference_record(void);
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
static void microphone_inference_end(void);
static void capture_samples(void *arg);
static void audio_inference_callback(uint32_t n_bytes);

/**
 * @brief      Arduino setup function
 */
void setup() {
  // Initialize serial at high baud rate
  Serial.begin(115200);
  
  // Don't wait too long for serial to avoid blocking in production
  uint32_t serialStartTime = millis();
  while (!Serial && (millis() - serialStartTime < SERIAL_TIMEOUT));
  
  Serial.println("Edge Impulse Inferencing Demo");

  // Configure LED pins
  pinMode(LED_1, OUTPUT);  
  pinMode(LED_2, OUTPUT);
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);

  // Initialize I2S with specific pins for this board
  I2S.setAllPins(-1, I2S_MIC_SERIAL_CLOCK, I2S_MIC_SERIAL_DATA, -1, -1);
  if (!I2S.begin(PDM_MONO_MODE, SAMPLE_RATE, SAMPLE_BITS)) {
    Serial.println("Failed to initialize I2S!");
    // Continue anyway - don't block in production
  }

  // Only print debug info when serial is connected
  if (Serial) {
    // summary of inferencing settings
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");
    ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf(" ms.\n");
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
    ei_printf("\nStarting continuous inference in 2 seconds...\n");
  }
  
  delay(1000); // Shorter delay for production

  if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
    if (Serial) {
      ei_printf("ERR: Could not allocate audio buffer (size %d)\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    }
    // Continue anyway - don't block in production
  }

  if (Serial) {
    ei_printf("Recording...\n");
  }
}

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void loop() {
  // Record audio
  if (!microphone_inference_record()) {
    // If recording fails, try again after a short delay
    delay(10);
    return;
  }

  // Short delay before processing
  delay(INFERENCE_DELAY);

  // Set up signal for classifier
  signal_t signal;
  signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
  signal.get_data = &microphone_audio_signal_get_data;
  ei_impulse_result_t result = { 0 };

  // Run the classifier
  EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
  if (r != EI_IMPULSE_OK) {
    if (Serial) {
      ei_printf("ERR: Failed to run classifier (%d)\n", r);
    }
    return;
  }

  // Find the prediction with highest confidence
  int pred_index = 0;
  float pred_value = 0;
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    if (result.classification[ix].value > pred_value) {
      pred_index = ix;
      pred_value = result.classification[ix].value;
    }
  }

  // Only print debug info when serial is connected
  if (Serial) {
    ei_printf("Predictions (DSP: %d ms., Classification: %d ms.): \n",
              result.timing.dsp, result.timing.classification);
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
      ei_printf("    %s: ", result.classification[ix].label);
      ei_printf_float(result.classification[ix].value);
      ei_printf("\n");
    }
  }

  // Process commands only if confidence is high enough
  if (pred_value > CONFIDENCE_THRESHOLD) {
    switch (pred_index) {
      case 0:
        digitalWrite(LED_1, HIGH);
        if (Serial) Serial.println("LED1 ON");
        break;
      case 3:
        digitalWrite(LED_1, LOW);
        if (Serial) Serial.println("LED1 OFF");
        break;
      case 1:
        digitalWrite(LED_2, HIGH);
        if (Serial) Serial.println("LED2 ON");
        break;
      case 4:
        digitalWrite(LED_2, LOW);
        if (Serial) Serial.println("LED2 OFF");
        break;
    }
  }
}

static void audio_inference_callback(uint32_t n_bytes) {
  for (int i = 0; i < n_bytes >> 1; i++) {
    inference.buffer[inference.buf_count++] = sampleBuffer[i];

    if (inference.buf_count >= inference.n_samples) {
      inference.buf_count = 0;
      inference.buf_ready = 1;
    }
  }
}

static void capture_samples(void *arg) {
  const int32_t i2s_bytes_to_read = (uint32_t)arg;
  size_t bytes_read = i2s_bytes_to_read;

  while (record_status) {
    // Read data from I2S
    esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, (void *)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

    if (bytes_read <= 0) {
      // Error handling - wait a bit before retrying
      delay(10);
      continue;
    }

    // Scale the data (amplify the sound)
    for (int x = 0; x < i2s_bytes_to_read / 2; x++) {
      sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
    }

    if (record_status) {
      audio_inference_callback(i2s_bytes_to_read);
    } else {
      break;
    }
  }
  vTaskDelete(NULL);
}

/**
 * @brief      Init inferencing struct and setup/start PDM
 */
static bool microphone_inference_start(uint32_t n_samples) {
  inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));

  if (inference.buffer == NULL) {
    return false;
  }

  inference.buf_count = 0;
  inference.n_samples = n_samples;
  inference.buf_ready = 0;

  delay(100);

  record_status = true;

  // Create task with higher stack size and priority for audio sampling
  BaseType_t task_created = xTaskCreate(
    capture_samples,
    "CaptureSamples",
    1024 * 32,
    (void *)sample_buffer_size,
    10,
    &sampling_task_handle
  );
  
  return (task_created == pdPASS);
}

/**
 * @brief      Wait on new data
 */
static bool microphone_inference_record(void) {
  // Use a timeout to avoid blocking forever
  uint32_t start_time = millis();
  while (inference.buf_ready == 0) {
    delay(10);
    // Timeout after 1 second to prevent hanging
    if (millis() - start_time > 1000) {
      return false;
    }
  }

  inference.buf_ready = 0;
  return true;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
  numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
  return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void) {
  // Stop the sampling task first
  if (sampling_task_handle != NULL) {
    record_status = false;
    // Wait for task to delete itself
    delay(100);
  }
  
  // Free memory
  free(sampleBuffer);
  ei_free(inference.buffer);
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif