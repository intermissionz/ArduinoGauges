enum Sensor { oilPressure, afr, ethanolContent, boost, oilTemp, chargeTemp };

struct SensorData {
  int x;
  int y;
  double value;
  String label;  
};
