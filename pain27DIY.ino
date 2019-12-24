#include <FastLED.h>
#include <Keyboard.h>

#define NUM_KEYS      27
#define NUM_LAYERS    3
#define DEBUG_KEYMAP  1

const char lch1 = (char)1, lch2 = (char)2, ech = (char)0;

int keys[NUM_KEYS];
int diodePins[3] = {7, 8, A0};
int switchPins[9] = {2, 4, 5, 6, 14, 15, A1, A2, A3};

//translates pin positions to array positions
const int keyMap[NUM_KEYS] = {13, 12, 11, 10, 14, 17, 8, 16, 15, 22,
                              21, 20, 19, 26, 25, 18, 24, 23, 3,
                              2, 1, 0, 4, 7, 9, 6,
                              5
                             };

//layers (ech is an empty character, lch1 is the key for layer one, lch2 for layer 2)
char lChar[NUM_LAYERS][NUM_KEYS] = {
  { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    'z', 'x', 'c', 'v', 'b', 'n', 'm',
    ' '
  },
  { '!', (char)KEY_UP_ARROW, '#', '$', '%', '*', '{', '}', '-', '=',
    (char)KEY_LEFT_ARROW, (char)KEY_DOWN_ARROW, (char)KEY_RIGHT_ARROW, '+', '[', ']', '(', ')', ';',
    '<', '>', '|', '/', ',', '.', '?',
    ech
  },
  { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    (char)KEY_TAB, '@', ech, ech, ech, ech, ech, (char)KEY_ESC, (char)KEY_DELETE,
    '&', '^', '_', ech, ech, ech, ech,
    ech
  }
};

//hold layer 
char hChar[NUM_LAYERS][NUM_KEYS] = {
  { ech, ech, ech, ech, ech, ech, ech, ech, ech, (char)KEY_BACKSPACE,
    (char)KEY_LEFT_SHIFT, ech, ech, ech, ech, ech, ech, (char)KEY_RETURN, (char)KEY_RIGHT_SHIFT,
    (char)KEY_LEFT_CTRL, (char)KEY_LEFT_GUI, ech, ech, ech, lch2, (char)KEY_RIGHT_CTRL,
    lch1
  },
  { ech, ech, ech, ech, ech, ech, ech, ech, ech, ech,
    ech, ech, ech, ech, ech, ech, ech, ech, ech,
    ech, ech, ech, ech, ech, lch2, ech,
    lch1
  },
  { ech, ech, ech, ech, ech, ech, ech, ech, ech, ech,
    ech, ech, ech, ech, ech, ech, ech, ech, ech,
    ech, ech, ech, ech, ech, lch2, ech,
    lch1
  }
};

int activeLayer = 0;
long lastLoopTime = 0;
long deltat;
boolean keyheld = false;

#define NUM_LEDS    9
CRGB leds[NUM_LEDS];
int longPressThreshold = 300;
int pressThreshold = 20;
int doubleTapThreshold = 300;

void setup() {
  FastLED.addLeds<WS2812B, 16, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  Serial.begin(115200);
  Keyboard.begin();
  for (int i = 0; i < 9; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);
  }
  for (int i = 0; i < 3; i++) {
    pinMode(diodePins[i], OUTPUT);
    digitalWrite(diodePins[i], LOW);
  }
  remapKeys();
}

void loop() {
  deltat = millis() - lastLoopTime;
  lastLoopTime = millis();
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(diodePins[i], LOW);
    digitalWrite(diodePins[(i + 1) % 3], HIGH);
    digitalWrite(diodePins[(i + 2) % 3], HIGH);
    
    for (int j = 0; j < 9; j++) {
      keyUpdate(i * 9 + j, j);
    }
  }

  //fills the underglow leds with a rainbow pattern 
  //and turns the spacebar leds red if long press is active
  fill_rainbow( leds, NUM_LEDS, millis() / 20, 10);
  if (keyheld) {
    leds[0] = CRGB(255, 0, 0);
    leds[1] = CRGB(255, 0, 0);
  } else {
    leds[0] = CRGB(0, 0, 0);
    leds[1] = CRGB(0, 0, 0);

  }
  FastLED.show();

  //debugPrint();
}

//key update saves the time every key has been pressed/released for with an upper limit of 2s
void keyUpdate(int k, int j) {
  bool newkeystate = !digitalRead(switchPins[j]);

  //key was already pressed, update hold
  if (newkeystate && keys[k] > 0)
  {
    keys[k] = constrain(keys[k] + deltat, 1, 2000);
    pressUpdate(k);
  }

  //key pressed this frame
  else if (newkeystate && keys[k] <= 0)
  {
    pressUpdate(k);
    keys[k] = deltat;
  }

  //keep key released
  else if (!newkeystate && keys[k] <= 0)
  {
    keys[k] = constrain(keys[k] - deltat, -2000, -1);;
    releaseUpdate(k);
  }

  //key was released
  else
  {
    releaseUpdate(k);
    keys[k] = 0;
  }


}

