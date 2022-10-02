/*
8 Step Sequencer
With MIDI output
 */
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

int currentStep = 8;
unsigned long previousMillis = 0;
int sequencerStopped = 0;
int gateOff_ms = 100;
int gateOn_ms = 0;
int gateOn = 0;
int midiNoteNum = 0;
int lastMidiNoteNum = 0;

#define STEP1_PIN 2
#define STEP2_PIN 3
#define STEP3_PIN 4
#define STEP4_PIN 5
#define STEP5_PIN 6
#define STEP6_PIN 7
#define STEP7_PIN 8
#define STEP8_PIN 9
#define STEP_RESET_PIN 10
#define TEMPO_PIN A2
#define CV_PIN A3 // control voltage
#define GATE_PIN 15 //A1
#define GATE_EN_PIN 11
#define SEL0_PIN 14 //A0
#define SEL1_PIN 13
#define SEL2_PIN 12

// the setup routine runs once when you press reset:
void setup() {    
  MIDI.begin(MIDI_CHANNEL_OMNI);                  
  pinMode(STEP1_PIN, OUTPUT);     
  pinMode(STEP2_PIN, OUTPUT);     
  pinMode(STEP3_PIN, OUTPUT);     
  pinMode(STEP4_PIN, OUTPUT);     
  pinMode(STEP5_PIN, OUTPUT);     
  pinMode(STEP6_PIN, OUTPUT);     
  pinMode(STEP7_PIN, OUTPUT);     
  pinMode(STEP8_PIN, OUTPUT);
  pinMode(GATE_PIN, OUTPUT);
  pinMode(SEL0_PIN, OUTPUT);
  pinMode(SEL1_PIN, OUTPUT);
  pinMode(SEL2_PIN, OUTPUT);
  digitalWrite(STEP1_PIN, LOW);     
  digitalWrite(STEP2_PIN, LOW);     
  digitalWrite(STEP3_PIN, LOW);     
  digitalWrite(STEP4_PIN, LOW);     
  digitalWrite(STEP5_PIN, LOW);     
  digitalWrite(STEP6_PIN, LOW);     
  digitalWrite(STEP7_PIN, LOW);     
  digitalWrite(STEP8_PIN, LOW);
  digitalWrite(GATE_PIN, LOW);
  digitalWrite(SEL0_PIN, LOW);
  digitalWrite(SEL1_PIN, LOW);
  digitalWrite(SEL2_PIN, LOW);
  pinMode(STEP_RESET_PIN, INPUT);
  pinMode(GATE_EN_PIN, INPUT);
  setNewStep(); // get things started
}

void setAnalogMux() {
  switch (currentStep) {
    case 1:
      digitalWrite(SEL0_PIN, LOW);
      digitalWrite(SEL1_PIN, LOW);
      digitalWrite(SEL2_PIN, LOW);
      break;
    case 2:
      digitalWrite(SEL0_PIN, HIGH);
      digitalWrite(SEL1_PIN, LOW);
      digitalWrite(SEL2_PIN, LOW);
      break;
    case 3:
      digitalWrite(SEL0_PIN, LOW);
      digitalWrite(SEL1_PIN, HIGH);
      digitalWrite(SEL2_PIN, LOW);
      break;
    case 4:
      digitalWrite(SEL0_PIN, HIGH);
      digitalWrite(SEL1_PIN, HIGH);
      digitalWrite(SEL2_PIN, LOW);
      break;
    case 5:
      digitalWrite(SEL0_PIN, LOW);
      digitalWrite(SEL1_PIN, LOW);
      digitalWrite(SEL2_PIN, HIGH);
      break;
    case 6:
      digitalWrite(SEL0_PIN, HIGH);
      digitalWrite(SEL1_PIN, LOW);
      digitalWrite(SEL2_PIN, HIGH);
      break;
    case 7:
      digitalWrite(SEL0_PIN, LOW);
      digitalWrite(SEL1_PIN, HIGH);
      digitalWrite(SEL2_PIN, HIGH);
      break;
    case 8:
      digitalWrite(SEL0_PIN, HIGH);
      digitalWrite(SEL1_PIN, HIGH);
      digitalWrite(SEL2_PIN, HIGH);
      break;
  }
}

void setNewStep() {
  // find out if we need to reset to first step by reading the step reset pin
  int stepReset = 0;
  if (digitalRead(STEP_RESET_PIN) == HIGH) {
    stepReset = 1;
  }
  // turn off the current step pin
  int pin = 1 + currentStep;
  digitalWrite(pin, LOW);
  // advance to next step
  if (currentStep == 8 || stepReset == 1) {
    currentStep = 1;
  }
  else {
    currentStep++;
  }
  pin = currentStep + 1;
  // turn on the new step pin
  digitalWrite(pin, HIGH);
  setAnalogMux();
}

int getTempo() {
  // read the tempo pot
  int potVal = analogRead(TEMPO_PIN);
  int ret;
  if (potVal < 5) {
    sequencerStopped = 1; // stopped
  }
  else {
    sequencerStopped = 0;
  }
  return (1050 - potVal);
}

void setGate(int onOff) {
  if (onOff) {
    if (digitalRead(GATE_EN_PIN) == 1) {
      MIDI.sendNoteOn(midiNoteNum,127,1);
      digitalWrite(GATE_PIN, HIGH);
    }
    gateOn = 1;
  }
  else {
    MIDI.sendNoteOn(midiNoteNum,0,1); // note off
    digitalWrite(GATE_PIN, LOW);
    gateOn = 0;
  }
}

void getControlVoltage() {
  // read the analogue control voltage and compute midi note number
  // midi note range is 21-84
  midiNoteNum = (analogRead(CV_PIN) >> 4) + 21;
}

void sendNote(int onOff) {
  // midi note on/off
  if (onOff) {
    // note on
    MIDI.sendNoteOn(midiNoteNum,127,1);
  } else {
    // note off
    MIDI.sendNoteOn(midiNoteNum,0,1);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  gateOn_ms = getTempo();
  if (gateOn_ms < 50) {
    gateOff_ms = gateOn_ms;
  }
  else {
    gateOff_ms = 50;
  }
  if (gateOn == 0) {
    if (currentMillis - previousMillis > gateOff_ms) {
      previousMillis = currentMillis;
      getControlVoltage();
      setGate(1);
    }
  }
  else {
    if (currentMillis - previousMillis > gateOn_ms) {
      previousMillis = currentMillis;
      if (sequencerStopped == 0) {
        setGate(0);
        setNewStep();
      }
    }
  }
  if (sequencerStopped == 1) {
    // update midi when sequencer is stopped
    getControlVoltage();
    if (midiNoteNum != lastMidiNoteNum) {
      MIDI.sendNoteOn(midiNoteNum,127,1); // new note on
      MIDI.sendNoteOn(lastMidiNoteNum,0,1); // old note off
    }
  }
  lastMidiNoteNum = midiNoteNum;
}
