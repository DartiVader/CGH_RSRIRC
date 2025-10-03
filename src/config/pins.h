#ifndef PINS_H
#define PINS_H

// Пины для приемника
#ifdef RECEIVER_NODE
#define ULTRASOUND_RX_PIN 36    // АЦП для датчика звука
#define TFT_DC_PIN 2
#define TFT_CS_PIN 5
#define STATUS_LED 2
#endif

// Пины для маяков
#ifdef BEACON_NODE
#define ULTRASOUND_TX_PIN 25    // Для зуммера
#define STATUS_LED 2
#endif

#endif
