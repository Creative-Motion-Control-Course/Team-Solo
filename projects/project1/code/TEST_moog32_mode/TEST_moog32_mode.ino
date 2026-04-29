#define module_driver  // Enable Stepdance Driver Module PCB setting (Teensy 4.1 pin configuration)
#include "stepdance.hpp"

// -- Output Ports --
// Output ports that generate step/direction electrical signals
OutputPort output_a;  // AxiDraw Left Motor
OutputPort output_b;  // AxiDraw Right Motor
OutputPort output_c;  // Z-axis Servo Driver (Pen Up/Down)

// -- Motion Channels --
// Tracks target position for each motor and connects to output ports
Channel channel_a;  // AxiDraw A-axis → Left Motor
Channel channel_b;  // AxiDraw B-axis → Right Motor
Channel channel_z;  // AxiDraw Z-axis → Pen Up/Down

// -- Kinematics --
// Converts XY coordinates → AB motor coordinates (CoreXY structure)
KinematicsCoreXY axidraw_kinematics;
KinematicsPolarToCartesian polar_kinematics;

// -- Inputs --
AnalogInput a1_synth;        // A1 Pin - Mother-32 LFO TRI signal input
AnalogInput a2_radiusSpeed;  // for radius speed
Button d1_startStopToggle;   // D1 Toggle - Start/Stop
Button d2_resetButton;       // D2 Button - Reset

VelocityGenerator velocity_gen;

// -- RPC --
// Serial command interface
RPC rpc;

// XXX maybe not need?
// // -- Initialize Time Based Interpolator --
// time_based_interpolator.begin();
// time_based_interpolator.output_x.map(&axidraw_kinematics.input_x);
// time_based_interpolator.output_y.map(&axidraw_kinematics.input_y);
// time_based_interpolator.output_z.map(&channel_z.input_target_position);

// -- State Variables --
float t = 0.0;         // Accumulated time (used for radius slow increment)
float r_min = 3.0;     // minimum base radius
float r_max = 10.0;    // maxium base radius
float radius = r_min;  // base radius
float r;               // actual radius send to polar_Kinematics

// maybe not need? are we putting all these into modular synth part? 
// float drift_state = 0.0;  // Accumulated drift value for Mode 2
// float y_offset = 0.0;     // Y offset for line breaks (increases/decreases by 0.5mm)
// float y_direction = 1.0;  // Y offset movement direction (1.0 = Down, -1.0 = Up)
// float y_step = 0.5;       // Line break interval (mm)

void setup() {
	Serial.begin(9600);

	// -- Initialize Output Ports --
	output_a.begin(OUTPUT_A);  // Use physical port OUTPUT_A on the PCB
	output_b.begin(OUTPUT_B);
	output_c.begin(OUTPUT_C);
	enable_drivers();  // Enable stepper drivers

	// -- Initialize Channels --
	channel_a.begin(&output_a, SIGNAL_E);  // SIGNAL_E: 7us pulse width
	channel_b.begin(&output_b, SIGNAL_E);

	// AxiDraw V3 Ratio: 25.4mm (1 inch) = 2032 steps
	channel_a.set_ratio(25.4, 2032);
	channel_a.invert_output();  // Invert so X-axis direction is Left → Right
	channel_b.set_ratio(25.4, 2032);
	channel_b.invert_output();  // Invert so Y-axis direction is Up → Down
	channel_z.begin(&output_c, SIGNAL_E);
	channel_z.set_ratio(1, 50);  // Servo: step direct pass-thru

	//polar to cartisian
	velocity_gen.begin();
	velocity_gen.output.map(&polar_kinematics.input_angle);

	polar_kinematics.output_x.map(&axidraw_kinematics.input_x);
	polar_kinematics.output_y.map(&axidraw_kinematics.input_y);
	polar_kinematics.begin();

	axidraw_kinematics.output_a.map(&channel_a.input_target_position);
	axidraw_kinematics.output_b.map(&channel_b.input_target_position);
	axidraw_kinematics.begin();

	// signal mapping to radius
	a1_synth.begin(IO_A1);
	a1_synth.set_callback(&radiusCallBack);

	// analogue input mapping to speed
	a2_radiusSpeed.set_floor(0, 25);
	a2_radiusSpeed.set_ceiling(6.28, 1020);  //radians per second
	a2_radiusSpeed.map(&velocity_gen.speed_units_per_sec);
	a2_radiusSpeed.begin(IO_A2);

	// start-stop toggle
	d1_startStopToggle.begin(IO_D1);
	d1_startStopToggle.set_mode(BUTTON_MODE_TOGGLE);
	d1_startStopToggle.set_callback_on_toggle(&startStopToggle);


	d2_resetButton.begin(IO_D2);
	d2_resetButton.set_callback_on_press(&reset_radius);

	// -- Register RPC Serial Commands --
	rpc.begin();
	rpc.enroll("hello", hello_serial);  // Connection test
	// rpc.enroll("reset_origin", reset_origin);      // Reset current position to 0,0
	rpc.enroll("motors_enable", motors_enable);    // Enable motors
	rpc.enroll("motors_disable", motors_disable);  // Disable motors

	dance_start();  // Start Stepdance library
}

LoopDelay draw_delay;
LoopDelay report_delay;

void loop() {
	// maybe don't need, since we're using analogue call back, which is the radiusCallBack()
	// draw_delay.periodic_call(&draw_step, 50);           // Execute drawing every 50ms
	report_delay.periodic_call(&report_overhead, 500);  // Serial output every 500ms
	dance_loop();                                       // Stepdance loop (automatic mapping updates)
}

