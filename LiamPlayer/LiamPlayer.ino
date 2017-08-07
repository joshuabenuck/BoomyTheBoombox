// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SdFat.h>
SdFat SD;
#include <Adafruit_VS1053.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

int RECV_PIN = 4; //an IR detector/demodulatord is connected to GPIO pin 2
IRrecv irrecv(RECV_PIN);

// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)
#define VS1053_CS      16     // VS1053 chip select pin (output)
#define VS1053_DCS     15     // VS1053 Data/command select pin (output)
#define CARDCS          2     // Card chip select pin
#define VS1053_DREQ     0     // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

// the name of what we're going to play
char foundname[20];
boolean isPaused = false;
uint8_t volume = 0;
int lastRemoteVal = 0;

char broker_ip[20];
char ssid[32];
char password[32];

WiFiServer server(80);
WiFiClient client;
PubSubClient mqtt(client);

File currentTrack;
boolean playing = false;
uint8_t mp3buffer[32];

void setup() {
  memset(broker_ip, 0, sizeof(broker_ip));
  memset(ssid, 0, sizeof(ssid));
  memset(ssid, 0, sizeof(password));
  Serial.begin(115200);

  Serial.println("\n\nAdafruit VS1053 Feather Test");
  
  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }

  Serial.println(F("VS1053 found"));
 
  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  Serial.println("SD OK!");
  
  //writeWiFiCredentials("ssid", "password");
  //writeBrokerIP("ip address here");
  loadWiFiCredentials();
  loadBrokerIP();
  //writeButtonMappings();
  loadButtonMappings();
    
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(volume,volume);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  mqtt.setServer(broker_ip, 1883);
  mqtt.setCallback(handle_mqtt_message);

  // If DREQ is on an interrupt pin we can do background
  // audio playing
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

  musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working
  
  irrecv.enableIRIn(); // Start the receiver
  server.begin();
}

void loop() {
  decode_results results;

 //if(digitalRead(VS1053_DREQ) && !musicPlayer.stopped() && !isPaused) {
    feedBuffer();
  //} 
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();

  WiFiClient client = server.available();
  if(client) {
    while(!client.available()) {
      delay(1);
    }
    String req = client.readStringUntil('\r');
    Serial.println(req);
    client.flush();
  }
    
  // look for a message!
  if (irrecv.decode(&results)) {
    //Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value

    // handle repeat codes!
    if (results.value == 0xFFFFFFFF) {
      // only for vol+ or vol-
      if ( (lastRemoteVal == 0xFD40BF) || (lastRemoteVal == 0xFD00FF))
         results.value = lastRemoteVal;
    } else {
      lastRemoteVal = results.value;
    }

    Serial.println((uint32_t)(results.value), HEX);

    if (results.value == 0xFD08F7) { // 1
      press1();
    }
    if (results.value == 0xFD8877) { // 2
      press2();
    }
    if (results.value == 0xFD48B7) { // 3
      press3();
    }
    if (results.value == 0xFD28D7) { // 4
      press4();
    }
    if (results.value == 0xFDA857) { // 5
      press5();
    }
    if (results.value == 0xFD6897) { // 6
      press6();
    }
    if (results.value == 0xFD18E7) { // 7
      press7();
    }
    if (results.value == 0xFD9867) { // 8
      press8();
    }
    if (results.value == 0xFD58A7) { // 9
      press9();
    }
    if (results.value == 0xFD30CF) { // 0/10+
      press0();
    }

    if (results.value == 0xFD40BF) { // vol+
      Serial.println("Vol+");
      volumeUp();
    }
    if (results.value == 0xFD00FF) { // vol-
      Serial.println("Vol-");
      volumeDown();
    }

    if (results.value == 0xFD807F) { // playpause
      Serial.println("Play/Pause");
      isPaused = !isPaused; // toggle!
    } 
  }

  delay(1);
}

// Buttons

// TODO: Store in a file on the SD card.
char track0[250] = "";
char track1[250] = ""; //"bingo/001bingo.mp3";
char track2[250] = ""; //"orig/001cstf.mp3";
char track3[250] = ""; //"orig/003faf.mp3";
char track4[250] = ""; //"orig/004gb.mp3";
char track5[250] = ""; //"orig/005shoes.mp3";
char track6[250] = ""; //"orig/006rss.mp3";
char track7[250] = ""; //"orig/007beats.mp3";
char track8[250] = "";
char track9[250] = "";
char* tracks[] = { track0, track1, track2, track3, track4, track5, track6, track7, track8, track9 };

