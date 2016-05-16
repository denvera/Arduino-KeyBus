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
  Serial1.begin(115200);
  Serial1.println("DSC KeyBus Gateway 0.1\r\n");
  
  cli();  
 // TCCR0A = 0; // Dsiable counter 0 (and its interrupts)
 // TCCR0B = 0;
  
  EICRA |= (1 << ISC11 | 1 << ISC10); // Rising edge triggers INT1
  EIMSK |= (1 << INT1);  
 
//  // set up timer with prescaler = 64 and CTC mode
  TCCR1A = 0;
  //TCCR1B = (1 << WGM12)|(1 << CS11)|(1 << CS10);
  TCCR1B = (1 << WGM12);
//  // initialize counter
  //TCNT1 = 0;
//  // initialize compare value
  OCR1A = 76; // 640 uS
  //OCR1A = e6
//  // enable compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  sei();
  //Serial.begin(115200);
  //Serial.println("DSC KeyBus Gateway 0.1 [USB]");

  // Interrupt 1 = Pin 2 = PD1
  //attachInterrupt(1, bit_isr, CHANGE);
  for (unsigned char i = 0; i < 2; i++) {    
    memset((char *)cur_msg[i], 0, 32);
    memset((char *)last_msg[i], 0, 32);
  }
}

void loop() {
  /*if (millis() - led_ts > 1000) {
    PORTC ^= 1 << 7;
    led_ts = millis();
  } */ 
  if (new_bit) {
    read_ts = micros();
    new_bit = false;
    ++bit_count;
    if (bit_count == 8 || bit_count == 9 || (bit_count > 9 && ((bit_count - 9) % 8 == 0))) byte_count++;    

    //delayMicroseconds(560);    
    //bit_processed = false;        
  }
//  if ((micros() - read_ts > 650) && !bit_processed) {
//    bit_processed = true;
//    //new_bit = false;        
//    // Read bit from client
////    cur_msg[CLIENT][byte_count] <<= 1;
////    cur_msg[CLIENT][byte_count] |= ((PIND >> 0) & 1);
//    // Write if necessary
//    
//    // Split up bytes    
//  }
  if (((micros() - read_ts) > 2000) && (bit_count >= 17)) {
  //if (write_msg) {
    write_msg = false;
    new_msg();
  }  
}

//void bit_isr() {
ISR(INT1_vect) {  
  if (PIND & _BV(1)) {
    // Rising, read
    //read_ts = micros();
    cur_msg[MASTER][byte_count] <<= 1;
    cur_msg[MASTER][byte_count] |= (PIND >> 0) & 1;
    //cur_msg[byte_count] |= digitalRead(DSC_DATA);           
    TCNT1 = 0; 
    TCCR1B = (1 << WGM12)|(1 << CS11)|(1 << CS10); 
    //new_bit = true; 
  } else {

    // Falling, write
    
  }
}

ISR (TIMER1_COMPA_vect)
{  
//  if (PIND & _BV(1)) {
//    TCCR1B = (1 << WGM12);
//    write_msg = true;
//    return;
//  }  
  cur_msg[CLIENT][byte_count] <<= 1;
  cur_msg[CLIENT][byte_count] |= ((PIND >> 0) & 1);    
//  ++bit_count;
//  if (bit_count == 8 || bit_count == 9 || (bit_count > 9 && ((bit_count - 9) % 8 == 0))) byte_count++;
  TCCR1B = (1 << WGM12);  
  new_bit = true;  
}

void new_msg() {
  if (byte_count <= 3) goto bad_msg;
  if (!binary) {
    if (memcmp((char *)last_msg[MASTER], (char *)cur_msg[MASTER], 32) || last_msg[CLIENT][2] != cur_msg[CLIENT][2]) {
      print_msg();
      memcpy((char *)last_msg[MASTER], (char *)cur_msg[MASTER], 32);
      memcpy((char *)last_msg[CLIENT], (char *)cur_msg[CLIENT], 32);
    }
    memset((char *)cur_msg[MASTER], 0, 32);
    memset((char *)cur_msg[CLIENT], 0, 32);
  }
bad_msg:
  byte_count = 0;
  bit_count = 0;
  PORTC ^= 1 << 7;
}


void print_msg() {
   Serial1.print("M [B: "); Serial1.print(byte_count, DEC);
    Serial1.print(" b: "); Serial1.print(bit_count, DEC);        
    Serial1.print("]: ");
    for (int i = 0; i <= byte_count; i++) {
      Serial1.print((unsigned char)cur_msg[MASTER][i], HEX); Serial1.print(" ");
    }
    Serial1.print("\tC: ");
    for (int i = 0; i <= byte_count; i++) {
      Serial1.print((unsigned char)cur_msg[CLIENT][i], HEX); Serial1.print(" ");
    }    
    Serial1.println();}


