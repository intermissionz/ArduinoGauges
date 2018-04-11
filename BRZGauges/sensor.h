enum Sensor { oilPressure, afr, ethanolContent, boost, oilTemp, intakeAirTemp };

struct SensorData {
  int x;
  int y;
  double value;
  String label;
  String displayValue;
};