void playTrack(int index) {
  if (strlen(tracks[index]) == 0) return;
  stopPlaying();
  startPlayingFile(tracks[index]);
}

// TODO: Support something other than playing a track when a button is pressed.
// e.g. playlists, internet radio streams, custom callbacks, etc.
void press1() { playTrack(1); }

void press2() { playTrack(2); }

void press3() { playTrack(3); }

void press4() { playTrack(4); }

void press5() { playTrack(5); }

void press6() { playTrack(6); }

void press7() { playTrack(7); }

void press8() { playTrack(8); }

void press9() { playTrack(9); }

void press0() {
  printDirectory(SD.open("/"), 0);
  Serial.println(WiFi.localIP());
}

// Settings

void writeWiFiCredentials(const char* ssid_text, const char* password_text) {
  File ssid = SD.open("/ssid", FILE_WRITE);
  ssid.truncate(0);
  ssid.write(ssid_text, strlen(ssid_text));
  ssid.close();

  File password = SD.open("/password", FILE_WRITE);
  password.truncate(0);
  password.write(password_text, strlen(password_text));
  password.close();
}

void loadWiFiCredentials() {
  File ssid_file = SD.open("/ssid");
  ssid_file.read(ssid, sizeof(ssid));
  ssid_file.close();

  File password_file = SD.open("/password");
  password_file.read(password, sizeof(password));
  password_file.close();
}

void loadBrokerIP() {
  File broker_file = SD.open("/broker");
  broker_file.read(broker_ip, sizeof(broker_ip));
  broker_file.close();
}

void writeBrokerIP(const char* broker_text) {
  File broker = SD.open("/broker", FILE_WRITE);
  broker.truncate(0);
  broker.write(broker_text, strlen(broker_text));
  broker.close();
}

void writeButtonMappings() {
  File mappings = SD.open("/mappings", FILE_WRITE);
  mappings.truncate(0);
  Serial.println("file opened");
  for (int i = 0; i < sizeof(tracks)/sizeof(tracks[0]); i++) {
    Serial.println(i, DEC);
    Serial.println(tracks[i]);
    mappings.write(tracks[i]);
    mappings.write("\n");
  }
  mappings.close();
}

void loadButtonMappings() {
  Serial.println("loading mappings...");
  File mappings = SD.open("/mappings");
  Serial.println("mappings opened...");
  int i = 0;
  int j = 0;
  while(mappings.available()) {
    char c = mappings.read();
    Serial.print(c);
    if (c == '\n') {
      tracks[i][j] = 0;
      i++;
      j = 0;
      continue;
    }
    tracks[i][j] = c;
    j++;
  }
  mappings.close();  
}

// MQTT

