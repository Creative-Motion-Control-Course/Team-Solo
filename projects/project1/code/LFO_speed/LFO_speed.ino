#define module_driver
#include "stepdance.hpp"

// -- Output Ports --
OutputPort output_a;
OutputPort output_b;
OutputPort output_c;

// -- Motion Channels --
Channel channel_a;
Channel channel_b;
Channel channel_z;

// -- Kinematics --
KinematicsCoreXY axidraw_kinematics;

// -- Inputs --
AnalogInput lfo_input;  // A1 - Mother-32 LFO TRI
Button button_d1;       // D1 - stop/start toggle
Button button_d2;       // D2 - line break

// -- RPC --
RPC rpc;

// -- Generators --
WaveGenerator1D wave_gen;
VelocityGenerator x_velocity_gen;

// -- State Variables --
float y_offset = 0.0;
float y_direction = 1.0;
float y_step = 0.5;
float prev_signal = 0.0;
bool is_running = false;  // start stopped

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

  // D1: stop/start toggle
  button_d1.begin(IO_D1, INPUT_PULLDOWN);
  button_d1.set_mode(BUTTON_MODE_STANDARD);
  button_d1.set_callback_on_press(&toggle_start_stop);

  // D2: line break
  button_d2.begin(IO_D2, INPUT_PULLDOWN);
  button_d2.set_mode(BUTTON_MODE_STANDARD);
  button_d2.set_callback_on_press(&next_line);

  // -- WaveGenerator1D --
  wave_gen.setNoInput();
  wave_gen.frequency = 0.0;
  wave_gen.amplitude = 0.0;
  wave_gen.output.map(&axidraw_kinematics.input_y);
  wave_gen.begin();

  // -- X VelocityGenerator --
  x_velocity_gen.begin();
  x_velocity_gen.speed_units_per_sec = 0.0;  // start stopped
  x_velocity_gen.output.map(&axidraw_kinematics.input_x);

  rpc.begin();
  rpc.enroll("hello", hello_serial);
  rpc.enroll("reset_origin", reset_origin);
  rpc.enroll("motors_enable", motors_enable);
  rpc.enroll("motors_disable", motors_disable);

  dance_start();
}

LoopDelay lfo_delay;
LoopDelay report_delay;

void loop() {
  lfo_delay.periodic_call(&lfo_update, 50);
  report_delay.periodic_call(&report_overhead, 500);
  dance_loop();
}

void lfo_update() {
  if (!is_running) return;  // stopped → skip

  float raw = lfo_input.read();
  float signal = raw / 1023.0;
  signal = constrain(signal, 0.0, 1.0);

  // amplitude: LFO voltage → wave height
  wave_gen.amplitude = signal * 10.0;

  // frequency: LFO rate of change → wave density
  float rate = abs(signal - prev_signal);
  wave_gen.frequency = rate * 10.0;
  prev_signal = signal;
}

void toggle_start_stop() {
  is_running = !is_running;

  if (is_running) {
    x_velocity_gen.speed_units_per_sec = 2.0;  // start moving
    Serial.println("START");
  } else {
    x_velocity_gen.speed_units_per_sec = 0.0;  // stop moving
    wave_gen.amplitude = 0.0;
    wave_gen.frequency = 0.0;
    Serial.println("STOP");
  }
}

void next_line() {
  y_offset += y_step * y_direction;
  if (y_offset > 150.0 || y_offset < 0.0) {
    y_direction *= -1.0;
    y_offset += y_step * y_direction;
  }
  axidraw_kinematics.input_x.reset_deep(0);
}

void motors_enable()  { enable_drivers(); }
void motors_disable() { disable_drivers(); }

void reset_origin() {
  axidraw_kinematics.input_x.reset_deep(0);
  axidraw_kinematics.input_y.reset_deep(0);
  y_offset = 0.0;
  y_direction = 1.0;
  prev_signal = 0.0;
  is_running = false;
  x_velocity_gen.speed_units_per_sec = 0.0;
  wave_gen.amplitude = 0.0;
  wave_gen.frequency = 0.0;
}

void hello_serial() {
  Serial.print("hello!");
}

void report_overhead() {
  float raw = lfo_input.read();
  float voltage = raw * (3.3 / 1023.0);
  Serial.print(is_running ? "RUNNING" : "STOPPED");
  Serial.print(" | LFO: ");       Serial.print(raw);
  Serial.print(" | Voltage: ");   Serial.print(voltage);   Serial.print(" V");
  Serial.print(" | Amplitude: "); Serial.print(wave_gen.amplitude);
  Serial.print(" | Frequency: "); Serial.print(wave_gen.frequency);
  Serial.print(" | Y offset: ");  Serial.println(y_offset);
}