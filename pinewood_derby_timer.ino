//
// pinewood_derby_time.ino
//
// timer for Pinewood Derby
//
#include <Arduino.h>


#if 0
// Timer states.
enum State {
  READY,   // not running and with a value 0. 
  RUNNING,     // running.
  TRIGGERED,   // sensor triggered, not running and with a value.
  STOPPED,     // stopped, not running and with a value.
};

#else
typedef int State;
#define READY 0
#define RUNNING 1
#define TRIGGERED 2
#define STOPPED 3
#endif

#define BAUDRATE 76800

#define N_SENSORS 4
int sensor_pins[N_SENSORS] = {A0, A1, A2, A3};
int sensor_thresholds[N_SENSORS] = {0, 0, 0, 0};

// We have as many timers as sensors.
#define N_TIMERS	N_SENSORS
bool timer_enables[N_TIMERS] = {false, false, false, false};
State global_timers_state = READY;
unsigned long global_start_millis;
unsigned long global_stop_millis;

State timer_states[N_TIMERS] = {READY, READY, READY, READY};
unsigned long timer_stop_millis[N_TIMERS] = {0, 0, 0, 0};

// Runtime statistics
unsigned long last_reset_millis;
unsigned long num_loops;
unsigned long total_loop_millis;
unsigned long max_loop_millis;
unsigned long num_running_loops;
unsigned long total_running_loop_millis;
unsigned long max_running_loop_millis;
char max_loop_command = '?';
char max_running_loop_command = '?';
unsigned long num_commands;

void reset_internal();
void clear_timers_internal();

void setup() {
  Serial.begin(BAUDRATE);
  Serial.setTimeout(50);

  reset_internal();
  // We assume millis() does not return 0.
  if (millis() == 0) {
    delay(2);
  }
  
  clear_timers_internal();
  reset_internal();
}

void clear_timers_internal() {
  for (int i = 0; i < N_TIMERS; i++) {
    timer_states[i] = READY;
    timer_stop_millis[i] = 0;
  }
  global_timers_state = READY;
  global_start_millis = 0;
  global_stop_millis = 0;
}

void clear_timers() {
  clear_timers_internal();
  Serial.println("Cok");
}

void reset_internal() {
  last_reset_millis = 0; 
  num_loops = 0;
  total_loop_millis = 0;
  max_loop_millis = 0;
  max_loop_command = '?';
  num_running_loops = 0;
  total_running_loop_millis = 0;
  max_running_loop_millis = 0;
  max_running_loop_command = '?';

  // Resets timers.
  for (int i = 0; i < N_SENSORS; i++) {
    timer_enables[i] = false;
  }
  clear_timers_internal();
}

void reset() {
  reset_internal();
  Serial.println("Rok");
}

void read_active_sensors() {
  for (int i = 0; i < N_SENSORS; i++) {
    if (timer_states[i] == RUNNING) {
      int value = analogRead(sensor_pins[i]);
      // LDR sensors trigger when value < threshold.
      // Optical logic sensor trigger when value > threshhold.
      if (value < sensor_thresholds[i]) {
        timer_states[i] = TRIGGERED;
        timer_stop_millis[i] = millis() - global_start_millis;
      }
    }
  }
}

#if 0
// Template does not work with some verision of Arduino IDE.
template<typename T>
void print_array(const T array[], int n) {
  for (int i = 0; i < n; i++) {
    Serial.print(array[i], DEC);
    if (i < n-1) {
      Serial.write(",");
    }
  }
}
#endif

void print_int_array(const int array[], int n) {
  for (int i = 0; i < n; i++) {
    Serial.print(array[i], DEC);
    if (i < n-1) {
      Serial.write(",");
    }
  }
}

void print_unsigned_long_array(const unsigned long array[], int n) {
  for (int i = 0; i < n; i++) {
    Serial.print(array[i], DEC);
    if (i < n-1) {
      Serial.write(",");
    }
  }
}

void get_sensor_values() {
  int values[N_SENSORS];
  for (int i = 0; i< N_SENSORS; i++) {
    values[i] = analogRead(sensor_pins[i]);
  }
  Serial.write("A");
  print_int_array(values, N_SENSORS);
  Serial.println("");
}

void start_timers() {
  if (global_timers_state == STOPPED) {
    Serial.println("Gfailed");
    return;
  }

  if (global_timers_state == READY) {
    global_start_millis = millis();
    global_timers_state = RUNNING;
    for (int i = 0; i < N_SENSORS; i++) {
      if (timer_enables[i]) {
        timer_states[i] = RUNNING;
      }
    }
  }
  Serial.println("Gok");
}

void stop_timers()
{
  if (global_timers_state == READY) {
    Serial.println("Gfailed");
    return;
  }

  if (global_timers_state != STOPPED) {
    global_timers_state = STOPPED;
    global_stop_millis = millis();
    for (int i = 0; i < N_TIMERS; i++) {
      if (timer_states[i] == RUNNING) {
        timer_stop_millis[i] =
          global_stop_millis - global_start_millis;
        timer_states[i] = STOPPED;
      }
    }
  }
  Serial.println("Sok");
}

void get_timer_enables() {
  char buffer[N_TIMERS+2];
  buffer[0] = 'E';
  for (int i = 0; i < N_TIMERS; i++) {
    buffer[i+1] = timer_enables[i] ? '1' : '0';
  }
  buffer[N_SENSORS+1] = '\0';
  Serial.println(buffer);
}

