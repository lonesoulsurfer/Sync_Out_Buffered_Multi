
/* Synth Sync Signal Splitter with BPM display and clock division/groove
  Creates 6 buffered sync output signals from 1 input signal
  For Arduino Nano
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // Display driver

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// Constants for groove patterns
#define GROOVE_STRAIGHT 0    // No groove
#define GROOVE_SWING_8TH 1   // Standard swing feel
#define GROOVE_SHUFFLE 2     // Triplet-based shuffle
#define GROOVE_HUMANIZE 3    // Random humanization

// Groove settings (-8 to -1 now represent groove patterns instead of multiplication)
int grooveAmount[] = {0, 0, 0, 0, 0, 0}; // Amount of groove (0-100%)
int grooveType[] = {GROOVE_STRAIGHT, GROOVE_STRAIGHT, GROOVE_STRAIGHT, GROOVE_STRAIGHT, GROOVE_STRAIGHT, GROOVE_STRAIGHT};

const int syncInPin = A0;    // Analog input pin for sync signal
const int syncOutPins[] = {2, 3, 4, 5, 6, 7}; // Six digital output pins
const int indicatorLED = 8;  // LED to indicate sync pulses
const unsigned long minPulseWidth = 30;  // Increased pulse width for better synth compatibility
const int threshold = 400;   // Adjusted for 2V input

// Button pins
const int selectButton = 9;  // Button to select which output to adjust
const int divideButton = 10; // Button to increase division
const int multiplyButton = 11; // Button to increase multiplication

// Button states
bool lastSelectState = HIGH;
bool lastDivideState = HIGH;
bool lastMultiplyState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; // Debounce time in ms

// Division/groove values
// Values < 0 represent groove mode (e.g., -1 = groove active)
// Values > 0 represent division (e.g., 2 = 1/2 speed)
// Value of 1 is normal speed
int clockRatios[] = {1, 1, 1, 1, 1, 1};
int selectedOutput = 0; // Currently selected output (0-5)

// Count pulses for each output
int pulseCounts[] = {0, 0, 0, 0, 0, 0};
// Output state trackers
bool outputStates[] = {false, false, false, false, false, false};

unsigned long lastPulseTime = 0;
unsigned long currentTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long pulseStartTime[6] = {0}; // Track when each output was turned on
float bpm = 120; // Start with default value
bool showingBpm = false;
float lastValidBpm = 0;
unsigned long bpmStableTime = 0;
bool lastSyncState = false;
unsigned long lastPulseTimeout = 2000;
unsigned long lastSubdivisionTime[6] = {0}; // Track timing for each output
unsigned long lastStableBpm = 0; // Track when we last had a stable BPM
bool bpmDisplayStable = false; // Flag to indicate if BPM is stable

// BPM averaging
#define BPM_SAMPLES 3
unsigned long pulseTimes[BPM_SAMPLES];
int pulseIndex = 0;

void setup() {
  // Set all output pins
  for (int i = 0; i < 6; i++) {
    pinMode(syncOutPins[i], OUTPUT);
    digitalWrite(syncOutPins[i], LOW);
  }
  
  // Set button pins as inputs with pull-up resistors
  pinMode(selectButton, INPUT_PULLUP);
  pinMode(divideButton, INPUT_PULLUP);
  pinMode(multiplyButton, INPUT_PULLUP);
  
  pinMode(syncInPin, INPUT);
  pinMode(indicatorLED, OUTPUT);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 0);
  display.println("SYNC OUT");
  display.display();
  delay(1000);
  
  // Initialize pulse time array
  for (int i = 0; i < BPM_SAMPLES; i++) {
    pulseTimes[i] = 0;
  }
}

float calculateBPM() {
  // Calculate average duration between pulses
  unsigned long totalDuration = 0;
  int validSamples = 0;
  
  // Return early if we don't have valid samples
  if (pulseTimes[0] == 0) {
    return lastValidBpm > 0 ? lastValidBpm : 0;
  }
  
  for (int i = 0; i < BPM_SAMPLES-1; i++) {
    if (pulseTimes[i] > 0 && pulseTimes[i+1] > 0) {
      totalDuration += pulseTimes[i+1] - pulseTimes[i];
      validSamples++;
    }
  }
  
  if (validSamples > 0) {
    float avgDuration = (float)totalDuration / validSamples;
    return 60000.0 / avgDuration;
  }
  
  return 0;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0); 
  display.println("SYNC OUT");
  
  // Show BPM
  display.setTextSize(2);
  display.setCursor(0, 12); 
  
  // Only show a BPM if we've had pulses in the last 3 seconds
  if (currentTime - lastPulseTime < 3000) {
    // Only update lastValidBpm if bpm is not zero
    if (int(lastValidBpm) > 0) {
      display.print(int(lastValidBpm));
     }
 }
  else {
    display.print("---");
  }
  display.println(" BPM");
  
  // Add more space between BPM and clock divisions
  display.setCursor(0, 34);
  
  // Show clock divisions/groove
  display.setTextSize(1); // Back to size 1 (default size)
  
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < 6; i++) {
    display.print(i+1);
    display.print(":");
    
    if (clockRatios[i] < 0) {
      display.print("G");
      switch(grooveType[i]) {
       case GROOVE_SWING_8TH: display.print("S"); break;  // GS = Groove Swing
        case GROOVE_SHUFFLE: display.print("F"); break;    // GF = Groove Shuffle
        case GROOVE_HUMANIZE: display.print("H"); break;   // GH = Groove Humanize
      }
      display.print(grooveAmount[i]);
      display.print("%");
    } 
    else if (clockRatios[i] == 1) {
      display.print("1:1");
    } 
    else {
      display.print("1/");
      display.print(clockRatios[i]);
    }
    
    if (i == selectedOutput) {
      display.print("*"); // Mark selected output
    }
    
    display.print(" ");
    
    // New line every 3 outputs
    if (i == 2) {
      display.setCursor(0, 46);
    }
  }
  
  display.display();
}

void handleButtons() {
  // Read button states
  bool selectState = digitalRead(selectButton);
  bool divideState = digitalRead(divideButton);
  bool multiplyState = digitalRead(multiplyButton);
  
  // Handle select button (with debounce)
  if (selectState != lastSelectState && millis() - lastDebounceTime > debounceDelay) {
    lastDebounceTime = millis();
    if (selectState == LOW) { // Button pressed
      selectedOutput = (selectedOutput + 1) % 6;
      updateDisplay();
    }
    lastSelectState = selectState;
  }
  
  // Handle divide button (with debounce)
  if (divideState != lastDivideState && millis() - lastDebounceTime > debounceDelay) {
    lastDebounceTime = millis();
    if (divideState == LOW) { // Button pressed
      // Increase division (or decrease multiplication)
      if (clockRatios[selectedOutput] < 0) {
        // Currently in groove mode
        // Go to normal
        clockRatios[selectedOutput] = 1;
        grooveAmount[selectedOutput] = 0;
        grooveType[selectedOutput] = GROOVE_STRAIGHT;
      } else if (clockRatios[selectedOutput] == 1) {
        // Currently normal, switch to division
        clockRatios[selectedOutput] = 2; // Start at 1/2 speed
      } else {
        // Already in division mode
        clockRatios[selectedOutput] *= 2; // Double division (slower)
        if (clockRatios[selectedOutput] > 64) clockRatios[selectedOutput] = 64; // Max division 1/64
      }
      updateDisplay();
    }
    lastDivideState = divideState;
  }
  
  // Handle multiply button (with debounce)
  if (multiplyState != lastMultiplyState && millis() - lastDebounceTime > debounceDelay) {
    lastDebounceTime = millis();
    if (multiplyState == LOW) { // Button pressed
      // Increase multiplication (or decrease division)
      if (clockRatios[selectedOutput] > 1) {
        // Currently in division mode
        // If we're at 1/2, go straight to normal
        if (clockRatios[selectedOutput] == 2) {
          clockRatios[selectedOutput] = 1;
          grooveType[selectedOutput] = GROOVE_STRAIGHT;
        } else {
          clockRatios[selectedOutput] /= 2; // Move closer to 1 (faster)
        }
      } else if (clockRatios[selectedOutput] == 1) {
        // Currently normal, switch to groove mode
        clockRatios[selectedOutput] = -1; // Negative values = groove
        grooveType[selectedOutput] = GROOVE_SWING_8TH;
        grooveAmount[selectedOutput] = 50; // Start with 50% swing
      } else {
        // Already in groove mode, cycle through patterns
        // Skip GROOVE_STRAIGHT (0) in the cycle to avoid those "do nothing" modes
        if (grooveType[selectedOutput] == GROOVE_SWING_8TH) {
          grooveType[selectedOutput] = GROOVE_SHUFFLE;
        } else if (grooveType[selectedOutput] == GROOVE_SHUFFLE) {
          grooveType[selectedOutput] = GROOVE_HUMANIZE;
        } else { // GROOVE_HUMANIZE or any other
          // After cycle completes, return to SWING but with increased intensity
          grooveType[selectedOutput] = GROOVE_SWING_8TH;
          grooveAmount[selectedOutput] = min(grooveAmount[selectedOutput] + 25, 100);
          if (grooveAmount[selectedOutput] > 75) {
            clockRatios[selectedOutput] = 1; // Back to normal after max groove
          }
        }
      }
      updateDisplay();
    }
    lastMultiplyState = multiplyState;
  }
}

void handleGroove() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < 6; i++) {
    // Only process if in groove mode
    if (clockRatios[i] < 0) {
      // Skip if no recent pulses or BPM too low
      if (currentTime - lastPulseTime > 2000 || bpm < 30) continue;
      
      // Get beat duration
      unsigned long beatDuration = 60000 / bpm;
      
      // Only process for outputs not already high
      if (!outputStates[i]) {
        unsigned long timeSinceMainPulse = currentTime - lastSubdivisionTime[i];
         
        // Apply groove based on type
        switch (grooveType[i]) {
          case GROOVE_SWING_8TH:
            {
         // Calculate swing timing (move off-beat later based on amount)     
              // More dramatic swing effect: up to 66% of beat duration at max intensity
              unsigned long swingTime = beatDuration/2 + 
                                      (beatDuration/3 * grooveAmount[i]/100);              
               
            // Use a wider detection window (like in humanize mode)
              if (!outputStates[i] && timeSinceMainPulse >= (swingTime - 20) && 
                  timeSinceMainPulse <= (swingTime + 20)) {    // No randomization in swing mode - always consistent timing
                outputStates[i] = true;
                digitalWrite(syncOutPins[i], HIGH);
                pulseStartTime[i] = currentTime;       
              }
            }
            break;
            
          case GROOVE_SHUFFLE:
            // Triplet-based shuffle - create pulses at 1/3 and 2/3 points
            {
           // New approach: use 2-against-3 polyrhythm for shuffle feel
              // Only fire pulses on the downbeat and one weighted offset
               
              // Effective time since the last main pulse   
              unsigned long timeSinceMainPulse = currentTime - lastSubdivisionTime[i];
               
              // Ensure we're not past the next beat
              if (timeSinceMainPulse >= beatDuration) {
                break;
              }
               
              // Instead of triplets, use a weighted offset that feels like shuffle
              // At 100% intensity, this becomes close to a dotted 8th
              // Start with base offset (halfway through beat)
              unsigned long baseOffset = beatDuration / 2;
              
              // Adjust offset based on groove intensity (0-100%)
              // This creates a continuum between straight 8ths and dotted 8ths
              unsigned long adjustedOffset = baseOffset + 
                                           (baseOffset/3 * grooveAmount[i]/100);
                                           
              // Use a wide detection window for reliable triggering
              if (timeSinceMainPulse >= (adjustedOffset - 15) && 
                  timeSinceMainPulse <= (adjustedOffset + 15) && 
                  !outputStates[i]) {
                outputStates[i] = true;
                digitalWrite(syncOutPins[i], HIGH);
                pulseStartTime[i] = currentTime;
                
                 // Extra pulse width handled by handleClockDivision()
                // Avoid delays here to prevent timing issues
              }
             }                   
            break;
            
          case GROOVE_HUMANIZE:
            // Random timing variations around the off-beat
            if (beatDuration > 0) {
              // Create pulses at half-beat points with random variation
              unsigned long halfBeat = beatDuration / 2;  
              unsigned long timeSinceMainPulse = currentTime - lastPulseTime;
               // Much stronger humanize effect: up to 40% of beat at max intensity
               int variation = map(grooveAmount[i], 0, 100, 0, beatDuration*2/5);
              
              // Check if we're near the halfway point (with tolerance)
              if (!outputStates[i] && 
               timeSinceMainPulse >= (halfBeat - 20) && 
                   timeSinceMainPulse <= (halfBeat + 20)) {
                int randomOffset = random(-variation, variation);
                // Apply the random offset to make humanize more pronounced
                // Using a much shorter delay to avoid blocking
                delayMicroseconds(abs(randomOffset) * 100);
                outputStates[i] = true;
                digitalWrite(syncOutPins[i], HIGH);
                pulseStartTime[i] = currentTime;       
              }
            }
            break;
        }
      }
    }
  }
}

void handleClockDivision() {
  currentTime = millis();

  // For shuffle (GH) mode, force a pulse if we're in triplet zone
  if (bpm > 0) {
    unsigned long beatDuration = 60000 / bpm;
    unsigned long elapsedTime = currentTime - lastPulseTime;
    
    for (int i = 0; i < 6; i++) {
      if (clockRatios[i] < 0 && grooveType[i] == GROOVE_SHUFFLE && 
          grooveAmount[i] > 0 && !outputStates[i]) {
      }
    }
  }
  // Turn off outputs that have been high long enough
  for (int i = 0; i < 6; i++) {
  // Make pulse width longer for groove modes to help sequencer detection    
unsigned long pulseWidth;
    if (clockRatios[i] < 0) {
      // Use consistent pulse width for all groove types
      pulseWidth = minPulseWidth * 5;  // Very long pulse for all groove modes
    } else {
      pulseWidth = minPulseWidth;
    }
    
    if (outputStates[i] && currentTime - pulseStartTime[i] >= pulseWidth) {
      digitalWrite(syncOutPins[i], LOW);
      outputStates[i] = false;
     
    }
  }
  
  // Check if this is a new pulse
  int syncValue = analogRead(syncInPin);
  bool syncState = (syncValue >= threshold);
  
  if (syncState && !lastSyncState) { // Rising edge detected
    // Handle BPM calculation
    pulseTimes[pulseIndex] = currentTime;
    pulseIndex = (pulseIndex + 1) % BPM_SAMPLES;

    // Calculate BPM and store valid values
    float newBpm = calculateBPM();
    if (newBpm > 30) {  // Only accept reasonable BPM values
      lastValidBpm = newBpm;
    }
    lastPulseTime = currentTime; 
    bpmStableTime = currentTime;
    showingBpm = true; // Signal that we should show BPM
    // Turn on indicator LED
    digitalWrite(indicatorLED, HIGH);
    
    // Process all outputs
    for (int i = 0; i < 6; i++) {
      // Normal speed or groove-enabled channels
      if (clockRatios[i] <= 1) {
        pulseCounts[i]++; // Count all pulses for groove patterns
        lastSubdivisionTime[i] = currentTime; // Reset subdivision timer for groove patterns
        // Output on main beat for both normal and groove modes
        if (clockRatios[i] == 1) {
          digitalWrite(syncOutPins[i], HIGH);
          outputStates[i] = true;
          pulseStartTime[i] = currentTime;
        }
      }
      // Division
      else if (clockRatios[i] > 1) {
        // Only output on specific divisions
        if (pulseCounts[i] % clockRatios[i] == 0) {
          digitalWrite(syncOutPins[i], HIGH);
          outputStates[i] = true;
          pulseStartTime[i] = currentTime;
        }
        pulseCounts[i]++;
      }
    }
  } else if (!syncState && lastSyncState) { // Falling edge detected
    // Turn off indicator LED
    digitalWrite(indicatorLED, LOW);
  }
  
  lastSyncState = syncState;
  
  // Check for timeout on sync pulses
  if (bpm > 0 && currentTime - lastPulseTime > lastPulseTimeout) {
    // If no pulses for a while, reset the BPM display 
  }
  
  // Occasionally update the display
  if (currentTime - lastDisplayUpdate >= 500) {
    lastDisplayUpdate = currentTime;
    
    // Update display with current BPM
    if (currentTime - bpmStableTime > 5000) {
      bpmDisplayStable = false;
    }
    
    updateDisplay();
  }
}

void loop() {
  // Process button presses
  handleButtons();
 
   // Process any groove patterns
   handleGroove();
  
  // Process incoming clock signal
  handleClockDivision();
}