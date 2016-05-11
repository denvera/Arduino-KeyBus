#define DSC_CLK 2 // PD1
#define DSC_DATA 3 // PPD 0

#define STATUS_LED 13 // PC7

volatile unsigned long read_ts = 0;
volatile unsigned long led_ts = 0;

volatile unsigned char cur_msg[32];
volatile unsigned char cur_c_msg[32];
volatile char byte_count = 0;
volatile char bit_count = 0;
volatile bool new_bit = false, bit_processed = false;

enum state { IDLE, NEW_BIT, WAIT_READ };
enum state current_state = IDLE;

bool binary = false;

void setup() {
  // put your setup code here, to run once:

  pinMode(STATUS_LED, OUTPUT);
  pinMode(DSC_CLK, INPUT_PULLUP);
  pinMode(DSC_DATA, INPUT_PULLUP);
  //Serial.begin(115200);
  //Serial.println("DSC KeyBus Gateway 0.1 [USB]");
  Serial1.begin(115200);
  Serial1.println("DSC KeyBus Gateway 0.1\r\n");
  // Interrupt 1 = Pin 2 = PD1
  attachInterrupt(1, bit_isr, RISING);
  memset((char *)cur_msg, 0, 32);
}

void loop() {
  if (millis() - led_ts > 1000) {
    PORTC ^= 1 << 7;
    led_ts = millis();
  }  
  if (new_bit) {
    read_ts = micros();
    new_bit = false;
    bit_processed = false;
    ++bit_count;
    if (bit_count == 8 || bit_count == 9 || (bit_count > 9 && ((bit_count - 9) % 8 == 0))) byte_count++;
  }
  if ((micros() - read_ts > 600) && !bit_processed) {
    //new_bit = false;        
    // Read bit from client
    cur_c_msg[byte_count] <<= 1;
    cur_c_msg[byte_count] |= ((PIND >> 0) & 1);
    // Write if necessary
    bit_processed = true;
    // Split up bytes

    current_state = IDLE;
  }
  if (((micros() - read_ts) > 2000) && (bit_count >= 17)) {
    new_msg();
  }  
}

void bit_isr() {
  if (PIND & _BV(1)) {
    // Rising, read
    //read_ts = micros();
    cur_msg[byte_count] <<= 1;
    cur_msg[byte_count] |= (PIND >> 0) & 1;
    //cur_msg[byte_count] |= digitalRead(DSC_DATA);
    new_bit = true;    
  } else {
    // Falling, write
  }
}

void new_msg() {

  if (!binary) {
    Serial1.print("M [B: "); Serial1.print(byte_count, DEC);
    Serial1.print(" b: "); Serial1.print(bit_count, DEC);    
    bit_count = 0;  
    Serial1.print("]: ");
    for (int i = 0; i < byte_count; i++) {
      Serial1.print((unsigned char)cur_msg[i], HEX); Serial1.print(" ");
    }
    Serial1.print("\tC: ");
    for (int i = 0; i < byte_count; i++) {
      Serial1.print((unsigned char)cur_c_msg[i], HEX); Serial1.print(" ");
    }
    byte_count = 0;
    Serial1.println();
    memset((char *)cur_msg, 0, 32);
    memset((char *)cur_c_msg, 0, 32);
  }
}

