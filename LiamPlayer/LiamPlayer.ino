// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
#include <IRrecv.h>
#include <IRutils.h>

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
uint8_t volume = 10;
int lastRemoteVal = 0;

void setup() {
  Serial.begin(115200);

  Serial.println("\n\nAdafruit VS1053 Feather Test");
  
  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }

  Serial.println(F("VS1053 found"));
 
  musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working
  
  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  Serial.println("SD OK!");
  
  // list files
  printDirectory(SD.open("/"), 0);
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(volume,volume);

  // If DREQ is on an interrupt pin we can do background
  // audio playing
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

  irrecv.enableIRIn(); // Start the receiver
  Serial.println("Playing one Track");
}

File currentTrack;
boolean playing = false;
uint8_t mp3buffer[32];

void startPlayingFile(const char* trackName) {
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

void loop() {
  decode_results results;

 //if(digitalRead(VS1053_DREQ) && !musicPlayer.stopped() && !isPaused) {
    feedBuffer();
  //} 

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
    
    if (results.value == 0xFD08F7) {
       stopPlaying();
       Serial.println("playing track #1");
       startPlayingFile("001cstf.mp3");
    }
    if (results.value == 0xFD8877) {
       stopPlaying();
       Serial.println("playing track #2");
       startPlayingFile("002wnsd.mp3");
    }
    if (results.value == 0xFD48B7) {
       stopPlaying();
       Serial.println("playing track #3");
       startPlayingFile("003faf.mp3");
    }
    if (results.value == 0xFD28D7) {
       stopPlaying();
       Serial.println("playing track #4");
       startPlayingFile("004gb.mp3");
    }
    if (results.value == 0xFDA857) {
       stopPlaying();
       Serial.println("playing track #5");
       startPlayingFile("005shoes.mp3");
    }
    if (results.value == 0xFD6897) {
       stopPlaying();
       Serial.println("playing track #6");
       startPlayingFile("006rss.mp3");
    }
    if (results.value == 0xFD689) {
       stopPlaying();
       Serial.println("playing track #7");
       startPlayingFile("007beats.mp3");
    }

    if (results.value == 0xFD40BF) { //vol+
      Serial.println("Vol+");
      if (volume > 0) {
         volume--;
         musicPlayer.setVolume(volume,volume);
      }
    }
    if (results.value == 0xFD00FF) { //vol-
      Serial.println("Vol-");
      if (volume < 100) {
         volume++;
         musicPlayer.setVolume(volume,volume);
      }
    }

    if (results.value == 0xFD807F) { // playpause
      Serial.println("Play/Pause");
      isPaused = !isPaused; // toggle!
    } 
  }

  delay(1);
}


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
     Serial.print(entry.name());
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


