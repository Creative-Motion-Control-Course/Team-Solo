#define module_driver   // Stepdance Driver Module PCB 사용 설정 (Teensy 4.1 핀 설정)
#include "stepdance.hpp"

// -- Output Ports --
// step/direction 전기 신호를 생성하는 출력 포트
OutputPort output_a;  // AxiDraw 왼쪽 모터
OutputPort output_b;  // AxiDraw 오른쪽 모터
OutputPort output_c;  // Z축 서보 드라이버 (펜 업/다운)

// -- Motion Channels --
// 각 모터의 목표 위치를 추적하고 출력 포트와 연결
Channel channel_a;  // AxiDraw A축 → 왼쪽 모터
Channel channel_b;  // AxiDraw B축 → 오른쪽 모터
Channel channel_z;  // AxiDraw Z축 → 펜 업/다운

// -- Kinematics --
// XY 좌표 → AB 모터 좌표로 변환 (CoreXY 구조)
KinematicsCoreXY axidraw_kinematics;

// -- Inputs --
AnalogInput lfo_input;  // A1 핀 - Mother-32 LFO TRI 신호 입력
Button button_d1;       // D1 핀 - 모드 전환 버튼

// -- RPC --
// 시리얼 명령 인터페이스
RPC rpc;

// -- Position Generators --
// 단발성 위치 명령 생성
PositionGenerator position_gen_x;  // X축 위치 제어
PositionGenerator position_gen_y;  // Y축 위치 제어

// -- Time Based Interpolator --
// 여러 위치 명령을 큐에 쌓아서 순차 실행
TimeBasedInterpolator time_based_interpolator;

// -- State Variables --
int mode = 0;             // 현재 모드 (0~3)
float t = 0.0;            // 시간 누적값 (X축 이동 및 sin 계산에 사용)
float drift_state = 0.0;  // Mode 2 drift 누적값
float y_offset = 0.0;     // 줄 바꿈 Y 오프셋 (0.5mm씩 증가/감소)
float y_direction = 1.0;  // Y 오프셋 이동 방향 (1.0=아래, -1.0=위)
float y_step = 0.5;       // 줄 바꿈 간격 (mm)

void setup() {
  Serial.begin(9600);

  // -- 출력 포트 초기화 --
  output_a.begin(OUTPUT_A);  // PCB의 OUTPUT_A 물리 포트 사용
  output_b.begin(OUTPUT_B);
  output_c.begin(OUTPUT_C);
  enable_drivers();  // 스테퍼 드라이버 활성화

  // -- 채널 초기화 --
  channel_a.begin(&output_a, SIGNAL_E);  // SIGNAL_E: 7us 펄스폭
  channel_b.begin(&output_b, SIGNAL_E);
  // AxiDraw V3 비율: 25.4mm(1인치) = 2032 steps
  channel_a.set_ratio(25.4, 2032);
  channel_a.invert_output();  // X축이 왼→오 방향이 되도록 반전
  channel_b.set_ratio(25.4, 2032);
  channel_b.invert_output();  // Y축이 위→아래 방향이 되도록 반전

  channel_z.begin(&output_c, SIGNAL_E);
  channel_z.set_ratio(1, 50);  // 서보: step 직통 pass-thru

  // -- 키네마틱스 초기화 --
  axidraw_kinematics.begin();
  // 키네마틱스 출력을 각 채널 입력에 매핑
  axidraw_kinematics.output_a.map(&channel_a.input_target_position);
  axidraw_kinematics.output_b.map(&channel_b.input_target_position);

  // -- LFO 인풋 초기화 --
  lfo_input.begin(IO_A1);  // A1 핀에서 아날로그 신호 읽기

  // -- 버튼 초기화 --
  button_d1.begin(IO_D1, INPUT_PULLDOWN);
  button_d1.set_mode(BUTTON_MODE_STANDARD);
  button_d1.set_callback_on_press(&switch_mode);  // 누를 때마다 모드 전환

  // -- Position Generator 초기화 --
  position_gen_x.output.map(&axidraw_kinematics.input_x);
  position_gen_x.begin();
  position_gen_y.output.map(&axidraw_kinematics.input_y);
  position_gen_y.begin();

  // -- Time Based Interpolator 초기화 --
  time_based_interpolator.begin();
  time_based_interpolator.output_x.map(&axidraw_kinematics.input_x);
  time_based_interpolator.output_y.map(&axidraw_kinematics.input_y);
  time_based_interpolator.output_z.map(&channel_z.input_target_position);

  // -- RPC 시리얼 명령 등록 --
  rpc.begin();
  rpc.enroll("hello", hello_serial);          // 연결 테스트
  rpc.enroll("reset_origin", reset_origin);   // 현재 위치를 0,0으로 리셋
  rpc.enroll("motors_enable", motors_enable); // 모터 활성화
  rpc.enroll("motors_disable", motors_disable); // 모터 비활성화

  dance_start();  // Stepdance 라이브러리 시작
}

LoopDelay draw_delay;
LoopDelay report_delay;

