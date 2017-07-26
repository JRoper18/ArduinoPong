#include <gamma.h>
#include <RGBmatrixPanel.h>

#include <Adafruit_GFX.h>
#include <gfxfont.h>

#include "pitches.h"

#define player1InputPIN A8
#define player2InputPIN A10
#define player3InputPIN A12
#define player4InputPIN A14

#define segment1Reset 30
#define segment1Count 31
#define segment2Reset 32
#define segment2Count 33
#define segment3Reset 34
#define segment3Count 35
#define segment4Reset 36
#define segment4Count 37

#define startGameButtonPIN 42
#define fourPlayerSwitchPIN 43

#define speakerPIN 12

#define paddleSideOffset 1 //How far the paddles are from the sides of the board. DO NOT CHANGE TO ZERO!
#define boardSize 32 //Length of the board (in pixels)

#define paddleSpeed 1 //No unit
#define ballAccel .1 //How fast the ball accelerates. 
#define paddleSize 8 //Pixels
#define ballSpeed 0.1 //How fast is the ball when the round starts
#define startAngleOffset 40 //The offset of the angle which the ball initially launches from in a new round
#define paddleMoveError 1
#define fourBallSpeed 0.2

#define bounceDeviation 105 //How much the ball's angle changes randomly on a bounce (in degrees)

#define p1Color matrix.Color333(255, 0, 0)
#define p2Color matrix.Color333(255, 255, 0)
#define p3Color matrix.Color333(255, 0, 255)
#define p4Color matrix.Color333(0, 0, 255)
#define ballColor matrix.Color333(255, 255, 255)
#define wallColor matrix.Color333(0, 0, 255)
#define logoColor matrix.Color333(255, 255, 255)
#define offColor matrix.Color333(0, 0, 0)
#define pressStartColor matrix.Color333(255, 165, 0)

#define scoreDisplayTime 3000 //Time that the scores are shown (ms)

#define CLK 11  // MUST be on PORTB! (Use pin 11 on Mega)
#define OE  9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3
// If your matrix has the DOUBLE HEADER input, use:
//#define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
//#define LAT 9
//#define OE  10
//#define A   A3
//#define B   A2
//#define C   A1
//#define D   A0
RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

float ballX, ballY, ballMoveX, ballMoveY; //ballX and ballY represent the position of the ball from 0 to boardSize, and ballMoveX and ballMoveY are the momentum of the ball in both axises.  

float paddlePositions [4] = {}; //0 for all paddles by default. Positions range from 0 to boardSize. 

float lastPaddlePositions [4] = {};

int points [4] = {}; //Fills with zeros. 
uint16_t playerColors [4] = {p1Color, p2Color, p3Color, p4Color};

int lastHit = 0; //From 1 to 4, representing the last player to hit the ball. 0 is no one has hit it yet. 

boolean fourPlayer;

boolean playing = false;

const int oppositeOffset = boardSize - paddleSideOffset - 1;

int numPlayers;

boolean startButtonPressed = true;

void setup() {
  // put your setup code here, to run once:  

  Serial.begin(9600);
  pinMode(player1InputPIN, INPUT);
  pinMode(player2InputPIN, INPUT);
  pinMode(player3InputPIN, INPUT);
  pinMode(player4InputPIN, INPUT);
  pinMode(speakerPIN, OUTPUT);
  pinMode(startGameButtonPIN, INPUT_PULLUP);
  pinMode(fourPlayerSwitchPIN, INPUT);
  resetRound();
  fourPlayer = true; //Add input to this later. 
  numPlayers = (fourPlayer + 1) * 2;
  matrix.begin();
  loadStart();
}

void loop() {
  if(!digitalRead(startGameButtonPIN)){
    if(!startButtonPressed){
      startButtonPressed = true;
      clearBoard();
      if(!playing){
        noTone(speakerPIN);
        playing = true;
        resetRound();
        if(digitalRead(fourPlayerSwitchPIN)){
          fourPlayer = true;
        }
        else{
          fourPlayer = false;
        }
        renderStatic();
      }
      else{
        playing = false;
        resetPoints();
        loadStart();
      }
    }
  }
  else{
    startButtonPressed = false;
  }
  if(playing){
    playGame();
  }
}
void renderStatic(){ //Render the pixels that never change
  if(!fourPlayer){ //Render the walls
    matrix.fillRect(0, 0, boardSize, 1, wallColor);
    matrix.fillRect(0, boardSize-1, boardSize, 1, wallColor);
  }
}
void loadStart(){
  matrix.setCursor(4, 1);
  matrix.setTextSize(1);
  matrix.setTextColor(logoColor);
  matrix.print("PONG");
  matrix.setCursor(1, 16);
  matrix.setTextColor(pressStartColor);
  matrix.print("PRESSSTART");
}

