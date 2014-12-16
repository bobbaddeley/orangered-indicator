//for the breathing of the LEDs, we do math.
#include "math.h"

//reddit username and password
char *username = "<YOURUSERNAME>";
char *password = "<YOURPASSWORD>";
//a custom user-agent because Reddit's API terms want one.
char *user_agent = "Spark Core IoT Orangered Indicator-1.0 by bobbaddeley";

//Debug mode dumps a bunch of stuff to the serial port
bool debug = false;

//how often do we check for updates
//currently every minute because we don't need to piss off Reddit
//and a minute is fine.
unsigned int frequency = 60;

TCPClient client;
unsigned int nextTime = 0;    // Next time to contact the server
//are we currently connected?
bool connected = false;
//the data that comes in from the server gets added to this string
String line = "";
//we have to send the cookie value with requests
String cookie = "";
//the output LED
int ledPin =A0; // Orange LEDs
int led2 = D7; //for debugging - built in LED
//location of the has_mail variable in the response json
int has_mail = 0;
bool mail_active = false;

void setup()
{
  //make the Spark status LED shut up.
  RGB.control(true); 
  RGB.color(0, 0, 0);
  //make the LED an output
  pinMode(ledPin, OUTPUT);
  pinMode(led2, OUTPUT);
  if (debug){
      // Make sure your Serial Terminal app is closed before powering your Core
      Serial.begin(38400);
      // Now open your Serial Terminal, and hit any key to continue!
      while(!Serial.available()) SPARK_WLAN_Loop();
      Serial.println("connecting...");
  }
  else {
    //for some reason, we can't immediately make a request, so we'll wait a bit for things to be ready
    delay(10000);
  }
  //connect to the server and log in.
  if (client.connect("reddit.com", 80))
  {
    connected = true;
    if (debug){Serial.println("connected");}
    client.print("POST /api/login?user=");
    client.print(username);
    client.print("&passwd=");
    client.print(password);
    client.println("&api_type=json&rem=true HTTP/1.1");
    client.println("Host: www.reddit.com");
    client.print("User-Agent: ");
    client.println(user_agent);
    client.println("Content-Type: text/plain;charset=UTF-8");
    client.println("Content-Length: 0");
    client.println();
  }
  else
  {
    //we didn't successfully connect to reddit. hmmm.
    connected = false;
    if (debug){Serial.println("connection failed");}
  }
    //we'll wait for 10 seconds before we make our first request for mail notification. This gives the login response some time.
    nextTime = millis() + 10000;
  
}

void loop()
{
    if (mail_active){
        //make the LED breathe. yoinked from http://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/
        float val = (exp(sin(millis()/2000.0*3.14159)) - 0.36787944)*108.0;
        analogWrite(ledPin, val);
        if (debug){analogWrite(led2,val);}
    }
    else {
        analogWrite(ledPin, 0);
    }
    //if we're connected to the reddit
     if (client.available())
    {
        //and there's data to be read
        char c = client.read();
        //Serial.print(c);
        //if it's the end of a line (headers) or greater than 300 long (because the response body doesn't end in a \n
        //but for /api/me.json is longer than 300 characters)
        if (c=='\n' || line.length()>300){
            //if we've got our cookie, then we're logged in.
            if (cookie.length()>0){
                //so the line might have a has_mail string
                has_mail = line.indexOf("has_mail");
                //if it does, let's see if we have mail.
                if (has_mail>0){
                    //if we do have a has_mail string, let's see if its value is true
                    if (line.substring(has_mail+11,has_mail+15)== "true"){
                        //I have mail!!! Show the LED
                        mail_active = true;
                        digitalWrite(led2, HIGH);
                        if (debug){Serial.println("Yay! You have mail!");}
                        
                    }
                    else{
                        //No mail. :( If the LED was on before, it should be off now.
                        mail_active = false;
                        digitalWrite(led2, LOW);
                        if (debug){Serial.println("Sorry, no mail");}
                    }
                }
                //Serial.println(line);
            }
            //this line has our login cookie! Save it!
            else if (line.indexOf("set-cookie: reddit_session=")==0){
                cookie = cookie+line.substring(line.indexOf("=")+1,line.indexOf(";")+1);
            }
            //clear out the line because we're done processing it.
            line = "";
        }
        //no specialness about this character. let's add it to the string.
        else {
            line = line + c;
        }
        
    }
    //we lost our connection. weird.
    if (connected && !client.connected())
    {
        connected = false;
        if (debug){Serial.println("disconnecting.");}
    }
    //if we're not at our frequency yet, then go back to the beginning of loop.
    //this way we can loop without making the request for new mail over and over.
    if (nextTime > millis()) {
        return;
    }
    //connect to reddit and request whether we have mail or not.
    if (client.connect("reddit.com", 80))
    {
        connected = true;
        if (debug){Serial.println("connected");}
        client.println("GET /api/me.json HTTP/1.1");
        client.println("Host: www.reddit.com");
        client.print("User-Agent: ");
        client.println(user_agent);
        client.println("Content-Type: text/plain;charset=UTF-8");
        client.print("Cookie: reddit_session=");
        client.println(cookie);
        client.println("Content-Length: 0");
        client.println();
    }
    else
    {
        connected = false;
        if (debug){Serial.println("connection failed");}
    }
    //and set up the next interval to be <frequency> seconds from now.
    nextTime = millis() + (frequency*1000);
}
