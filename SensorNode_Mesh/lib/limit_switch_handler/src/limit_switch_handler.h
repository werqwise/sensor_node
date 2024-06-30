
#include "OneButton.h"

int get_limit_sw_state();
int setup_limit_switch();
void loop_limit_switch();
void LongPressStart(void *oneButton);
void LongPressStop(void *oneButton);

#define PIN_INPUT 34


int limit_sw_state = 0;
// Setup a new OneButton on pin PIN_INPUT
// The 2. parameter activeLOW is true, because external wiring sets the button to LOW when pressed.
OneButton button(PIN_INPUT, true);

// In case the momentary button puts the input to HIGH when pressed:
// The 2. parameter activeLOW is false when the external wiring sets the button to HIGH when pressed.
// The 3. parameter can be used to disable the PullUp .
// OneButton button(PIN_INPUT, false, false);

// setup code here, to run once:
int setup_limit_switch()
{
    // pinMode(PIN_INPUT, INPUT_PULLUP);
    // link functions to be called on events.
    button.attachLongPressStart(LongPressStart, &button);
    // button.attachDuringLongPress(DuringLongPress, &button);
    button.attachLongPressStop(LongPressStop, &button);

    button.setLongPressIntervalMs(1000);
    return 1;
} // setup

// main code here, to run repeatedly:
void loop_limit_switch()
{
    // keep watching the push button:
    button.tick();

} // loop

// this function will be called when the button started long pressed.
void LongPressStart(void *oneButton)
{
    // Serial.print(((OneButton *)oneButton)->getPressedMs());
    // Serial.println("\t - LongPressStart()");
    limit_sw_state = 1;
}

void LongPressStop(void *oneButton)
{
    //   Serial.print(((OneButton *)oneButton)->getPressedMs());
    //   Serial.println("\t - LongPressStop()\n");
    limit_sw_state = 0;
}

int get_limit_sw_state()
{
    // limit_sw_state=digitalRead(PIN_INPUT);
    return limit_sw_state;
}