void showScores(){
  clearBoard();
  matrix.setTextSize(1);
  for(int i = 0;i<4; i++){
    matrix.setCursor(0, (8*i) + 1);    
    matrix.setTextColor(playerColors[i]);
    matrix.print("P" + String(i+1) + ": " + points[i]);
  }
  delay(scoreDisplayTime);
  clearBoard();
}
void resetPoints(){
  digitalWrite(segment1Reset, HIGH);
  digitalWrite(segment2Reset, HIGH);
  digitalWrite(segment3Reset, HIGH);
  digitalWrite(segment4Reset, HIGH);
  points[0] = 0;
  points[1] = 0;
  points[2] = 0;
  points[3] = 0;
}
void playGame(){
  removeOldPositions();
  //Move paddles
  float inputDirections [4] = {};

  inputDirections[0] = map(analogRead(player1InputPIN), 0, 1023, 1, boardSize-paddleSize-1); 
  inputDirections[1] = map(analogRead(player2InputPIN), 0, 1023, 1, boardSize-paddleSize-1); 
  if(fourPlayer){
    inputDirections[2] = map(analogRead(player3InputPIN), 0, 1023, 1, boardSize-paddleSize-1); 
    inputDirections[3] = map(analogRead(player4InputPIN), 0, 1023, boardSize-paddleSize-1, 1); 
  }
  for(int i = 0; i<4; i++){
    float currentDir = inputDirections[i];
    if(abs(lastPaddlePositions[i] - currentDir)>=paddleMoveError){
      lastPaddlePositions[i] = paddlePositions[i];
      paddlePositions[i] = currentDir;
    }
    if(paddlePositions[i] <= 0){
      paddlePositions[i] = 0;
    }
    else if(paddlePositions[i] > boardSize){
      paddlePositions[i] = boardSize-1;
    }
  }
  
  
  //Render ball
  matrix.drawPixel(round(ballX), round(ballY), ballColor);
  
  //Render paddles
  const int paddle1Top = (paddlePositions[0] + paddleSize)-1;
  const int paddle2Top = (paddlePositions[1] + paddleSize)-1;
  const int paddle3Left = (paddlePositions[2] + paddleSize)-1;
  const int paddle4Left = (paddlePositions[3] + paddleSize)-1;
  
  matrix.fillRect(paddleSideOffset, paddlePositions[0], 1, paddleSize, p1Color);
  matrix.fillRect(oppositeOffset, paddlePositions[1], 1, paddleSize, p2Color);
  if(fourPlayer){
    matrix.fillRect(paddlePositions[2], paddleSideOffset, paddleSize, 1, p3Color);
    matrix.fillRect(paddlePositions[3], oppositeOffset, paddleSize, 1, p4Color);    
  }
  else{

  }
  //check for collisions
  if(round(ballX) == paddleSideOffset && (round(ballY) <= paddle1Top && round(ballY) >= paddlePositions[0])){ //It's in the left (P1) paddle.
    playHitSound();
    lastHit = 1;
    ballMoveX = -ballMoveX;
    ballX++;
    accelerateBall();
  }
  else if(round(ballX) == oppositeOffset && (round(ballY) <= paddle2Top && round(ballY) >= paddlePositions[1])){ //It's in the right (P2) paddle.
    playHitSound();
    lastHit = 2;
    ballMoveX = -ballMoveX;
    ballX--;
    accelerateBall();
  }  //Now check for score.
  else if(round(ballX) < paddleSideOffset || round(ballX) > oppositeOffset){ //It's behind P1 or P2's paddle.
    scorePoint();
  }
  else if(fourPlayer){
    if(round(ballY) > oppositeOffset || round(ballY) < paddleSideOffset){ //Above or below P3 or P4's paddle.
      scorePoint();
    }
    else if(round(ballY) == paddleSideOffset && (round(ballX) <= paddle3Left && round(ballX) >= paddlePositions[2])){ //In P3's paddle
      playHitSound();
      lastHit = 3;
      ballMoveY = -ballMoveY;
      ballY++;
      accelerateBall();
    }
    else if(round(ballY) == oppositeOffset && (round(ballX) <= paddle4Left && round(ballX) >= paddlePositions[3])){ //In P4's paddle
      playHitSound();
      lastHit = 4;
      ballMoveY = -ballMoveY;
      ballY--;
      accelerateBall();
    }
  }
  else{
    if(ballY >= oppositeOffset || ballY <= paddleSideOffset){ //Hitting the top or bottom. 
      ballMoveY = -ballMoveY;
    }
  }  

  //Move the ball
  ballX += ballMoveX;
  ballY += ballMoveY; 
}

