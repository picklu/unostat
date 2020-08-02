// simplestat
// Based on the paper "Microcontroller based potentiostat"
// DOI: 10.1021/acs.jchemed.5b00961 (J. Chem. Educ. 2016, 93, 1320−1322)
// 

// PWM and ADC pin on arduino uno
int DAC_OUT_R = 10;     // PWM
float ADC_IN_C = A0;    // ADC (Current)
float ADC_IN_V = A1;    // ADC (Current)

float potstep = 0.019608; // fixed due to the DAC resolution (1/0.019608 == 51)
int srs[] = {10,25,50,100,200,300}; //multiple scan rates values (mV/s)
int const count = 6;
int const data_count = 10;
long intervals[count];
int cdata[data_count];
float avg_data;

float c = 0;
float v = 0;
float r = 0;
int val = 0;
int inputs = 0;

String inputstr;

unsigned int halt;            // 0 for stop and 1 for run       
unsigned int sr;              // 0, 1, 2, 3, 4, and 5 
unsigned int mode;            // 0 for lsv and 1 for cv
int pstart;          // should be smaller than end point (0, 255)
int pend;            // should be larger than start point (0, 255)

void setup() {
  // setting up the device
  Serial.begin(9600);
  Serial.println("==== setting up =====");
  
  pinMode(DAC_OUT_R,OUTPUT);
  // Set PWM frequency to 31 kHz for pin 9 and 10 (Timer1)
  TCCR1B = TCCR1B & 0b11111000 | 0b00000001; //Set dividers to change PWM frequency
  
  delay(500);
  
  pinMode(ADC_IN_C,INPUT);
  pinMode(ADC_IN_V,INPUT);
  delay(500);
  
  // get the reference voltages
  analogWrite(DAC_OUT_R,127); // set potential and
  delay(200); // wait
  v = analogRead(ADC_IN_V);
  v = v * 255.0 / 1023;
  
  // reset val
  val = 0;
    
  //  populate intervalos
  for(int pos = 0; pos < count; pos++)
  {
    intervals[pos] = 1000000L /(srs[pos] * 51L);
  }

  Serial.println("======= done ========");
  broadcast_header();
  delay(500);
}

void loop() {
  // Output headers
  halt = 0;
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  analogWrite(DAC_OUT_R, 130); // set potential
  
  delay(100);
  
  inputs = get_inputs();
  if (inputs > 0 && halt == 0 && pend > pstart) {
    inputs = 0;

    switch(mode) {
      // if mode = 0 then forward scan
      case 0:
        forwardScan(sr);
        break;

      // if mode = 1 then reverse scan
      case 1:
        reverseScan(sr);
        break;

      // if mode = 2 then reverse scan follows forward scan (cycle)
      case 2:
        forwardScan(sr);
        reverseScan(sr);
        break;

      // if mode = 3 then forward scan follows reverse scan (cycle)
      case 3:
        reverseScan(sr);
        forwardScan(sr);
        break;

      // if mode = 4 or more then reverse scan follows forward scan ((mode -2)  no. of cycles)
      default:
        for (int i = 0; i < mode - 2; i++) {
          forwardScan(sr);
          reverseScan(sr);
        }
    }
  }  
}

void forwardScan(unsigned int pos) {
  for(val = pstart; val <= pend; val++)
  {
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }
    analogWrite(DAC_OUT_R,val); // set potential and
    delay(intervals[pos]); // wait

    // get data and average
    for (int i = 0; i < data_count && halt == 0; i++) {
      c = analogRead(ADC_IN_C);
      cdata[i] = c;
    }
    avg_data = average(cdata, data_count);
    
    // send data to the serial    
    broadcast_data(pos);
  }
}


void reverseScan(unsigned int pos) {
  for(val = pend; val >= pstart; val--)
  {
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }
    
    analogWrite(DAC_OUT_R,val);
    delay(intervals[pos]); // wait

    // get data and average
    for (int i = 0; i < data_count; i++) {
      c = analogRead(ADC_IN_C);
      cdata[i] = c;
    }
    avg_data = average(cdata, data_count);

    // send data to the serial
    broadcast_data(pos);
  }
}

float average(int numbers[], int count) {
  int sum = 0;
  for (int i = 0; i < count; i++) {
    sum += numbers[i];
  }
  return ((float)sum / count);
}

void broadcast_header() {
  Serial.print("Voltage");
  Serial.print(",");
  Serial.print("Current");
  Serial.print(",");
  Serial.print("vin");
  Serial.print(",");
  Serial.print("sr");
  Serial.print(",");
  Serial.print("delay");
  Serial.print(",");
  Serial.println();
}

void broadcast_data(int pos) {
  Serial.print(val);
  Serial.print(",");
  Serial.print(avg_data);
  Serial.print(",");
  Serial.print(v);
  Serial.print(",");
  Serial.print(srs[pos]);
  Serial.print(",");  
  Serial.print(intervals[pos]);
  Serial.print(",");
  Serial.println();
}

int get_inputs() {
  int result = 0;
  
  if (Serial.available()) {
    inputstr = String("");
    while (Serial.available())
    {
      delay(10);
      if (Serial.available() >0)
      {
        char c = Serial.read();
        inputstr = String(inputstr + c);
      }
    }
    
    // s => "sr,halt,mode,pstart,pend"
    result = sscanf(inputstr.c_str(), "%u,%u,%u,%d,%d\r", &sr, &halt ,&mode, &pstart, &pend);
  }
  
  return result;
}
