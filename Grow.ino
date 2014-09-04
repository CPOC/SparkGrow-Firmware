// This #include statement was automatically added by the Spark IDE.
//#include "idDHT22/idDHT22.h"


uint32_t lastReset = 0; // last known reset time

int pinFan = A4;
int pinFanSpeed = 30;
int pinFanMaxSpeed = 255;
int pinFanMinSpeed = 30;

int pinMoisture = A0;
int pinMoisture2 = A1;

int waterRelay = D0;

int pinCoreLED = D7;

int pinLightRelay = D2;
int pinSolenoid = D5;

int LightsOff = 255;
int LightsOn = 0;

uint32_t ms = 0; // timer storage bucket

int interval = 5000; // POST every x milliseconds

// Cycle Variables
bool Running = false;
int Cycle = 0;
int Hours = 0;
int Days = 0;
int min = 0;
bool On = false;
bool waterOn = false;

int GestationDays = 5;
int GrowthDays = 7;
int FlowerDays = 65;

int update_interval = 60000;

int wateringOffset;            //Used as a timer offset length of time to water, This setting is adjusted by the Moister sensors
int waterLength = 4;             // How long to water for in Minuets, defined by moisture sensor starts at 1 minuet
int wateringFrequency = 12;       // Water when hour time counter equals this value, Only when lights On
int waterHour;            		// system adjusted value to water on Hours after day break
int waterHourset = 2;           // day light Hour to start watering,  2 setting will trigger 2 hours after daylight.
int waterMinElapCount = 1;    // counts the number of minuets elapsed
int moistCurrent1;            // stores the current moisture reading,  Read every minuet
int moistCurrent2;            // stores the current moisture2 reading,  Read every minuet
int moisture;                 // adverage moisture of all the sensors




void setup() {

    Spark.function("cycle", r_cycle);
    Spark.function("fan", fan);
    Spark.function("lights", lights);

    pinMode(pinLightRelay, OUTPUT);
    
    pinMode(pinCoreLED, OUTPUT);
    
    pinMode(pinFan, OUTPUT);

    pinMode(waterRelay, OUTPUT);
    
    analogWrite(pinFan, pinFanSpeed);
    
    // Need to add code to startup_cycle if errors connecting the first time
    startup_cycle();
}


void loop() {
    long x = millis() - ms;

    // Everybody 60 seconds...
    if ( ( x == 0 || x > update_interval ) && Running == true )
    {
        ++min; 
 
        bool update_server = false;

        if ( min == 60 ) { ++Hours; min = 0; update_server = true; } 
        if ( Hours == 24 ) { ++Days; Hours = 0; }   
        if ( begin_cycle() == 0 ) {
            //post_cycle("active"); 
            }
        
        // if ( update_server == true ) update_server_cycle(); // Disable update on the hour since we update the server every minute
        
        ms = millis();		
    }   

	
	// Every 1 Minuet events
	if ( oneMinElapsed() ) {
		soil_Moisture();
		cycle_Watering();

    } 

// Every 1 Hour events
    if ( oneHourElapsed() ) {
	post_cycle("active");
    }
}

// ********** CYCLE ******************************************************************************************************************
void startup_cycle()
{
    if ( read_cycle() ) { 
			Running = true; 
			post("started","", true);
			begin_cycle(); 
			}
}

int begin_cycle()
{
    if ( Cycle == 1 ) return cycle_Gestation();
    if ( Cycle == 2 ) return cycle_Growth();
    if ( Cycle == 3 ) return cycle_Flower();
    
    return 0;
}

void end_cycle()
{
    Running = false;
    Cycle = 0;
    Days = 0;
    Hours = 0;
    min = 0;
    
    lights_Off();

    update_server_cycle();
    
    post_cycle("completed");
}

int cycle_Gestation ()
{
    if ( Days >= GestationDays ) { Days = 0; min = 0; Cycle = 2; post_cycle("evolution"); cycle_Growth(); return 1; }
    
    lights_On();
    
    return 0;
}

