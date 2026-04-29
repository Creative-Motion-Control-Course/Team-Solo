#define module_driver
#include "stepdance.hpp"

// -- Output Ports --
OutputPort output_a;  // AxiDraw Left Motor
OutputPort output_b;  // AxiDraw Right Motor
OutputPort output_c;  // Z-axis Servo Driver (Pen Up/Down)

// -- Motion Channels --
Channel channel_a;
Channel channel_b;
Channel channel_z;

// -- Kinematics --
KinematicsCoreXY axidraw_kinematics;
KinematicsPolarToCartesian polar_kinematics;

// -- Inputs --
AnalogInput a1_synth;        // A1 - Mother-32 LFO TRI signal
AnalogInput a2_radiusSpeed;  // A2 - rotation speed control
Button d1_startStopToggle;   // D1 - Start/Stop toggle
Button d2_resetButton;       // D2 - Reset button

// -- Generators --
VelocityGenerator velocity_gen;  // controls rotation angle

// -- RPC --
RPC rpc;

// -- State Variables --
float r_min = 3.0;
float r_max = 10.0;
float radius = r_min;
float r;
float smoothed_r = 3.0;  // smoothed radius value

void setup() {
  Serial.begin(9600);

  // -- Output Ports --
  output_a.begin(OUTPUT_A);
  output_b.begin(OUTPUT_B);
  output_c.begin(OUTPUT_C);
  enable_drivers();

  // -- Channels --
  channel_a.begin(&output_a, SIGNAL_E);
  channel_b.begin(&output_b, SIGNAL_E);
  channel_a.set_ratio(25.4, 2032);
  channel_a.invert_output();
  channel_b.set_ratio(25.4, 2032);
  channel_b.invert_output();
  channel_z.begin(&output_c, SIGNAL_E);
  channel_z.set_ratio(1, 50);

  // -- Polar to Cartesian --
  velocity_gen.begin();
  velocity_gen.output.map(&polar_kinematics.input_angle);

  polar_kinematics.output_x.map(&axidraw_kinematics.input_x);
  polar_kinematics.output_y.map(&axidraw_kinematics.input_y);
  polar_kinematics.begin();

  axidraw_kinematics.output_a.map(&channel_a.input_target_position);
  axidraw_kinematics.output_b.map(&channel_b.input_target_position);
  axidraw_kinematics.begin();

  // -- Analog Inputs --
  a1_synth.begin(IO_A1);

  // rotation speed: 0 ~ 2.0 rad/s
  a2_radiusSpeed.set_floor(0, 25);
  a2_radiusSpeed.set_ceiling(2.0, 1020);
  a2_radiusSpeed.map(&velocity_gen.speed_units_per_sec);
  a2_radiusSpeed.begin(IO_A2);

  // -- Buttons --
  d1_startStopToggle.begin(IO_D1);
  d1_startStopToggle.set_mode(BUTTON_MODE_TOGGLE);
  d1_startStopToggle.set_callback_on_toggle(&startStopToggle);

  d2_resetButton.begin(IO_D2);
  d2_resetButton.set_callback_on_press(&reset_radius);

  // -- RPC --
  rpc.begin();
  rpc.enroll("hello", hello_serial);
  rpc.enroll("motors_enable", motors_enable);
  rpc.enroll("motors_disable", motors_disable);

  dance_start();
}

LoopDelay draw_delay;
LoopDelay report_delay;

void loop() {
  draw_delay.periodic_call(&radiusCallBack, 50);
  report_delay.periodic_call(&report_overhead, 500);
  dance_loop();
}

void radiusCallBack() {
  float raw = a1_synth.read();
  float signal = raw / 1023.0;
  signal = constrain(signal, 0.0, 1.0);

  // radius increases slowly
  radius += 0.001;
  if (radius > r_max) reset_radius();

  // LFO modulates radius
  float target_r = radius + signal * 2.0;
  target_r = constrain(target_r, r_min, r_max + 2.0);

  // smoothing: 90% previous + 10% new value
  // increase 0.9/0.1 ratio for smoother, decrease for faster response
  smoothed_r = smoothed_r * 0.9 + target_r * 0.1;

  polar_kinematics.input_radius.set(smoothed_r);
}

void startStopToggle() {
  enable_drivers();
}

void motors_enable()  { enable_drivers(); }
void motors_disable() { disable_drivers(); }

void reset_radius() {
  radius = r_min;
  smoothed_r = r_min;  // smoothed value도 같이 리셋
}

void hello_serial() {
  Serial.print("hello!");
}

void report_overhead() {
  float raw = a1_synth.read();
  float voltage = raw * (3.3 / 1023.0);
  Serial.print(" | LFO: ");       Serial.print(raw);
  Serial.print(" | Voltage: ");   Serial.print(voltage); Serial.print(" V");
  Serial.print(" | Radius: ");    Serial.print(r);
  Serial.print(" | Smoothed: ");  Serial.println(smoothed_r);
}