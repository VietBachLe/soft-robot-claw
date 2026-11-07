/* 
 * KHIỂN ROBOT MỀM SPIROBS QUA SERIAL MONITOR
 * Nhập 'c' -> Cuộn hết cỡ (dừng khi kẹt cơ khí)
 * Nhập 'u' -> Duỗi hết cỡ (dừng khi đạt giới hạn hành trình)
 * Nhập 's' -> Dừng khẩn cấp
 */

// Định nghĩa chân phần cứng
const int ENA = 9;       
const int IN1 = 7;       
const int IN2 = 8;       
const int ENC1_A = 3;    
const int ENC1_B = 4;    

const int ENB = 10;      
const int IN3 = 5;       
const int IN4 = 6;       
const int ENC2_A = 2;    
const int ENC2_B = 18;   

// Biến đếm xung Encoder
volatile long encoderCount1 = 0; 
volatile long encoderCount2 = 0; 

// Hằng số giới hạn hành trình (Bạn có thể thay đổi số xung này cho khớp với thực tế robot của nhóm)
const long MAX_UNCURL_PULTS = 1500; // Số xung tối đa khi duỗi thẳng hoàn toàn
const int STALL_THRESHOLD = 3;      // Ngưỡng kiểm tra kẹt cơ khí (nếu trong một khoảng thời gian xung không tăng)

void isrEncoder1() {
  if (digitalRead(ENC1_B) == HIGH) encoderCount1++; else encoderCount1--;
}

void isrEncoder2() {
  if (digitalRead(ENC2_B) == HIGH) encoderCount2++; else encoderCount2--;
}

void stopRobot() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  pinMode(ENC1_A, INPUT_PULLUP); pinMode(ENC1_B, INPUT_PULLUP);
  pinMode(ENC2_A, INPUT_PULLUP); pinMode(ENC2_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC1_A), isrEncoder1, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC2_A), isrEncoder2, RISING);

  stopRobot();
  
  Serial.println("==========================================");
  Serial.println("DA SANG SANG! Hay nhap lenh tren Serial Monitor:");
  Serial.println("  - Nhan 'c': Cuon len (dung khi ket co khi)");
  Serial.println("  - Nhan 'u': Duoi het co");
  Serial.println("  - Nhan 's': Dung khan cap");
  Serial.println("==========================================");
}

void loop() {
  // Kiểm tra xem người dùng có gõ phím xuống Serial Monitor không
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    if (cmd == 'c' || cmd == 'C') {
      Serial.println("--> Dang cuon tay don...");
      runCurlingUntilBlocked();
    } 
    else if (cmd == 'u' || cmd == 'U') {
      Serial.println("--> Dang duoi tay don het co...");
      runUncurlingToLimit();
    }
    else if (cmd == 's' || cmd == 'S') {
      Serial.println("--> Dung khan cap!");
      stopRobot();
    }
  }
}

// Hàm cuộn: Chạy liên tục cho đến khi kẹt cơ khí (xung encoder ngừng tăng)
void runCurlingUntilBlocked() {
  // Kích hoạt motor cuộn
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, 180);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); analogWrite(ENB, 120);

  long lastCount = -999;
  int stableTimes = 0;

  while (true) {
    long currentCount = encoderCount1; // Lấy mốc xung của motor trung tâm
    
    // Kiểm tra nếu số xung không thay đổi trong vài nhịp kiểm tra liên tiếp -> Đã kẹt cứng / cuộn hết cỡ
    if (abs(currentCount - lastCount) < STALL_THRESHOLD) {
      stableTimes++;
      if (stableTimes > 10) { // Nếu đứng yên trong khoảng 100ms thì dừng
        Serial.println("--> Da cuon toi da (Cham gioi han co khi). Dung lai!");
        break;
      }
    } else {
      stableTimes = 0; // Reset lại nếu vẫn đang chạy
      lastCount = currentCount;
    }
    
    // Cho phép dừng khẩn cấp bằng phím 's' ngay trong lúc đang chạy vòng lặp
    if (Serial.available() > 0 && Serial.peek() == 's') {
      Serial.read();
      break;
    }
    
    delay(10);
  }
  stopRobot();
}

// Hàm duỗi: Chạy cho đến khi đạt đủ số xung giới hạn hành trình duỗi
void runUncurlingToLimit() {
  // Kích hoạt motor duỗi
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); analogWrite(ENA, 120);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  analogWrite(ENB, 180);

  long startEncoder = encoderCount2;

  while (true) {
    long currentDisplacement = abs(encoderCount2 - startEncoder);

    // Nếu đã dịch chuyển đủ số xung giới hạn cho phép duỗi thẳng
    if (currentDisplacement >= MAX_UNCURL_PULTS) {
      Serial.println("--> Da duoi het co hanh trinh. Dung lai!");
      break;
    }

    // Cho phép dừng khẩn cấp bằng phím 's'
    if (Serial.available() > 0 && Serial.peek() == 's') {
      Serial.read();
      break;
    }

    delay(10);
  }
  stopRobot();
}