int cycle_Growth()
{
    if ( Days >= GrowthDays ) { Days = 0; min = 0; Cycle = 3; post_cycle("evolution"); cycle_Flower(); return 1; }
    
    if ( Hours < 18 ) return lights_On();
    else return lights_Off();
}

int cycle_Flower()
{
    if ( Days >= FlowerDays ) { Days = 0; min = 0; Cycle = 0; end_cycle(); return 1; }
    
    if ( Hours < 12 ) return lights_On();
    else return lights_Off();

}

void verify_cycle()
{

    if ( Running == false ) return; // Don't update/correct cycle if we aren't running!
    
//    if ( Days > ( GrowthDays + GestationDays ) ) Cycle = 3;
//    else if ( Days > GestationDays ) Cycle = 2;
//    else if ( Days >= 0 && Days < GestationDays ) Cycle = 1;

}

int lights_On ()
{
    int r = 0;
    
    pinFanSpeed = pinFanMaxSpeed;
    
    analogWrite(pinFan, pinFanSpeed);
    digitalWrite(pinLightRelay, HIGH);

	waterHour = waterHourset;
    
    if ( On == false ) { On = true; post_cycle("daytime"); r = 1; }


	
    return r;
	
}

int lights_Off ()
{
    int r = 0;
    
    pinFanSpeed = pinFanMinSpeed;
    
    analogWrite(pinFan, pinFanSpeed);
    digitalWrite(pinLightRelay, LOW);
    
    if ( On == true ) { On = false; post_cycle("nighttime"); r = 1; }
		
    return r;
}


void soil_Moisture(){
	moistCurrent1 = read_moisture();
	moistCurrent2 = read_moisture2();
    moisture = moistCurrent2 + moistCurrent1;
	moisture /= 2;
}


void cycle_Watering(){
	if ( On == true ){                  // Lights on,  Watering only happens during the day,  Maybe an option for night later
		if ( Hours == waterHour){				// Water on if the Hour = the start Water hour in the day time.  0 will water when lights turn on
				
				if(waterMinElapCount <= waterLength){   // Counter for every minuet,  Need to water plants
					++waterMinElapCount;

					if ( waterOn == false ) { 
						waterLength += wateringOffset;	   // Off set watering length based on soil moisture conten		
						post_cycle("Watering Cycle On"); 
						digitalWrite(waterRelay, HIGH); 
						waterOn = true; 
						post_cycle("Watering"); 
					}	
				}else if(waterMinElapCount > waterLength){   // Watering Minuet counter has pass the watering length time to turn off
					if( waterOn == true) {
						post_cycle("Watering Cycle off"); 
						digitalWrite(waterRelay, LOW);    // Trigger physical relay to turn off Pump
						waterOn = false; 				  // set system watering status variable to off
						post_cycle("Watering Done"); 	  // Post to online data base watering is complete	
						
						waterHour += wateringFrequency;    // Allows for multiple watering each day based on the hour of the days

					}
				}
				
		}else if( Hours != waterHour){
			//This area handles variables that only happens POST watering cycle.  Reset counters
			waterMinElapCount = 1;            // reset counter after watering Hour is over			
			
/*
          // The below IF's handle the Mositure sensor readings and timing offsets
		    if ( moisture > 4000 && moisture < 5000)        {	wateringOffset = 0; waterLength = 0;} // Don't water
			else if ( moisture > 3500 && moisture <= 4000 ) {	wateringOffset = 0; waterLength = 1;}   
			else if ( moisture > 3000 && moisture <= 3500 ) {	wateringOffset = 1;}      
			else if ( moisture > 2500 && moisture <= 3000 ) {	wateringOffset = 2; post_cycle("Low Moist");}    
			else if ( moisture > 2000 && moisture <= 2500 ) {	wateringOffset = 3; post_cycle("Very Low Moist");}
			else if ( moisture >= 0 && moisture <= 2000 )   {	wateringOffset = 4; post_cycle("Danger Moist");}
*/

			if( waterOn == true) {
				post_cycle("Watering Cycle off"); 
				digitalWrite(waterRelay, LOW);    // Trigger physical relay to turn off Pump
				waterOn = false; 				  // set system watering status variable to off
				post_cycle("Watering Done"); 	  // Post to online data base watering is complete	
				}
			
			
			}
	}	
}



