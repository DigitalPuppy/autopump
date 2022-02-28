/*
 * AutoPump: automatically cycle inflation and deflation of an air pump, controlled by one button!
 */

/* Constants */
const int BUTTON_PORT = 4;
const int RELAY_MOTOR_PORT = 2;
const int RELAY_VALVE_PORT = 3;

/* state */
unsigned long pumpTargetDuration = 0;
unsigned long pumpStartTime = 0;
unsigned long deflateStartTime = 0;
unsigned long waitStartTime = 0;

#define DEBUG 1

enum ButtonState
{
   BTN_PROCESSING,
   BTN_LONG_CLICK,
   BTN_SHORT_CLICK,
   BTN_SHORT_DBLCLICK
};

struct Button
{
  ButtonState state;
  bool rawPressed;
  unsigned long pressedSince;
  unsigned long lastRelease;
};


enum State
{
   STATE_IDLE = 0,
   STATE_INIT_PUMP,
   STATE_INIT_WAIT,
   STATE_DEFLATE,
   STATE_INFLATE,
   STATE_WAIT,
   STATE_NULL,
};

typedef void(*Func)(void);
typedef bool(*Check)(void);

struct Transition
{
  State target;    /* where to go...*/
  Check condition; /* ...if this function returns true*/
};

struct StateDefinition
{
  State state; /* redundant, index in 'states' is used*/
  Func onEnter; /* called once when entering state */
  Func onLoop; /* called repeatedly when in state*/
  Func onLeave; /* called once when leaving state*/
  Transition transitions[5]; /* where we can go from there*/
};

struct StateMachineDefinition
{
  Func preLoop; /* called each step once */
  StateDefinition states[STATE_NULL];
  Transition always[5]; /* transitions from every state */
};

struct StateMachine
{
  State currentState;
};

Button button = Button { BTN_PROCESSING, false, 0, 0 };

/*
 * Process button with debouncing given current raw state
 * Will set the state variable other than BTN_PROCESSING when raw state releases only.
 */
void processButton(Button& button, bool current)
{
  unsigned long now = millis();
  button.state = BTN_PROCESSING;
  if (now - button.pressedSince < 80 || now - button.lastRelease < 80)
      return; // debounce, no read, do nothing
  bool prev = button.rawPressed;
  button.rawPressed = current;
  
  if (prev && !current)
  {
    // release
    unsigned long duration = now - button.pressedSince;
    if (duration < 400)
    {
      if (now - button.lastRelease < 1000)
          button.state = BTN_SHORT_DBLCLICK;
      else
          button.state = BTN_SHORT_CLICK;
    }
    else
        button.state = BTN_LONG_CLICK;
    button.lastRelease = now;
  }
  if (current && !prev)
  {
    button.pressedSince = now;
  }
#if DEBUG
  if (prev != current)
  {
    Serial.print("btn ");
    Serial.print(prev);
    Serial.print(" ");
    Serial.print(current);
    Serial.print(button.state);
    Serial.print("\n");
  }
#endif
}

void setup() {
  pinMode(RELAY_MOTOR_PORT, OUTPUT);
  pinMode(RELAY_VALVE_PORT, OUTPUT);
  pinMode(BUTTON_PORT, INPUT_PULLUP);
  // put your setup code here, to run once:
  digitalWrite(RELAY_MOTOR_PORT, HIGH);
  digitalWrite(RELAY_VALVE_PORT, HIGH);
#if DEBUG
  Serial.begin(9600);
  Serial.print("Running\n");
#endif
}

/* basic actions and trigger conditions of the state machine */

bool check_pressing()
{
  return button.rawPressed;
}

bool check_long_release()
{
  return button.state == BTN_LONG_CLICK;
}

bool check_short_release()
{
  return button.state == BTN_SHORT_CLICK || button.state == BTN_SHORT_DBLCLICK;
}

bool check_deflate_end()
{
  return millis() - deflateStartTime > 2000;
}

bool check_pump_end()
{
  /* we stop when timer elapses and button is not being pressed*/ 
  return (!button.rawPressed && millis()-pumpStartTime > pumpTargetDuration) || button.state == BTN_LONG_CLICK;
}

bool check_wait_end()
{
  return millis() - waitStartTime > 3000 || button.state == BTN_SHORT_CLICK;
}