void accelerateBall(){
  if(ballMoveX < 0){
     ballMoveX -= ballAccel;
  }
  else{
    ballMoveX += ballAccel;
  }
  if(ballMoveY < 0){
    ballMoveY -= ballAccel;
  }
  else{
    ballMoveY += ballAccel;  
  }
}
void deviateBall(){
  const float angleToChange = degToRad(random(-1 * bounceDeviation, bounceDeviation));
  const float hypo = sqrt(pow(ballMoveX, 2) + pow(ballMoveY, 2));
  const float currentAngle = acos(ballMoveX/hypo);
  float newAngle = currentAngle + angleToChange;
  for(int i = 0; i<numPlayers;i++){
    if(constrain(currentAngle, i * 2 * PI/numPlayers, (i+1)*2*PI/numPlayers) == currentAngle){
      newAngle = constrain(newAngle, i * 2 * PI/numPlayers, (i+1)*2*PI/numPlayers);
    }
  }
  ballMoveX += cos(newAngle)*hypo;
  ballMoveY += sin(newAngle)*hypo;
}
void scorePoint(){
  boolean finished = false;
  playScoreSound();
  if(lastHit != 0){
    points[lastHit-1]++;
    switch(lastHit){
      case 1:
        digitalWrite(segment1Count, HIGH);
        digitalWrite(segment1Count, LOW);
        break;  
      case 2:
        digitalWrite(segment2Count, HIGH);
        digitalWrite(segment2Count, LOW);
        break;
      case 3:
        digitalWrite(segment3Count, HIGH);
        digitalWrite(segment3Count, LOW);
        break;
      case 4:
        digitalWrite(segment4Count, HIGH);
        digitalWrite(segment4Count, LOW);
        break;
    }
    int recentScore = points[lastHit-1];
    if(recentScore > 9){
      winScreen(lastHit-1);
      playing = false;
      resetRound();
      loadStart();
      resetPoints();
      finished = true;
    }
  }
  if(!finished){
    showScores();
    resetRound();
    renderStatic();    
  }
}
void removeOldPositions(){
  //Remove the paddles
  matrix.fillRect(paddleSideOffset, 1, 1, boardSize-paddleSideOffset-1, offColor);
  matrix.fillRect(oppositeOffset, 1, 1, boardSize-paddleSideOffset-1, offColor);
  if(fourPlayer){
    matrix.fillRect(1, paddleSideOffset, boardSize-paddleSideOffset/2, 1, offColor);
    matrix.fillRect(1, oppositeOffset, boardSize-paddleSideOffset/2, 1, offColor);
  }
  //Remove ball
  const int lastBallX = round(ballX - ballMoveX);
  const int lastBallY = round(ballY - ballMoveY);
  matrix.drawPixel(lastBallX, lastBallY, offColor);
}
void resetRound(){
  removeOldPositions();
  lastHit = 0;
  matrix.drawPixel(ballX, ballY, offColor);
  ballX = boardSize/2;
  ballY = boardSize/2;
  const float angle = generateAngle(); //Generate angles
  if(fourPlayer){
    ballMoveX = cos(angle) * fourBallSpeed;
    ballMoveY = sin(angle) * fourBallSpeed; 
  }
  else{
    ballMoveX = cos(angle) * ballSpeed;
    ballMoveY = sin(angle) * ballSpeed;    
  }
}
float generateAngle(){
  int angle = random(0, 360);
  if(!fourPlayer){
    while((angle < 180-startAngleOffset && angle > startAngleOffset) || (angle > 180+startAngleOffset && angle < 360-startAngleOffset )){
      angle = random(0, 360);
    }
  }
  else{
    return random(0, 2*PI*100)/100;
  }
  return degToRad(angle);

}
void winMusic(){
  int melody[] = {
    NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_E5
  };
  int beats[] = {
    1, 1, 1, 3, 3, 3, 2, 1, 9
  };
  playMelody(melody, beats, 150, 9);
  
}
void winScreen(int winningPlayer){
  clearBoard();
  matrix.setCursor(10, 4);
  matrix.setTextColor(playerColors[winningPlayer]);
  matrix.print("P" + String(winningPlayer+1));
  matrix.setCursor(1, 16);
  matrix.print("WINS!");
  winMusic();
  clearBoard();
  
}
void playMelody(int melody[], int beats[], int tempo, int length){
  for(int i = 0; i<length; i++){
    int noteDuration = tempo*beats[i];
    tone(speakerPIN, melody[i], noteDuration);
    
    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(speakerPIN);
  }
}
void playHitSound(){
  tone(speakerPIN, 490, 90);
}
void playScoreSound(){
  tone(speakerPIN, 270, 48);
}
void clearBoard(){
  matrix.fillScreen(offColor); 
}

float degToRad(float deg){
  return (PI*deg)/180;
}



