// This program closed-loop controls electric vehicle current.
// Its goal is to maintain a minimum nonzero curtailment on a solar panel power supply.
// If the curtailment drops to zero, the program will promptly reduce allowable EV current.
// If the curtailment is nonzero, the program will gradually request an increase in EV current.

// Requests 0A current when curtailment is not detected.
// Shuts down inverter if curtailment does not occur for a very long time (~1 minute)

// Constants:
const int PWMPin = 6;                             // Pin for PWM output.
const int curtailmentPin = 3;                     // Pin that goes low when curtailment is taking place.
const int inverterPin = 15;                       // Pin  that controls the inverter power on/off relay.
const int maxDuty = 82;                           // PWM duty cycle (0-255 scale) corresponding to a maximum of 18 amps.
const int minDuty = 25;                           // PWM duty cycle (0-255 scale) corresponding to a minimum of 6 amps.
const int dutyStepSize = 2;                       // PWM duty cycle (0-255 scale) step size when going from mone current to the next, approximatley equal to 1 amp.
const unsigned long minUpdateTime = 250;          // Minimum update interval for updating PWM or data message.
const unsigned long stepUpDelay = 5000;           // Time delay before incrementing current request.
const unsigned long critCurtTime = 500;           // Amount of time since last curtailment event before current step-down is performed.
const unsigned long inverterShutdownTime = 60000; // Amount of time since last curtailment event before inverter is shut down (at minimum duty cycle)
const int stepDownAggression = 1;                 // Number of current command steps to take down in each current decrease event.
const int stepUpAggression = 1;                   // Number of current command steps to take down in each current increase event.

// Variables:
bool curtailmentOccurred = 0;             // Flag to indicate that curtailment has just occurred.
unsigned long lastCurtailmentMillis = 0;  // Timer variable for last-known curtailment event.
unsigned long lastUpdateMillis = 0;       // Timer variable for last PWM update event.
unsigned long lastStepUpMillis = 0;       // Timer variable for last PWM step-up event.
int currentDuty = 0;                      // Current duty cycle being commanded to the output.

// ISR for curtailmnet detection:
void updateCurtailmentISR() {
  // Update curtailment flag:
  curtailmentOccurred = 1;
}

void setup() {  
  // Enable curtailment interrupts:
  attachInterrupt(digitalPinToInterrupt(curtailmentPin), updateCurtailmentISR, FALLING);

  // Start inverter:
  pinMode(inverterPin, OUTPUT);
  digitalWrite(inverterPin, HIGH);

}

void loop() {
  // Check if curtailment has just occurred. If so, reset the timer:
  if(curtailmentOccurred)
  {
    // Reset curtailment flag:
    curtailmentOccurred = 0;

    // Update timer:
    lastCurtailmentMillis = millis();
  }

  // Check if curtailment is actively occurring. If so, reset the timer:
  if(!digitalRead(curtailmentPin))
  {
    // Update timer:
    lastCurtailmentMillis = millis();
  }

  // Check if sufficient time has passed to update PWM or send data message:
  if((millis() - lastUpdateMillis) > minUpdateTime)
  {
    // Check if a step-down in current needs to be performed:
    if((millis() - lastCurtailmentMillis) > critCurtTime)
    {

      // Perform a current step-down:
      currentDuty = currentDuty - (dutyStepSize*stepDownAggression);
      
    }
    else
    {      
      // Check if it is OK to perform a current step-up:
      if((millis() - lastStepUpMillis) > stepUpDelay)
      {
        // Re-enable inverter if it is shut down:
        //digitalWrite(inverterPin, HIGH);
        
        // Perform a current step-up:
        currentDuty = currentDuty + (dutyStepSize*stepUpAggression);

        // Reset step-up timer:
        lastStepUpMillis = millis();
        
      }
    }

    // Clamp current duty cycle between upper and lower bounds:
    if(currentDuty > maxDuty)
    {
      currentDuty = maxDuty;
    }
    if(currentDuty < minDuty)
    {
      currentDuty = minDuty;
    }

    // Apply the computed duty cycle to the CP pin driver.
    // The driver is inverting, so duty cycle must be inverted as well.
    analogWrite(PWMPin, 255-currentDuty);

    // Shut down inverter if curtailment has not occurred in critical time:
    if((millis() - lastCurtailmentMillis) > inverterShutdownTime)
    {
      digitalWrite(inverterPin, LOW);
    }
    
    // Reset update timer:
    lastUpdateMillis = millis();
  }

  
  
}