bool check_double_click()
{
  return button.state == BTN_SHORT_DBLCLICK;
}

void action_stop_all()
{
  digitalWrite(RELAY_MOTOR_PORT, HIGH);
  digitalWrite(RELAY_VALVE_PORT, HIGH);
}

void action_start_pumping()
{
  digitalWrite(RELAY_VALVE_PORT, LOW);
  delay(300); // current spike breaks my power supply and reboots the arduino otherwise
  digitalWrite(RELAY_MOTOR_PORT, LOW);
  pumpStartTime = millis();
}

void action_stop_pumping()
{
  digitalWrite(RELAY_MOTOR_PORT, HIGH);
  digitalWrite(RELAY_VALVE_PORT, LOW);
  pumpTargetDuration = millis() - pumpStartTime;
  waitStartTime = millis();
}

void action_process_button()
{
  processButton(button, digitalRead(BUTTON_PORT)==LOW);
}

void action_deflate()
{
  deflateStartTime = millis();
  digitalWrite(RELAY_MOTOR_PORT, HIGH);
  digitalWrite(RELAY_VALVE_PORT, HIGH);
}

/*
 * Perform a step of the state machine.
 * calls preloop, then loop, then check 'always' transitions, then state transitions.
 */
void stepStateMachine(StateMachine& sm, StateMachineDefinition& def)
{
  if (def.preLoop)
    def.preLoop();

  StateDefinition& st = def.states[sm.currentState];
  if (st.onLoop)
    st.onLoop();
  // always transitions
  for (int i = 0; i < 5 ; i++)
  {
    auto& tr = def.always[i];
    if (tr.target == STATE_NULL)
        break;
    bool trig = tr.condition();
    if (trig)
    {
#if DEBUG
      Serial.print("state change");
      Serial.print(st.state);
      Serial.print(" ");
      Serial.print(tr.target);
      Serial.print("\n");
#endif
      if (st.onLeave)
        st.onLeave();
      sm.currentState = tr.target;
      auto& ns = def.states[sm.currentState];
      if (ns.onEnter)
          ns.onEnter();
      return;
    }
  }
  // state transitions
  for (int i = 0; i < 5 ; i++)
  {
    auto& tr = st.transitions[i];
    if (tr.target == STATE_NULL)
        break;
    bool trig = tr.condition();
    if (trig)
    {
#if DEBUG
      Serial.print("state change");
      Serial.print(st.state);
      Serial.print(" ");
      Serial.print(tr.target);
      Serial.print("\n");
#endif
      if (st.onLeave)
        st.onLeave();
      sm.currentState = tr.target;
      auto& ns = def.states[sm.currentState];
      if (ns.onEnter)
          ns.onEnter();
      return;
    }
  }
}

Transition TR_END = {STATE_NULL, NULL};

StateMachineDefinition pump = StateMachineDefinition{
  action_process_button,
  {
    {STATE_IDLE, action_stop_all, NULL, NULL,                                                  /* initial state, wait for button */
      {{STATE_INIT_PUMP, check_pressing}, TR_END}   },
    {STATE_INIT_PUMP, action_start_pumping, NULL, action_stop_pumping,                         /* while button held, inflate */
      {{STATE_INIT_WAIT, check_long_release}, {STATE_IDLE, check_short_release}, TR_END}    },
    {STATE_INIT_WAIT, NULL, NULL, NULL,                                                        /* wait for button to start cycling*/
      {{STATE_DEFLATE, check_short_release}, TR_END}  },
    {STATE_DEFLATE, action_deflate, NULL, NULL,                                                /* deflate for a fixed duration */
      {{STATE_INFLATE, check_deflate_end}, TR_END}    },
    {STATE_INFLATE, action_start_pumping, NULL, NULL,                                          /* inflate again, watch for button */
      {{STATE_WAIT, check_pump_end}, {STATE_DEFLATE, check_short_release}, TR_END}          },
    {STATE_WAIT, action_stop_pumping, NULL, NULL,                                              /* wait a bit at max inflation */
      {{STATE_DEFLATE, check_wait_end}, TR_END}       },
  },
  {
    {STATE_IDLE, check_double_click}, /* emergency stop from anywhere */
    TR_END
  }
};

StateMachine sm = StateMachine {STATE_IDLE};

void loop() {
  // put your main code here, to run repeatedly:
  stepStateMachine(sm, pump);
}
