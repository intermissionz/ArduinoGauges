enum Sensor { oilPressure, afr, ethanolContent, boost, oilTemp, intakeAirTemp };

struct SensorData {
  int x;
  int y;
  double value;
  double tempValue;
  byte BTDataIn; //inData
  char BTCharIn; //inChar
  bool alert;
  String label;
  String displayValue;
  String precursorValue; //WorkingString
  String prePrecursorValue; //BuildINString
};

