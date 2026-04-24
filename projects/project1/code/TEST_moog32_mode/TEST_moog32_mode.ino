#define module_driver
#include "stepdance.hpp"

OutputPort output_a;
OutputPort output_b;
OutputPort output_c;

Channel channel_a;
Channel channel_b;
Channel channel_z;

KinematicsCoreXY axidraw_kinematics;

AnalogInput lfo_input;
Button button_d1;

RPC rpc;

PositionGenerator position_gen_x;
PositionGenerator position_gen_y;
TimeBasedInterpolator time_based_interpolator;

int mode = 0;
float t = 0.0;
float drift_state = 0.0;
float y_offset = 0.0;
float y_direction = 1.0;
float y_step = 0.5;

void setup() {
  Serial.begin(9600);

  output_a.begin(OUTPUT_A);
  output_b.begin(OUTPUT_B);
  output_c.begin(OUTPUT_C);
  enable_drivers();

  channel_a.begin(&output_a, SIGNAL_E);
  channel_b.begin(&output_b, SIGNAL_E);
  channel_a.set_ratio(25.4, 2032);
  channel_a.invert_output();
  channel_b.set_ratio(25.4, 2032);
  channel_b.invert_output();

  channel_z.begin(&output_c, SIGNAL_E);
  channel_z.set_ratio(1, 50);

  axidraw_kinematics.begin();
  axidraw_kinematics.output_a.map(&channel_a.input_target_position);
  axidraw_kinematics.output_b.map(&channel_b.input_target_position);

  lfo_input.begin(IO_A1);

  button_d1.begin(IO_D1, INPUT_PULLDOWN);
  button_d1.set_mode(BUTTON_MODE_STANDARD);
  button_d1.set_callback_on_press(&switch_mode);

  position_gen_x.output.map(&axidraw_kinematics.input_x);
  position_gen_x.begin();
  position_gen_y.output.map(&axidraw_kinematics.input_y);
  position_gen_y.begin();

  time_based_interpolator.begin();
  time_based_interpolator.output_x.map(&axidraw_kinematics.input_x);
  time_based_interpolator.output_y.map(&axidraw_kinematics.input_y);
  time_based_interpolator.output_z.map(&channel_z.input_target_position);

  rpc.begin();
  rpc.enroll("hello", hello_serial);
  rpc.enroll("reset_origin", reset_origin);
  rpc.enroll("motors_enable", motors_enable);
  rpc.enroll("motors_disable", motors_disable);

  dance_start();
}

LoopDelay draw_delay;
LoopDelay report_delay;

void loop() {
  draw_delay.periodic_call(&draw_step, 50);
  report_delay.periodic_call(&report_overhead, 500);
  dance_loop();
}

void next_line() {
  t = 0.0;
  drift_state = 0.0;
  y_offset += y_step * y_direction;

  if (y_offset > 150.0 || y_offset < 0.0) {
    y_direction *= -1.0;
    y_offset += y_step * y_direction;
  }

  position_gen_x.go(0, GLOBAL, 30.0);
  position_gen_y.go(y_offset, GLOBAL, 30.0);
}

void draw_step() {
  float raw = lfo_input.read();
  float signal = raw / 1023.0;
  signal = constrain(signal, 0.0, 1.0);

  float x = 0.0;
  float y = 0.0;

  if (mode == 0) {
    x = t * 2.0;
    y = sin(t) * signal * 10.0;
    if (x > 100.0) { next_line(); return; }
    y = constrain(y, -10.0, 10.0);
    position_gen_x.go(x, GLOBAL, 8.0);
    position_gen_y.go(y + y_offset, GLOBAL, 8.0);
    t += 0.1;

  } else if (mode == 1) {
    x = t * 2.0;
    float noise = ((float)random(-100, 100)) / 100.0;
    y = (sin(t) + signal * noise) * 10.0;
    if (x > 100.0) { next_line(); return; }
    y = constrain(y, -10.0, 10.0);
    position_gen_x.go(x, GLOBAL, 8.0);
    position_gen_y.go(y + y_offset, GLOBAL, 8.0);
    t += 0.1;

  } else if (mode == 2) {
    x = t * 2.0;
    drift_state += (signal - 0.5) * 0.3;
    drift_state = constrain(drift_state, -20.0, 20.0);
    y = drift_state;
    if (x > 100.0) { next_line(); return; }
    y = constrain(y, -10.0, 10.0);
    position_gen_x.go(x, GLOBAL, 8.0);
    position_gen_y.go(y + y_offset, GLOBAL, 8.0);
    t += 0.1;

  } else if (mode == 3) {
    float radius = signal * 30.0;
    x = 50.0 + cos(t) * radius;
    y = 75.0 + sin(t) * radius;
    x = constrain(x, 0.0, 100.0);
    y = constrain(y, 0.0, 150.0);
    position_gen_x.go(x, GLOBAL, 15.0);
    position_gen_y.go(y, GLOBAL, 15.0);
    t += 0.05;
  }
}

void switch_mode() {
  mode = (mode + 1) % 4;
  t = 0.0;
  drift_state = 0.0;
  y_offset = 0.0;
  y_direction = 1.0;
  Serial.print("Mode: ");
  Serial.println(mode);
}

void motors_enable()  { enable_drivers(); }
void motors_disable() { disable_drivers(); }

void reset_origin() {
  axidraw_kinematics.input_x.reset_deep(0);
  axidraw_kinematics.input_y.reset_deep(0);
  t = 0.0;
  drift_state = 0.0;
  y_offset = 0.0;
  y_direction = 1.0;
}

void hello_serial() {
  Serial.print("hello!");
}

void report_overhead() {
  float raw = lfo_input.read();
  float voltage = raw * (3.3 / 1023.0);
  Serial.print("Mode: "); Serial.print(mode);
  Serial.print(" | LFO: "); Serial.print(raw);
  Serial.print(" | Voltage: "); Serial.print(voltage); Serial.print(" V");
  Serial.print(" | Y offset: "); Serial.print(y_offset);
  Serial.print(" | X: "); Serial.print(axidraw_kinematics.input_x.read(ABSOLUTE));
  Serial.print(" | Y: "); Serial.println(axidraw_kinematics.input_y.read(ABSOLUTE));
}