void handle_mqtt_message(char* topic, byte* payload, unsigned int length) {
  // Can this go beyond the end of the buffer?
  payload[length] = '\0';
  for(int i=0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
  int size = sizeof("/play/")-1;
  if (strncmp((char*)payload, "/play/", size) == 0) {
    startPlayingFile((char*)&(payload[size]));
  }
  if (strncmp((char*)payload, "/stop", length) == 0) {
    stopPlaying();
  }
  if (strncmp((char*)payload, "/list", length) == 0) {
    list(SD.open("/"), 0);
  }
  size = sizeof("/list/")-1;
  if (strncmp((char*)payload, "/list/", size) == 0) {
    list(SD.open((char*)&(payload[size-1])), 0);
  }
  size = sizeof("/press/")-1;
  if (strncmp((char*)payload, "/press/", size) == 0) {
    char* key = (char*)&(payload[size]);
    Serial.print("key pressed: ");
    Serial.println(*key);
    if (*key == '1') { press1(); }
    if (*key == '2') { press2(); }
    if (*key == '3') { press3(); }
    if (*key == '4') { press4(); }
    if (*key == '5') { press5(); }
    if (*key == '6') { press6(); }
    if (*key == '7') { press7(); }
    if (*key == '8') { press8(); }
    if (*key == '9') { press9(); }
    if (*key == '0') { press0(); }
  }
  if (strncmp((char*)payload, "/vol+", length) == 0) {
    volumeUp();
  }  
  if (strncmp((char*)payload, "/vol-", length) == 0) {
    volumeDown();
  }  
  if (strncmp((char*)payload, "/volmax", length) == 0) {
    volumeSet(0);
  }
  if (strncmp((char*)payload, "/tracks", length) == 0) {
    char buffer[sizeof(tracks[0]) + 5];
    for (int i = 0; i < sizeof(tracks)/sizeof(tracks[0]); i++) {
      sprintf(buffer, "%2d %s", i, tracks[i]);
      mqtt.publish("tracks", buffer);
    }
  }
  if (strncmp((char*)payload, "/tracks/save", length) == 0) {
    writeButtonMappings();
  }
  size = sizeof("/track/")-1;
  if (strncmp((char*)payload, "/track/", size) == 0) {
    int i = size;
    int track = 0;
    while(payload[i] != '/' && i < length) {
      if (payload[i] < '0' || payload[i] > '9') {
        // TODO: Better error messages...
        mqtt.publish("track", F("Integer track number expected."));
        return;
      }
      track=track*10 + (payload[i]-'0');
      i++;
    }
    if (track >= 10) {
      mqtt.publish("track", F("Track out of range."));
      return;
    }
    if (i == length) {
      mqtt.publish("track", tracks[track]);
      return;
    }
    i++; // skip slash
    // TODO: Validate file exists.
    strncpy(tracks[track], (char*)&payload[i], length-i+1);
    // TODO: Track dirty state?
    //mappingsDirty = true;
  }
}

void reconnect() {
  while (!client.connected()) {
    if (mqtt.connect("boomy")) {
      mqtt.subscribe("boomy");
      return;
    }
    Serial.print("Connection failure=");
    Serial.print(mqtt.state());
    delay(1);
  }
}

// MP3

void volumeSet(int target) {
  musicPlayer.setVolume(target, target);
}

void volumeUp() {
  if (volume > 0) {
     volume--;
     volumeSet(volume);
  }
}

void volumeDown() {
  if (volume < 100) {
     volume++;
     volumeSet(volume);
  }
}

void startPlayingFile(const char* trackName) {
  Serial.print("Playing ");
  Serial.println(trackName);
  currentTrack = SD.open(trackName);
  playing = true;
  //if (!currentTrack) {
  //    playing = false;
  //}
}

void stopPlaying() {
  currentTrack.close();
  playing = false;
  isPaused = false;
}

void feedBuffer() {
  if (!playing || isPaused) return;
  if(musicPlayer.readyForData()) {
    int bytesRead = currentTrack.read(mp3buffer, 32);
    if (bytesRead == 0) {
      currentTrack.close();
      playing = false;
      return;
    }
    musicPlayer.playData(mp3buffer, bytesRead);
  }
}

// SD Card

boolean findFileStartingWith(char *start) {
  File root;
  root = SD.open("/");
  root.rewindDirectory();
  while (true) {
    File entry =  root.openNextFile();
    if (! entry) {
      return false;
    }
    String filename = entry.name();
    Serial.print(filename);
    if (entry.isDirectory()) {
      Serial.println("/");
    } else {
      Serial.println();
      if (filename.startsWith(start)) {
        filename.toCharArray(foundname, 20); 
        entry.close();
        root.close();
        return true;
      }
    }
    entry.close();
  }
}


/// File listing helper
void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     entry.printName(&Serial);
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}

void list(File dir, int numTabs) {
   char buffer[256];
   while(true) {
     char *bptr = buffer;
     memset(buffer, 0, sizeof(buffer));
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       *bptr = '\t';
       bptr++;
     }
     entry.getName(bptr, 255-(bptr-&buffer[0]));
     while(*bptr != 0) bptr++;
     if (entry.isDirectory()) {
       *bptr='/';
       bptr++;
       mqtt.publish("mp3s", buffer);
       list(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       *bptr='\t';
       bptr++;
       *bptr='\t';
       bptr++;
       sprintf(bptr, "%d", entry.size());
       while(*bptr != 0) bptr++;
       mqtt.publish("mp3s", buffer);
     }
     entry.close();
   }
}