void radiusCallBack() {
	float raw = a1_synth.read();  // 0-1023 -> 0.0-1.0
	float signal = raw / 1023.0;
	signal = constrain(signal, 0.0, 1.0);

	// incrementing basic radius
	radius = radius + t * 0.05;
	if (radius > r_max) reset_radius();

	r = radius + signal;
	// XXX this feels wrong to send a absolute result into a stream
	// should I use interpolater?
	// r.map(polar_to_cartesian_kinematics.input_radius);
	polar_kinematics.input_radius.set(r);
}

void startStopToggle() {
	enable_drivers();
}

void motors_enable() {
	enable_drivers();
}
void motors_disable() {
	disable_drivers();
}

//
void reset_radius() {
	// axidraw_kinematics.input_x.reset_deep(0);
	// axidraw_kinematics.input_y.reset_deep(0);
	// t = 0.0;
	// drift_state = 0.0;
	// y_offset = 0.0;
	// y_direction = 1.0;
	radius = 0;
	t = 0;
}

void hello_serial() {
	Serial.print("hello!");  // For connection testing
}

// -- Serial Status Output --
// Outputs current Mode, LFO value, Voltage, Y offset, and XY position every 500ms
void report_overhead() {
	float raw = a1_synth.read();
	float voltage = raw * (3.3 / 1023.0);  // Convert raw → voltage (V)
	Serial.print(" | LFO: ");
	Serial.print(raw);
	Serial.print(" | Voltage: ");
	Serial.print(voltage);
	Serial.print(" V");
	// Serial.print(" | Y offset: ");
	// Serial.print(y_offset);
	// Serial.print(" | X: ");
	// Serial.print(axidraw_kinematics.input_x.read(ABSOLUTE));
	// Serial.print(" | Y: ");
	// Serial.println(axidraw_kinematics.input_y.read(ABSOLUTE));
}

// ********************************
// XXX maybe dont need?
/*
// -- Line Break Function --
// Called when X reaches the end of B5 (182mm)
// Moves Y by 0.5mm, reverses direction if it hits the limit (257mm)
void next_line()
{
	t = 0.0;
	drift_state = 0.0;
	y_offset += y_step * y_direction;

	// Check Y limits: 0~257mm (B5 Vertical)
	if (y_offset > 257.0 || y_offset < 0.0)
	{
		y_direction *= -1.0;  // Reverse direction
		y_offset += y_step * y_direction;
	}

	// Move pen to the left edge and then to the new line Y position
	position_gen_x.go(0, GLOBAL, 30.0);
	position_gen_y.go(y_offset, GLOBAL, 30.0);
}
// -- Drawing Function --
// Called every 50ms, reads LFO and calculates XY per mode to move the plotter
void draw_step()
{
	// Read LFO: raw 0~1023 → normalize to 0.0~1.0
	float raw = lfo_input.read();
	float signal = raw / 1023.0;
	signal = constrain(signal, 0.0, 1.0);

	float x = 0.0;
	float y = 0.0;

	if (mode == 0)
	{
		// Mode 0: Wave
		// LFO intensity (signal) controls the sin wave amplitude
		x = t * 2.0;
		y = sin(t) * signal * 10.0;
		if (x > 182.0) { next_line(); return; }  // B5 Width 182mm
		y = constrain(y, -10.0, 10.0);
		position_gen_x.go(x, GLOBAL, 8.0);
		position_gen_y.go(y + y_offset, GLOBAL, 8.0);
		t += 0.1;
	}
	else if (mode == 1)
	{
		// Mode 1: Noise
		// Adds random noise based on LFO intensity to the sin wave
		x = t * 2.0;
		float noise = ((float)random(-100, 100)) / 100.0;
		y = (sin(t) + signal * noise) * 10.0;
		if (x > 182.0) { next_line(); return; }
		y = constrain(y, -10.0, 10.0);
		position_gen_x.go(x, GLOBAL, 8.0);
		position_gen_y.go(y + y_offset, GLOBAL, 8.0);
		t += 0.1;
	}
	else if (mode == 2)
	{
		// Mode 2: Drift
		// LFO signal accumulates above/below 0.5, causing the line to drift slowly
		x = t * 2.0;
		drift_state += (signal - 0.5) * 0.3;
		drift_state = constrain(drift_state, -20.0, 20.0);
		y = drift_state;
		if (x > 182.0) { next_line(); return; }
		y = constrain(y, -10.0, 10.0);
		position_gen_x.go(x, GLOBAL, 8.0);
		position_gen_y.go(y + y_offset, GLOBAL, 8.0);
		t += 0.1;
	}
	else if (mode == 3)
	{
		// Mode 3: Circular
		// LFO intensity (signal) controls the radius of the circle
		// B5 Center: X=91mm, Y=128.5mm
		float radius = signal * 30.0;
		x = 91.0 + cos(t) * radius;
		y = 128.5 + sin(t) * radius;
		x = constrain(x, 0.0, 182.0);  // B5 Horizontal range
		y = constrain(y, 0.0, 257.0);  // B5 Vertical range
		position_gen_x.go(x, GLOBAL, 15.0);
		position_gen_y.go(y, GLOBAL, 15.0);
		t += 0.05;  // Circle drawing speed
	}
}
// -- Mode Switching --
// Cycles 0→1→2→3→0 on button press
// Resets all states upon mode transition
void switch_mode()
{
	mode = (mode + 1) % 4;
	t = 0.0;
	drift_state = 0.0;
	y_offset = 0.0;
	y_direction = 1.0;
	Serial.print("Mode: ");
	Serial.println(mode);
}
// *********************************
*/