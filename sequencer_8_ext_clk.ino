/*
8 Step Sequencer
With MIDI output
This version is controlled by external clk / reset input
 */
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

int currentStep = 8;
int midiNoteNum = 0;
int currentMidiNoteNum = 0;
bool clk_active = false;
int ext_reset = 0;
bool manual_mode = false;
int manual_step;
int last_manual_step = 0;

#define STEP1_PIN 2
#define STEP2_PIN 3
#define STEP3_PIN 4
#define STEP4_PIN 5
#define STEP5_PIN 6
#define STEP6_PIN 7
#define STEP7_PIN 8
#define STEP8_PIN 9
#define INT_RESET_PIN 10
#define MANUAL_PIN A2
#define CV_PIN A3 // control voltage
#define CLK_IN_PIN 15 //A1
#define GATE_EN_PIN 11
#define SEL0_PIN 14 //A0
#define SEL1_PIN 13
#define SEL2_PIN 12
#define EXT_RESET_PIN 0
// the setup routine runs once when you press reset:
void setup() {    
  MIDI.begin(MIDI_CHANNEL_OFF);
  MIDI.turnThruOff();                  
  pinMode(STEP1_PIN, OUTPUT);     
  pinMode(STEP2_PIN, OUTPUT);     
  pinMode(STEP3_PIN, OUTPUT);     
  pinMode(STEP4_PIN, OUTPUT);     
  pinMode(STEP5_PIN, OUTPUT);     
  pinMode(STEP6_PIN, OUTPUT);     
  pinMode(STEP7_PIN, OUTPUT);     
  pinMode(STEP8_PIN, OUTPUT);
  pinMode(CLK_IN_PIN, INPUT_PULLUP);
  pinMode(EXT_RESET_PIN, INPUT_PULLUP);
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
  digitalWrite(SEL0_PIN, LOW);
  digitalWrite(SEL1_PIN, LOW);
  digitalWrite(SEL2_PIN, LOW);
  pinMode(GATE_EN_PIN, INPUT);
  setNewStep(1);
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
void setStepPin(int step, int state) {
  digitalWrite(step + 1, state);
}

void setNewStep(int step) {
  // find out if we need to reset to first step by reading the step reset pin
  int stepReset = 0;
  if (digitalRead(INT_RESET_PIN) == HIGH) {
    stepReset = 1;
  }
  // turn off the current step pin
  setStepPin(currentStep, LOW);
  // advance to next step
  if (step == 0) {
    if ((currentStep == 8) || (stepReset == 1)) {
      currentStep = 1;
    } else {
      currentStep++;
    }
  } else {
    currentStep = step;
  }
  // turn on the new step pin
  setStepPin(currentStep, HIGH);
  setAnalogMux();
}

void getControlVoltage() {
  // read the analogue control voltage and compute midi note number
  // midi note range is 21-84
  midiNoteNum = (analogRead(CV_PIN) >> 4) + 21;
}

void noteOn(int note) {
  MIDI.sendNoteOn(note,127,1);
}

void noteOff(int note) {
  MIDI.sendNoteOn(note,0,1);
}

void getManualMode() {
  // use tempo pot to control step when creating pattern
  int pot_val = analogRead(MANUAL_PIN);
  if ((pot_val < 8) || (pot_val > 1016)) {
    manual_mode = false;
  } else {
    manual_mode = true;
    manual_step = (pot_val >> 7) + 1;
  }
}

void loop() {
  getManualMode();
  if (manual_mode == true) {
    if (manual_step != currentStep) {
      setNewStep(manual_step);
      delay(1); // give time for analogue mux to settle
    }
    getControlVoltage();
    if (midiNoteNum != currentMidiNoteNum) {
      noteOn(midiNoteNum);
      noteOff(currentMidiNoteNum);
      currentMidiNoteNum = midiNoteNum;
    }
  } else {
    // normal mode
    // note that external clk and reset inputs are inverted by transistor buffers so active low here
    if (clk_active == false) {
      // wait for rising edge
      if (digitalRead(CLK_IN_PIN) == 0) {
        clk_active = true;
        if (digitalRead(EXT_RESET_PIN) == 0) {
          ext_reset = 1;
        } else {
          ext_reset = 0;
        }
        setNewStep(ext_reset);
        delay(1); // give time for analogue mux signal to settle
        if (digitalRead(GATE_EN_PIN) == 1) {
          getControlVoltage();
          noteOn(midiNoteNum);
          currentMidiNoteNum = midiNoteNum;
        }
      }
    } else {
      // wait for falling edge
      if (digitalRead(CLK_IN_PIN) == 1) {
        clk_active = false;
        noteOff(currentMidiNoteNum);
      }
    }
  }
}