void loop() {
  draw_delay.periodic_call(&draw_step, 50);       // 50ms마다 드로잉 실행
  report_delay.periodic_call(&report_overhead, 500); // 500ms마다 시리얼 출력
  dance_loop();  // Stepdance 루프 (매핑 자동 업데이트)
}

// -- 줄 바꿈 함수 --
// X가 B5 끝(182mm)에 도달하면 호출
// Y를 0.5mm씩 이동, 리밋(257mm) 치면 방향 반전
void next_line() {
  t = 0.0;
  drift_state = 0.0;
  y_offset += y_step * y_direction;

  // Y 리밋 체크: 0~257mm (B5 세로)
  if (y_offset > 257.0 || y_offset < 0.0) {
    y_direction *= -1.0;  // 방향 반전
    y_offset += y_step * y_direction;
  }

  // 펜을 왼쪽 끝으로 이동 후 새 줄 Y 위치로 이동
  position_gen_x.go(0, GLOBAL, 30.0);
  position_gen_y.go(y_offset, GLOBAL, 30.0);
}

// -- 드로잉 함수 --
// 50ms마다 호출, LFO 읽어서 모드별 XY 계산 후 플로터 이동
void draw_step() {
  // LFO 읽기: raw 0~1023 → 0.0~1.0 정규화
  float raw = lfo_input.read();
  float signal = raw / 1023.0;
  signal = constrain(signal, 0.0, 1.0);

  float x = 0.0;
  float y = 0.0;

  if (mode == 0) {
    // Mode 0: Wave
    // LFO 세기(signal)가 sin 웨이브 진폭을 조절
    x = t * 2.0;
    y = sin(t) * signal * 10.0;
    if (x > 182.0) { next_line(); return; }  // B5 가로 182mm
    y = constrain(y, -10.0, 10.0);
    position_gen_x.go(x, GLOBAL, 8.0);
    position_gen_y.go(y + y_offset, GLOBAL, 8.0);
    t += 0.1;

  } else if (mode == 1) {
    // Mode 1: Noise
    // sin 웨이브에 LFO 세기만큼 랜덤 노이즈 추가
    x = t * 2.0;
    float noise = ((float)random(-100, 100)) / 100.0;
    y = (sin(t) + signal * noise) * 10.0;
    if (x > 182.0) { next_line(); return; }
    y = constrain(y, -10.0, 10.0);
    position_gen_x.go(x, GLOBAL, 8.0);
    position_gen_y.go(y + y_offset, GLOBAL, 8.0);
    t += 0.1;

  } else if (mode == 2) {
    // Mode 2: Drift
    // LFO 신호가 0.5 기준으로 위/아래 누적되어 선이 서서히 흘러감
    x = t * 2.0;
    drift_state += (signal - 0.5) * 0.3;
    drift_state = constrain(drift_state, -20.0, 20.0);
    y = drift_state;
    if (x > 182.0) { next_line(); return; }
    y = constrain(y, -10.0, 10.0);
    position_gen_x.go(x, GLOBAL, 8.0);
    position_gen_y.go(y + y_offset, GLOBAL, 8.0);
    t += 0.1;

  } else if (mode == 3) {
    // Mode 3: Circular
    // LFO 세기(signal)가 원의 반지름을 조절
    // B5 중앙: X=91mm, Y=128.5mm
    float radius = signal * 30.0;
    x = 91.0 + cos(t) * radius;
    y = 128.5 + sin(t) * radius;
    x = constrain(x, 0.0, 182.0);  // B5 가로 범위
    y = constrain(y, 0.0, 257.0);  // B5 세로 범위
    position_gen_x.go(x, GLOBAL, 15.0);
    position_gen_y.go(y, GLOBAL, 15.0);
    t += 0.05;  // 원 그리는 속도
  }
}

// -- 모드 전환 --
// 버튼 누를 때마다 0→1→2→3→0 순환
// 모드 전환시 모든 state 초기화
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

// -- 원점 리셋 --
// 현재 위치를 XY 0,0으로 리셋, 모든 state 초기화
void reset_origin() {
  axidraw_kinematics.input_x.reset_deep(0);
  axidraw_kinematics.input_y.reset_deep(0);
  t = 0.0;
  drift_state = 0.0;
  y_offset = 0.0;
  y_direction = 1.0;
}

void hello_serial() {
  Serial.print("hello!");  // 연결 테스트용
}

// -- 시리얼 상태 출력 --
// 500ms마다 현재 Mode, LFO값, 전압, Y오프셋, XY 위치 출력
void report_overhead() {
  float raw = lfo_input.read();
  float voltage = raw * (3.3 / 1023.0);  // raw → 전압(V) 변환
  Serial.print("Mode: "); Serial.print(mode);
  Serial.print(" | LFO: "); Serial.print(raw);
  Serial.print(" | Voltage: "); Serial.print(voltage); Serial.print(" V");
  Serial.print(" | Y offset: "); Serial.print(y_offset);
  Serial.print(" | X: "); Serial.print(axidraw_kinematics.input_x.read(ABSOLUTE));
  Serial.print(" | Y: "); Serial.println(axidraw_kinematics.input_y.read(ABSOLUTE));
}