void set_timer_enables() {
  char buffer[256];
  size_t len = Serial.readBytesUntil('\n', buffer, sizeof(buffer));
  buffer[len] ='\0';

  if (len < N_TIMERS) {
    Serial.println("Ffailed:too short");
    return;
  }

  bool enables[N_TIMERS];
  int n_enables = 0;
  for (int i = 0; i < N_TIMERS; i++) {
    char c = buffer[i];
    if (c == '0') {
      enables[n_enables] = false;
    } else if (c == '1') {
      enables[n_enables] = true;
    } else {
      break;
    } 
    n_enables++;
  }

  if (n_enables == N_TIMERS) {
    for (int i = 0; i < N_TIMERS; i++) {
      timer_enables[i] = enables[i];
    }
    Serial.println("Fok");
  } else {
    Serial.println("Ffailed:bad input");
  }
}

void get_sensor_thresholds() {
  Serial.print("T");
  print_int_array(sensor_thresholds, N_SENSORS);
  Serial.println("");
}

void set_sensor_thresholds() {
  long thresholds[N_SENSORS];
  int i;
  for (i = 0; i < N_SENSORS; i++) {
    // get comma.
    if (i > 0) {
      char c = Serial.read();
      if (c != ',') {
        break;
      } 
    }
    thresholds[i] = Serial.parseInt();
  }

  // read rest of line.
  char buffer[256];
  Serial.readBytesUntil('\n', buffer, sizeof(buffer));
  
  if (i < N_SENSORS) {
    Serial.println("Ufailed:bad input");
    return;
  }

  for (int i = 0; i < N_SENSORS; i++) {
    sensor_thresholds[i] = thresholds[i];
  }
  Serial.println("Uok");
}

void get_timer_values() {
  unsigned long values[N_TIMERS];
  unsigned long now = millis();
  for (int i = 0; i < N_TIMERS; i++) {
    switch (timer_states[i]) {
    case RUNNING:
      values[i] = now - global_start_millis;
      break;
    case STOPPED:
    case TRIGGERED:
      values[i] = timer_stop_millis[i];
      break;
    default:
      values[i] = 0;
    }
  }

  Serial.print("V");
  print_unsigned_long_array(values, N_TIMERS);
  Serial.println("");
}

// Reports timer states.
void get_timer_states() {
  char buffer[N_TIMERS+2];
  buffer[0] = 'Q';
  for (int i = 0; i < N_TIMERS; i++) {
    char c = '?';
    if (timer_enables[i]) {
      if (global_timers_state == READY) {
        c = 'C';
      } else {
        switch (timer_states[i]) {
        case RUNNING:
          c = 'G';
          break;
        case TRIGGERED:
          c = 'T';
          break;
        case STOPPED:
          c = 'S';
          break;
        default:
          break;
        }
      }
    } else {
      c = 'D';
    }
    buffer[i+1] = c;
  }
  buffer[N_TIMERS+1] = '\0';
  Serial.println(buffer);
}

// Reports runtime statistics of the timer.
void get_infos() {
 long now_millis = millis();
 Serial.print("Inum_loops=");
 Serial.print(num_loops);

 long millis_since_reset = now_millis - last_reset_millis;
 float period = float(millis_since_reset) / num_loops;
 Serial.print(",period=");
 Serial.print(period);

 float average_loop_millis = num_loops > 0 ?
   float(total_loop_millis) / num_loops : 0;
 Serial.print(",average_loop_millis=");
 Serial.print(average_loop_millis);

 Serial.print(",max_loop_millis=");
 Serial.print(max_loop_millis);

 Serial.print(",max_loop_command=");
 Serial.print(max_loop_command);

 float average_running_loop_millis = 
   num_running_loops > 0 ?
   float(total_running_loop_millis) / num_running_loops : 0;
 Serial.print(",average_running_loop_millis=");
 Serial.print(average_running_loop_millis);

 Serial.print(",max_running_loop_millis=");
 Serial.print(max_running_loop_millis);

 Serial.print(",max_running_loop_command=");
 Serial.print(max_running_loop_command);
 
 long self_millis = millis() - now_millis;
 Serial.print(",self_millis=");
 Serial.print(self_millis);

 Serial.print(",num_commands=");
 Serial.print(num_commands);

 Serial.println("");
}

void loop() {
  long loop_start_millis = millis();
  char cmd = '?';

  if (Serial.available() > 0) {
    cmd = Serial.read();
    if (cmd > 0) {
      num_commands++;
      switch(cmd) {
      case 'a':
        get_sensor_values();
        break;
      case 'c':
        clear_timers();
        break;
      case 'e':
        get_timer_enables();
        break;
      case 'f':
        set_timer_enables();
        break;
      case 'g':
        start_timers();
        break;
      case 'h':
        Serial.println("Hello");
        break;
      case 'i':
        get_infos();
        break;
      case 'q':
        get_timer_states();
        break;
      case 'r':
        reset();
        break;
      case 's':
        stop_timers();
        break;
      case 't':
        get_sensor_thresholds();
        break;
      case 'u':
        set_sensor_thresholds();
        break;
      case 'v':
        get_timer_values();
        break;
      default:
        Serial.println("?failed");
      }
    }
  }
  Serial.flush();

  if (global_timers_state == RUNNING) {
    read_active_sensors();
  } else {
    delay(1);
  }

  // Update loop statistics
  unsigned long loop_millis = millis() - loop_start_millis;
  total_loop_millis += loop_millis;
  if (loop_millis > max_loop_millis) {
    max_loop_millis = loop_millis;
    max_loop_command = cmd;
  }
  num_loops++;
  if (global_timers_state == RUNNING) {
    total_running_loop_millis += loop_millis;
    if (loop_millis > max_running_loop_millis) {
      max_running_loop_millis = loop_millis;
       max_running_loop_command = cmd;
    }
    num_running_loops++;
  }
}