// ********** EXPOSED ROUTINES *******************************************************************************************************
int fan(String args) {
    int R = parse_args_int(args, "speed");

    if ( R != -1 ) {
        R = constrain(R, 0, 255);

        pinFanSpeed = R;

        analogWrite(pinFan, pinFanSpeed);

        post_cycle("post-fan:" + String(pinFanSpeed));
    }
    
    return R;
}

// #cycle
int r_cycle(String args) {
    int r = 0;
    
    // Reset takes priority!
    if ( parse_args_int(args, "reset") == 1 )
    {
        post_cycle("post-reset");

        end_cycle();

        return 1;
    }

    if ( parse_args_int(args, "debug") == 1 )
    {
		post_debug();

        return 1;
    }
	
    if ( parse_args_int(args, "post") == 1 )
    {
        post_cycle("post-cycle");
        
        int cycle = parse_args_int(args, "cycle");
        if ( cycle != -1 ) Cycle = constrain(cycle, 0, 3);
        
        int mins = parse_args_int(args, "mins");
        if ( mins != -1 ) min = constrain(mins, 0, 60);

        int hours = parse_args_int(args, "hours");
        if ( hours != -1 ) Hours = constrain(hours, 0, 24);
        
        int days = parse_args_int(args, "days");
        if ( days != -1 ) Days = constrain(days, 0, 365);

        // If we are not running but we have passed cycle info then we should begin
        if ( Running == false && Cycle != 0 ) { Running = true; verify_cycle(); post_cycle("started"); update_server_cycle(); }
        
        begin_cycle();

        r = 1;
    }
    else if ( parse_args_int(args, "active") == 1 )
    {
        if ( !Cycle ) Cycle = 1;

        update_server_cycle();

        startup_cycle();
        post_cycle("post-activate");
    }
    else 
    {
        post_cycle("post-request");
    }

    return r;
}

int lights(String args) {
    int R = parse_args_int(args, "on");

    if ( R == 1 ) {
        digitalWrite(pinLightRelay, HIGH);
        On = true;
    }
    else {
        digitalWrite(pinLightRelay, LOW);
        R = 0;
        On = false;
    }
    
    return R;
}



// ********** INTERNET PROTOCOL ******************************************************************************************************
void update_server_cycle()
// Update to server to write latest cycle information to the server cycle file
{
    post("update","", true);
}

void post_cycle(String rn)
// Program information updates on status changes
{

    post("program",rn, false);
}

String append_nv_pair( String n, String v )
{
	return "&" + n + "=" + v;
}

