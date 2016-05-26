#define DSC_CLK  ((PIND & _BV(1)) >> 1)
#define DSC_DATA ((PIND & 1))
#define DSC_CLK_PIN 2 // PD1
#define DSC_DATA_PIN 3 // PPD 0

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

enum state { IDLE, START_BIT, NEW_BIT, WAIT_READ, NEW_MSG };
enum state current_state = IDLE;

bool binary = false;

void (*start_bootloader) (void)=(void (*)(void))0x3800;

void setup() {
  // put your setup code here, to run once:

  pinMode(STATUS_LED, OUTPUT);
  pinMode(DSC_CLK_PIN, INPUT_PULLUP);
  pinMode(DSC_DATA_PIN, INPUT_PULLUP);
  Serial1.begin(115200);
  Serial1.println("DSC KeyBus Gateway 0.1\r\n");  
 
  for (unsigned char i = 0; i < 2; i++) {    
    memset((char *)cur_msg[i], 0, 32);
    memset((char *)last_msg[i], 0, 32);
  }
  current_state = IDLE;
  
}


void loop() {        
  switch (current_state) {
    case IDLE:         
        //Serial1.println(DSC_CLK, HEX);
        //Serial1.println("IDLE");
        current_state =  (wait_clk(0, 50, 2000) > 1000) ? IDLE : NEW_BIT;
        // Clk low - start bit            
        break;
    case START_BIT:
        wait_clk(1, 100, 2000);
        current_state = NEW_BIT;
        break;
    case NEW_BIT:
        //Serial1.println("NEW_BIT");
        //if (DSC_CLK) { goto dont_wait; }
        wait_clk(1, 50, 2000);
dont_wait:
        cur_msg[MASTER][byte_count] <<= 1;
        cur_msg[MASTER][byte_count] |= (PIND >> 0) & 1;        
        //if (!DSC_CLK) break;
        if (wait_clk(0, 50, 2000) > 1000) {
          //Serial1.println("NEW_MSG");
          current_state = NEW_MSG;
        }
        for (volatile uint8_t i = 0; i < 64; i++) { 0; }
        cur_msg[CLIENT][byte_count] <<= 1;
        cur_msg[CLIENT][byte_count] |= ((PIND >> 0) & 1);    
        ++bit_count;
        if (bit_count == 8 || bit_count == 9 || (bit_count > 9 && ((bit_count - 9) % 8 == 0))) byte_count++;        
        break;
    case NEW_MSG:
        new_msg();
        PORTC ^= 1 << 7;
        current_state = IDLE;
        break;            
  }                 
}


int wait_clk(uint8_t level, int wait_step_us, int timeout_us) {
  unsigned long start_ts = micros();
  while (((DSC_CLK != level) && (micros() - start_ts) <= timeout_us)) {
    delayMicroseconds(wait_step_us);        
  }
return (micros() - start_ts);

  int count = 0;
  int timeout = 0;
  volatile int x = 10; // 50us?
  while (((DSC_CLK != level) && ((count*50) <= timeout_us))) {
    //while (x--);
    count++;    
  }
  return (count*50);
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
  cur_msg[CLIENT][byte_count] <<= 1;
  cur_msg[CLIENT][byte_count] |= ((PIND >> 0) & 1);    
  TCCR1B = (1 << WGM12);  
  new_bit = true;  
}

void new_msg() {
  //if (byte_count <= 3) goto bad_msg;
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


