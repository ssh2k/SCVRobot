/*
  Next.js 로봇 제어용 최종 웹 서버 스케치 (통합 버전)
  - A* 경로탐색, 경로실행, 저수준 모터 제어 및 Lift 클래스 모듈 통합
*/

#include "WiFiS3.h"
#include "lift.h"
#include "astar5x5.h"
#include "PathRunner.h"
#include "arduino_secrets.h" 

// =================================================================
// 1. 와이파이 정보
// =================================================================
char ssid[] = SECRET_SSID;      // your network SSID (name)
char pass[] = SECRET_PASS;      // your network password
// =================================================================

int status = WL_IDLE_STATUS;
WiFiServer server(80);

// --- 제어 모듈 객체 생성 ---
Lift       robotLift;           // 리프트 제어기
gridMove   mover;               // 저수준 모터 제어기
PathRunner runner(mover, 150);  // 경로 실행기 (칸 이동 후 150ms 대기)

// --- 로봇의 현재 상태 ---
int currentX = 0; // 로봇의 현재 X 좌표 (0-4)
int currentY = 4; // 로봇의 현재 Y 좌표 (0-4)
bool isLiftUpState = false; // 리프트의 논리적 상태 (UI 토글용)

// +++ [수정] 목표 좌표를 임시 저장할 변수 추가 +++
int pathGoalX = 0;
int pathGoalY = 0;

// --- 5x5 지도 정의 ---
// true(1): 장애물, false(0): 이동 가능 경로
const bool grid[5][5] = {
  {0, 0, 0, 0, 0}, // 웹 앱의 최하단 줄과 일치
  {0, 1, 1, 0, 0},
  {0, 0, 0, 0, 0},
  {1, 0, 0, 1, 0},
  {0, 0, 0, 1, 0}  // 웹 앱의 최상단 줄과 일치
};

PathRunner::Node pathNodes[PathRunner::MAX_POINTS];

void setup() {
  Serial.begin(115200);
  
  robotLift.begin();
  Serial.println("Movement System Initialized.");

  while (status != WL_CONNECTED) {
    Serial.print("Connecting to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  server.begin();
  IPAddress ip = WiFi.localIP();
  Serial.println("\n>>> 와이파이 연결 및 서버 시작 완료! <<<");
  Serial.print("서버 주소: http://");
  Serial.println(ip);
  Serial.println("---------------------------------");
  Serial.println("Robot is at initial position (" + String(currentX) + ", " + String(currentY) + ")");
}

void loop() {
  runner.update();
  robotLift.update();

  // --- 경로 실행 완료 감지 ---
  static bool pathWasActive = false;
  bool pathIsActive = !runner.isFinished();
  if (pathWasActive && !pathIsActive) {
    // +++ [수정] 경로 완료 시, 저장해둔 목표 좌표로 현재 위치 업데이트 +++
    currentX = pathGoalX;
    currentY = pathGoalY;
    Serial.println(">>> Path finished. New position: (" + String(currentX) + ", " + String(currentY) + ")");
  }
  pathWasActive = pathIsActive;

  // --- 웹 클라이언트 처리 ---
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    int cmd_index = request.indexOf("?cmd=");
    if (cmd_index != -1) {
      int end_index = request.indexOf(' ', cmd_index);
      String command = request.substring(cmd_index + 5, end_index);
      
      Serial.print("\nReceived Command: ");
      Serial.println(command);

      if (runner.isFinished()) {
        if (command.startsWith("move_")) {
          String params = command.substring(5);
          int separator_index = params.indexOf('_');
          if (separator_index != -1) {
            int webX = params.substring(0, separator_index).toInt();
            int webY = params.substring(separator_index + 1).toInt();
            int targetGridX = constrain((webX * 5) / 100, 0, 4);
            int targetGridY = constrain((webY * 5) / 100, 0, 4);

            targetGridY = 4 - targetGridY;
            moveToGridPosition(targetGridX, targetGridY);
          }
        }
        else if (command == "lift_up") {
          Serial.println("Action: Lift Up command received.");
          robotLift.upFor(2000);
          isLiftUpState = true;
        }
        else if (command == "lift_down") {
          Serial.println("Action: Lift Down command received.");
          robotLift.downFor(2000);
          isLiftUpState = false;
        }
        // ... (다른 명령어들도 여기에 추가) ...
      }
      
      if (command == "disconnected") {
        Serial.println("Action: System Disconnected. Forcing stop.");
        runner.forceStop();
        robotLift.stop();
      }
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Connection: close");
    client.println();
    delay(1);
    client.stop();
  }
}

// 그리드 좌표로 이동 계획 및 실행
void moveToGridPosition(int targetX, int targetY) {
  if (!runner.isFinished()) {
    Serial.println("Cannot start new move: PathRunner is busy.");
    return;
  }
  
  Serial.println("Planning path from (" + String(currentX) + ", " + String(currentY) + ") to (" + String(targetX) + ", " + String(targetY) + ")");
  AStarResult result = planAstar5x5(grid, currentX, currentY, targetX, targetY, pathNodes, PathRunner::MAX_POINTS);

  if (result.ok) {
    Serial.println("Path found with " + String(result.n) + " nodes. Starting movement.");
    
    // +++ [수정] 경로 실행 전, 목표 좌표를 임시 변수에 저장 +++
    pathGoalX = targetX;
    pathGoalY = targetY;

    runner.loadPath(pathNodes, result.n);
    runner.start();
  } else {
    Serial.println("!!! Path not found !!!");
  }
}
