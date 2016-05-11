#define DSC_CLK 2 // PD1
#define DSC_DATA 3 // PPD 0

#define STATUS_LED 13 // PC7

#define MASTER 0
#define CLIENT 1

volatile unsigned long read_ts = 0;
volatile unsigned long led_ts = 0;

volatile unsigned char cur_msg[2][32];
volatile unsigned char last_msg[2][32];

volatile char byte_count = 0;
volatile char bit_count = 0;
volatile bool new_bit = false, bit_processed = false, write_msg = false;

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
  attachInterrupt(1, bit_isr, CHANGE);
  memset((char *)cur_msg[MASTER], 0, 32);
  memset((char *)cur_msg[CLIENT], 0, 32);
}

void loop() {
  /*if (millis() - led_ts > 1000) {
    PORTC ^= 1 << 7;
    led_ts = millis();
  } */ 
  if (new_bit) {
    read_ts = micros();
    ++bit_count;
    if (bit_count == 8 || bit_count == 9 || (bit_count > 9 && ((bit_count - 9) % 8 == 0))) byte_count++;    
    //delayMicroseconds(660);
    cur_msg[CLIENT][byte_count] <<= 1;
    cur_msg[CLIENT][byte_count] |= ((PIND >> 0) & 1);
    new_bit = false;
    bit_processed = false;
    
    
  //}
  //if ((micros() - read_ts > 50) && !bit_processed) {
    //new_bit = false;        
    // Read bit from client

    // Write if necessary
    //bit_processed = true;
    // Split up bytes    
  }
  if (((micros() - read_ts) > 4000) && (bit_count >= 17)) {
    new_msg();
  }  
}

void bit_isr() {
  if (PIND & _BV(1)) {
    // Rising, read
    //read_ts = micros();
    cur_msg[MASTER][byte_count] <<= 1;
    cur_msg[MASTER][byte_count] |= (PIND >> 0) & 1;
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
      Serial1.print((unsigned char)cur_msg[MASTER][i], HEX); Serial1.print(" ");
    }
    Serial1.print("\tC: ");
    /*for (int i = 0; i < byte_count; i++) {
      Serial1.print((unsigned char)cur_msg[CLIENT][i], HEX); Serial1.print(" ");
    }*/
    byte_count = 0;
    Serial1.println();
    memset((char *)cur_msg[MASTER], 0, 32);
    memset((char *)cur_msg[CLIENT], 0, 32);
  }
}