void post_debug()
{
    String append = "action=debug&deviceid=" + getCoreID();
	

	append += append_nv_pair( "lastReset", String(lastReset) );
	
	append += append_nv_pair( "pinFan", String(pinFan) );
	
	append += append_nv_pair( "pinFanSpeed", String(pinFanSpeed) );
	append += append_nv_pair( "pinFanMaxSpeed", String(pinFanMaxSpeed) );
	append += append_nv_pair( "pinFanMinSpeed", String(pinFanMinSpeed) );
	
	append += append_nv_pair( "pinMoisture", String(pinMoisture) );
	append += append_nv_pair( "pinMoisture2", String(pinMoisture2) );
	
	append += append_nv_pair( "waterRelay", String(waterRelay) );
	
	append += append_nv_pair( "pinCoreLED", String(pinCoreLED) );
	
	append += append_nv_pair( "pinLightRelay", String(pinLightRelay) );
	append += append_nv_pair( "pinSolenoid", String(pinSolenoid) );

	append += append_nv_pair( "LightsOff", String(LightsOff) );
	append += append_nv_pair( "LightsOn", String(LightsOn) ); 

	append += append_nv_pair( "ms", String(ms) ); 

	append += append_nv_pair( "interval", String(interval) ); 

	append += append_nv_pair( "Running", String(Running) ); 
	
	append += append_nv_pair( "Cycle", String(Cycle) ); 
	append += append_nv_pair( "Hours", String(Hours) ); 
	append += append_nv_pair( "Days", String(Days) ); 
	append += append_nv_pair( "min", String(min) ); 

	append += append_nv_pair( "On", String(On) ); 
	append += append_nv_pair( "waterOn", String(waterOn) ); 
	
	append += append_nv_pair( "GestationDays", String(GestationDays) ); 
	append += append_nv_pair( "GrowthDays", String(GrowthDays) ); 
	append += append_nv_pair( "FlowerDays", String(FlowerDays) ); 
	
	append += append_nv_pair( "update_interval", String(update_interval) ); 

	append += append_nv_pair( "wateringOffset", String(wateringOffset) ); 
	append += append_nv_pair( "waterLength", String(waterLength) ); 
	append += append_nv_pair( "wateringFrequency", String(wateringFrequency) ); 
	append += append_nv_pair( "waterHour", String(waterHour) ); 
	append += append_nv_pair( "waterHourset", String(waterHourset) ); 
	append += append_nv_pair( "waterMinElapCount", String(waterMinElapCount) ); 
	append += append_nv_pair( "moistCurrent1", String(moistCurrent1) ); 
	append += append_nv_pair( "moistCurrent2", String(moistCurrent2) ); 
	
	post_string(append);
}

void post(String action, String rn, bool limited)
{
    String append = "&deviceid=" + getCoreID() + "&running=" + String(Running) + "&cycle=" + String(Cycle) + "&days=" + String(Days) + "&hours=" + String(Hours) + "&mins=" + String(min);
// Need a debug variable write to server, Easy way to add new variables.    
    //if ( limited == false) append += "&temp=" + String(read_temperature(true)) + "&humid=" + String(read_humidity()) + "&moist=" + String(read_moisture());
    append += "&temp=" + String(read_temperature()) + "&humid=" + String(read_humidity()) + "&moist=" + String(moisture) ;
    append += "&fan=" + String(pinFanSpeed) + "&lights=" + String(On);
    
    if ( limited == true ) post_string("action=" + action + append);
    else post_string("action=" + action + "&status=" + rn + append);
}





int read_cycle()
{
    int r = 0;
    
    digitalWrite(pinCoreLED, HIGH);
    if ( connectToMyServer() == 1 )
    {

	String client_read;

        client.println("GET /post.pl?action=getcycle&device_id=" + getCoreID() + " HTTP/1.1");
        client.println("Host: spark.rynovation.com");
        client.println("User-Agent: SparkCore/0.0");
        client.println("Connection: close");
        client.println();

        client.flush();

        // wait 7 seconds or less for the server to respond
        uint32_t startTime = millis();
        while( !client.available() && (millis() - startTime) < 7000 ); 

        bool begin = false;

        while ( client.available() )
        {
            // Buffer every character read in from socket
            client_read += String ( (char)client.read() );
          
            // Reset buffer and begin reading content after HTTP header
            if ( !begin && client_read.endsWith("\r\n\r\n") ) { client_read = ""; begin = true; }
		}
        Cycle = constrain( client_read.substring(0,3).toInt(), 0, 3);
        Days = constrain( client_read.substring(3,6).toInt(), 0, 365);
        Hours = constrain( client_read.substring(6,9).toInt(), 0, 24);
        min = constrain( client_read.substring(9,12).toInt(), 0, 60);

      digitalWrite(pinCoreLED, LOW);
      delay(1000);
      digitalWrite(pinCoreLED, HIGH);
      delay(1000);
      digitalWrite(pinCoreLED, LOW);
	  
       if ( Cycle >= 1 && Cycle <= 3 ) Running = true;
        verify_cycle();
       if ( client_read != "" && Running == true  ) r=1;

	}else{
        digitalWrite(pinCoreLED, LOW);
        delay(200);
        digitalWrite(pinCoreLED, HIGH);
        delay(200);
		digitalWrite(pinCoreLED, LOW);

    }
    
    client.stop();
    digitalWrite(pinCoreLED, LOW);
    
    if ( !r ) post_cycle("inactive");
    return r;
}

