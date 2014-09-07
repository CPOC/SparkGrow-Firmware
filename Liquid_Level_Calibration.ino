// Liquid Level Sensor Sketch

// Show the raw resistance values measured from an eTape liquid level sensor.
// See details on the sensor at:
//   https://www.adafruit.com/products/1786

// Created by Tony DiCola
// Released under an MIT license: http://opensource.org/licenses/MIT

// Configuration values:
#define SERIES_RESISTOR     560    // Value of the series resistor in ohms.    
#define SENSOR_PIN          0      // Analog pin which is connected to the sensor. 

// The following are calibration values you can fill in to compute the volume of measured liquid.
// To find these values first start with no liquid present and record the resistance as the
// ZERO_VOLUME_RESISTANCE value.  Next fill the container with a known volume of liquid and record
// the sensor resistance (in ohms) as the CALIBRATION_RESISTANCE value, and the volume (which you've
// measured ahead of time) as CALIBRATION_VOLUME.
#define ZERO_VOLUME_RESISTANCE    0.00    // Resistance value (in ohms) when no liquid is present.
#define CALIBRATION_RESISTANCE    0.00    // Resistance value (in ohms) when liquid is at max line.
#define CALIBRATION_VOLUME        0.00    // Volume (in any units) when liquid is at max line.
 
void setup(void) {
  Serial.begin(115200);
}
 
void loop(void) {
  // Measure sensor resistance.
  float resistance = readResistance(SENSOR_PIN, SERIES_RESISTOR);
  Serial.print("Resistance: "); 
  Serial.print(resistance, 2);
  Serial.println(" ohms");
  // Map resistance to volume.
  float volume = resistanceToVolume(resistance, ZERO_VOLUME_RESISTANCE, CALIBRATION_RESISTANCE, CALIBRATION_VOLUME);
  Serial.print("Calculated volume: ");
  Serial.println(volume, 5);
  // Delay for a second.
  delay(1000);
}

float readResistance(int pin, int seriesResistance) {
  // Get ADC value.
  float resistance = analogRead(pin);
  // Convert ADC reading to resistance.
  resistance = (1023.0 / resistance) - 1.0;
  resistance = seriesResistance / resistance;
  return resistance;
}

float resistanceToVolume(float resistance, float zeroResistance, float calResistance, float calVolume) {
  if (resistance > zeroResistance || (zeroResistance - calResistance) == 0.0) {
    // Stop if the value is above the zero threshold, or no max resistance is set (would be divide by zero).
    return 0.0;
  }
  // Compute scale factor by mapping resistance to 0...1.0+ range relative to maxResistance value.
  float scale = (zeroResistance - resistance) / (zeroResistance - calResistance);
  // Scale maxVolume based on computed scale factor.
  return calVolume * scale;
}