void pressUpdate(int k) {
  if (hChar[activeLayer][k] != ech)
  {
    if (keys[k] >= longPressThreshold) {
      if (pressFunctionKey(hChar[activeLayer][k], k)) {
        Keyboard.press(hChar[activeLayer][k]);
        keyheld = true;
      }
      keys[k] = 2000;
    }
  }
  else {
    //no hold functionality
    if (keys[k] >= pressThreshold)
      Keyboard.press(lChar[activeLayer][k]);
  }
}

void releaseUpdate (int k) {
  if (hChar[activeLayer][k] != ech)
  {
    if (keys[k] >= longPressThreshold) {
      if (releaseFunctionKey(hChar[activeLayer][k], k))
      {
        Keyboard.release(hChar[activeLayer][k]);
        keyheld = false;
      }
    } else if (keys[k] > 0) {
      Keyboard.write(lChar[activeLayer][k]);
    }
  }
  else {
    //no hold functionality
    if (keys[k] - deltat > pressThreshold)
      Keyboard.release(lChar[activeLayer][k]);
  }
}

bool pressFunctionKey(char c, int k) {
  if (c == lch1)
  {
    //change layer
    if (activeLayer == 0)
      switchLayer(1, k);
    return false;
  }
  if (c == lch2)
  {
    //change layer
    if (activeLayer == 0)
      switchLayer(2, k);
    return false;
  }
  return true;
}
bool releaseFunctionKey(char c, int k) {
  if (c == lch1)
  {
    //change layer
    if (activeLayer == 1)
      switchLayer(0, -1);
    return false;
  }
  if (c == lch2)
  {
    //change layer
    if (activeLayer == 2)
      switchLayer(0, -1);
    return false;
  }
  return true;
}

void switchLayer(int l, int k) {
  Keyboard.releaseAll();
  for (int i = 0; i < NUM_KEYS; i++)
    keys[i] = 0;
  if (k >= 0)
    keys[k] = 2000;
  activeLayer = l;
}

void remapKeys() {
  char tmpChar[NUM_LAYERS][NUM_KEYS];
  //copy
  for (int i = 0; i < NUM_LAYERS; i++) {
    for (int j = 0; j < NUM_KEYS; j++) {
      tmpChar[i][j] = lChar[i][j];
    }
  }
  for (int i = 0; i < NUM_LAYERS; i++) {
    for (int j = 0; j < NUM_KEYS; j++) {
      lChar[i][j] = tmpChar[i][keyMap[j]];
    }
  }

  //copy
  for (int i = 0; i < NUM_LAYERS; i++) {
    for (int j = 0; j < NUM_KEYS; j++) {
      tmpChar[i][j] = hChar[i][j];
    }
  }

  for (int i = 0; i < NUM_LAYERS; i++) {
    for (int j = 0; j < NUM_KEYS; j++) {
      hChar[i][j] = tmpChar[i][keyMap[j]];
    }
  }
}

//DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_DEBUG_

void debugPrint() {
  if (DEBUG_KEYMAP) {
    for (int i = 0; i < 10; i++)
      Serial.print(debugText(lChar[activeLayer][debugRevertKeymap(i)], debugRevertKeymap(i)));
    Serial.println(); Serial.print(" ");
    for (int i = 10; i < 19; i++)
      Serial.print(debugText(lChar[activeLayer][debugRevertKeymap(i)], debugRevertKeymap(i)));
    Serial.println(); Serial.print("  ");
    for (int i = 19; i < 26; i++)
      Serial.print(debugText(lChar[activeLayer][debugRevertKeymap(i)], debugRevertKeymap(i)));
    Serial.println();
    Serial.print("         ");
    Serial.print(debugText(lChar[activeLayer][debugRevertKeymap(26)], debugRevertKeymap(26)));
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    delay(100);
  }
  else {
    for (int i = 0; i < NUM_KEYS; i++) {
      Serial.print(keys[i] / 2000.0 + 0.2 + i);
      Serial.print(",");
    }
    Serial.print(activeLayer - 2);
    Serial.println();
  }

}

String debugText(char c, int i) {

  if (c == ech)
    return "ech";
  else if (c == lch1)
    return "l_1";
  else if (c == lch2)
    return "l_2";
  else if (keys[i] > 0) {
    String s = "(2)";
    s.setCharAt(1, c);
    return s;
  } else {
    String s = " 2 ";
    s.setCharAt(1, c);
    return s;
  }
}

//translates keymapped key indices back to the pin indices or something like that...
int debugRevertKeymap(int k) {
  int index = 0;
  for (int i = 0; i < NUM_KEYS; i++) {
    if (keyMap[i] == k)
      return i;
  }
  return index;
}