void post_string(String content)
{

    digitalWrite(pinCoreLED, HIGH);
    if ( connectToMyServer() == 1 )
    {
        client.println("POST /post.pl HTTP/1.1");
        client.println("Host: spark.rynovation.com");
        client.println("User-Agent: SparkCore/0.0");
        client.println("Connection: close");
        client.println("Content-Type: application/x-www-form-urlencoded;");
        client.print("Content-Length: ");
        client.println(content.length());
        client.println();
        client.println(content);

        delay(50);
    }
    else
    {
    	digitalWrite(pinCoreLED, LOW);
        delay(200);
        digitalWrite(pinCoreLED, HIGH);
        delay(200);
    }
    
    client.stop();
    digitalWrite(pinCoreLED, LOW);
}


int connectToMyServer() {

  delay(5000);   // Delay added for new firmware crap, need to remvoe in the future
  
  if (client.connect("spark.rynovation.com", 80)) {
      digitalWrite(pinCoreLED, LOW);
      delay(1000);
      digitalWrite(pinCoreLED, HIGH);
      delay(1000);
	  digitalWrite(pinCoreLED, LOW);
  return 1; // successfully connected
  } else {
      digitalWrite(pinCoreLED, LOW);
      delay(200);
      digitalWrite(pinCoreLED, HIGH);
      delay(200);
      digitalWrite(pinCoreLED, LOW);
  return -1; // failed to connect
  }
  
}

int parse_args_int(String &args, String q) {
    if ( !args.length() || !q.length() || args.length() < q.length() ) return -1;
    
    if ( q.substring(q.length() - 1, 1) != ":" ) q = q + ":";
    int ql = q.length();

    int r1 = args.indexOf(q);
    if ( r1 != -1 ) {
        int r2 = args.indexOf(",", r1+ql);
        if ( r2 == -1 ) r2 = args.length();
        return args.substring(r1+ql,r2).toInt();
    }
    
    return -1;
}

String getCoreID()
{

  String coreIdentifier = "";
  char id[12];
  memcpy(id, (char *)ID1, 12);
  char hex_digit;
  for (int i = 0; i < 12; ++i)
  {
    hex_digit = 48 + (id[i] >> 4);
    if (57 < hex_digit)
     hex_digit += 39;
     coreIdentifier = coreIdentifier + hex_digit;
    hex_digit = 48 + (id[i] & 0xf);
   if (57 < hex_digit)
     hex_digit += 39;
   coreIdentifier = coreIdentifier + hex_digit;
 }
 return coreIdentifier;
}

uint8_t oneHourElapsed() {
  if( (millis()-lastReset) > (60*60*1000) ) {
    lastReset = millis();
    return 1; 
  } else {
    return 0; 
  }
}

uint8_t oneMinElapsed() {
  if( (millis()-lastReset) > (60*1000) ) {
    lastReset = millis();
    return 1; 
  } else {
    return 0; 
  }
}
// ********** READ SENSORS ***********************************************************************************************************

float read_temperature(){
return 1;
//return 	DHT22.getCelsius();
}


float read_humidity() {
return 1;
//return DHT22.getHumidity();
}

int read_moisture() {

   int smoothing_total = 0;
   int moist_avg = 0;
    
		for(int i=0; i<100; i++) {
			smoothing_total += analogRead(pinMoisture);
			delay(1);
		}

    moist_avg = smoothing_total / 100;

    return moist_avg;
}

int read_moisture2() {

   int smoothing_total = 0;
   int moist_avg = 0;
    
		for(int i=0; i<100; i++) {
			smoothing_total += analogRead(pinMoisture2);
			delay(1);
		}

    moist_avg = smoothing_total / 100;

    return moist_avg;
}

