#include <SPI.h>
#include <Arduino.h>
#include <HX711.h>
#include "Wire.h"


#define uchar unsigned char
#define uint unsigned int
 
//data array maxium length
#define MAX_LEN 16
 
/////////////////////////////////////////////////////////////////////
//set the pin
/////////////////////////////////////////////////////////////////////
const int chipSelectPin = 10;
//mega2560 use 53 port , uno use 10 port.
const int NRSTPD = 5;
int i=0;
 
//MF522 command bits
#define PCD_IDLE 0x00 //NO action; cancel current commands
#define PCD_AUTHENT 0x0E //verify password key
#define PCD_RECEIVE 0x08 //receive data
#define PCD_TRANSMIT 0x04 //send data
#define PCD_TRANSCEIVE 0x0C //send and receive data
#define PCD_RESETPHASE 0x0F //reset
#define PCD_CALCCRC 0x03 //CRC check and caculation
 
//Mifare_One card command bits
#define PICC_REQIDL 0x26 //Search the cards that not into sleep mode in the antenna area
#define PICC_REQALL 0x52 //Search all the cards in the antenna area
#define PICC_ANTICOLL 0x93 //prevent conflict
#define PICC_SElECTTAG 0x93 //select card
#define PICC_AUTHENT1A 0x60 //verify A password key
#define PICC_AUTHENT1B 0x61 //verify B password key
#define PICC_READ 0x30 //read
#define PICC_WRITE 0xA0 //write
#define PICC_DECREMENT 0xC0 //deduct value
#define PICC_INCREMENT 0xC1 //charge up value
#define PICC_RESTORE 0xC2 //Restore data into buffer
#define PICC_TRANSFER 0xB0 //Save data into buffer
#define PICC_HALT 0x50 //sleep mode
 
//THe mistake code that return when communicate with MF522
#define MI_OK 0
#define MI_NOTAGERR 1
#define MI_ERR 2
 
//------------------MFRC522 register ---------------
//Page 0:Command and Status
#define Reserved00 0x00
#define CommandReg 0x01
#define CommIEnReg 0x02
#define DivlEnReg 0x03
#define CommIrqReg 0x04
#define DivIrqReg 0x05
#define ErrorReg 0x06
#define Status1Reg 0x07
#define Status2Reg 0x08
#define FIFODataReg 0x09
#define FIFOLevelReg 0x0A
#define WaterLevelReg 0x0B
#define ControlReg 0x0C
#define BitFramingReg 0x0D
#define CollReg 0x0E
#define Reserved01 0x0F
//Page 1:Command
#define Reserved10 0x10
#define ModeReg 0x11
#define TxModeReg 0x12
#define RxModeReg 0x13
#define TxControlReg 0x14
#define TxAutoReg 0x15
#define TxSelReg 0x16
#define RxSelReg 0x17
#define RxThresholdReg 0x18
#define DemodReg 0x19
#define Reserved11 0x1A
#define Reserved12 0x1B
#define MifareReg 0x1C
#define Reserved13 0x1D
#define Reserved14 0x1E
#define SerialSpeedReg 0x1F
//Page 2:CFG
#define Reserved20 0x20
#define CRCResultRegM 0x21
#define CRCResultRegL 0x22
#define Reserved21 0x23
#define ModWidthReg 0x24
#define Reserved22 0x25
#define RFCfgReg 0x26
#define GsNReg 0x27
#define CWGsPReg 0x28
#define ModGsPReg 0x29
#define TModeReg 0x2A
#define TPrescalerReg 0x2B
#define TReloadRegH 0x2C
#define TReloadRegL 0x2D
#define TCounterValueRegH 0x2E
#define TCounterValueRegL 0x2F
//Page 3:TestRegister
#define Reserved30 0x30
#define TestSel1Reg 0x31
#define TestSel2Reg 0x32
#define TestPinEnReg 0x33
#define TestPinValueReg 0x34
#define TestBusReg 0x35
#define AutoTestReg 0x36
#define VersionReg 0x37
#define AnalogTestReg 0x38
#define TestDAC1Reg 0x39
#define TestDAC2Reg 0x3A
#define TestADCReg 0x3B
#define Reserved31 0x3C
#define Reserved32 0x3D
#define Reserved33 0x3E
#define Reserved34 0x3F
#define MemoryTimes 300 //可以記錄的次數
//-----------------------------------------------

 
//4 bytes Serial number of card, the 5 bytes is verfiy bytes
uchar serNum[5];
const int ledPin = 8; // pin to use for the LED 
bool a = false; 
bool b = false; //是不是第一次逼卡 或是第一次沒逼卡(只有在一開始的時後才會從true -> false)
//直到拿開卡 一拿開他就變false
const byte DS1307_I2C_ADDRESS = 0x68; // DS1307 (I2C) 地址
const byte NubberOfFields = 7; // DS1307 (I2C) 資料範圍
//byte m, d, w, h, mi, s; // 月/日/週/時/分/秒
//int y; // 年
enum timeUnit {s,mi,h,w,d,m,y};

int Weight = 0; //存重量
int Sec_Count = 0; //存時間
int Status = 1,Status_Pre = 1;
int Flag_Up = 0,Flag_Down = 0;
int CatComeTime[7] = {0}, CatNowTime[7] = {0};
int FoodComeWeight = 0, FoodNowWeight = 0;
int TimeCome[MemoryTimes]; //差值 memorytimes可以存的次數
int TimeDepart[MemoryTimes]; //離開 用來儲存用
int FoodCome[MemoryTimes]; 
int FoodDepart[MemoryTimes];
int TimesCount = 0; //用來計次數 貓每次來吃完後走就會+1一開始是0


/*
BLEPeripheral blePeripheral;  // BLE Peripheral Device (the board you're programming)
BLEService feederService("19B10000-E8F2-537E-4F6C-D104768A1214"); // BLE LED Service

// BLE LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic FeederWCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLEWrite);
BLECharacteristic FeederRCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 10);
 */


void setup()
{ 
    Serial.begin(9600);
    Serial.println("-------------------");
/*
    // set advertised local name and service UUID:
    blePeripheral.setLocalName("Feeder");
    blePeripheral.setAdvertisedServiceUuid(feederService.uuid());

    // add service and characteristic:
    blePeripheral.addAttribute(feederService);
    blePeripheral.addAttribute(FeederWCharacteristic);
    blePeripheral.addAttribute(FeederRCharacteristic);

    // set the initial value for the characeristic:

    // begin advertising BLE service:
    blePeripheral.begin();
*/
    delay(1000);

    Wire.begin();
    setTime(17,4,14,5,01,01,01); // 設定時間：2013 年 4 月 26 日 星期五 14 點 53 分 0 秒
 
    SPI.begin();
 
    /*pinMode(chipSelectPin,OUTPUT); // Set digital pin 10 as OUTPUT to connect it to the RFID /ENABLE pin
    digitalWrite(chipSelectPin, LOW); // Activate the RFID reader
    pinMode(NRSTPD,OUTPUT); // Set digital pin 5 , Not Reset and Power-down*/
 
    MFRC522_Init();

    TimesCount = 0;
    for (int i = 0;i < MemoryTimes; i++) { //設定變數的初始值，裡面就有寫我把他設成-1，不設0是因為我怕他delta有可能會是0這樣會誤判
        TimeCome[i] = -1;
        TimeDepart[i] = -1;
        FoodCome[i] = -1;
        FoodDepart[i] = -1;
    }

    Init_Hx711();        //初始化HX711模組連接的IO設置

    Serial.begin(9600);
    delay(7000);
    Serial.print("Welcome to use!\n");
    Get_Maopi();    //獲取毛皮
    delay(1000);
    Get_Maopi();    //獲取毛皮
    delay(3000);
    Serial.print("Start!\n");
}
 
void loop()
{
    uchar status;
    uchar str[MAX_LEN];

      // listen for BLE peripherals to connect:
 /* BLECentral central = blePeripheral.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {

      
        const unsigned char a[] = "Cat~~!!!!!";
        FeederRCharacteristic.setValue(a, 10);
        delay(3000);
      
     }
  }*/
    
    // Search card, return card types
    status = MFRC522_Request(PICC_REQIDL, str);
    if (status != MI_OK) { //沒逼到卡的時候因為剛剛已經把b 改成true了
        if ( b == true ) { //貓剛離開
            //然後就顯示重量
            Show_Weight(); 
            getTime(CatNowTime);
            digitalClockDisplay(CatNowTime);
            
            
            FoodNowWeight = Get_Weight();
            delay(10);
            Serial.println(stringTimeFormat(CatComeTime));
            TimeCome[TimesCount] = timeFormat(CatComeTime); //貓靠近的時間
            FoodCome[TimesCount] = FoodComeWeight;
            TimeDepart[TimesCount] = timeFormat(CatNowTime);
            FoodDepart[TimesCount] = FoodNowWeight;
            TimesCount++;
            ShowMemory(); //顯示出來 存了什麼 
        }
        b = false; //改成false，這樣之後就算是離開狀態他也不會作上面那些事情
        //Sec_Count++;
        delay(1000); //+1秒 
        return;
    }
 
    // Show card type
    //ShowCardType(str);
 
    //Prevent conflict, return the 4 bytes Serial number of the card
    status = MFRC522_Anticoll(str);
 
    // str[0..3]: serial number of the card
    // str[4]: XOR checksum of the SN.
    if (status == MI_OK) { //== OK成立是貓有逼到的狀況
        /*Serial.print("The card's number is: ");
        memcpy(serNum, str, 5);
        ShowCardID(serNum);
 
        // Check people associated with card ID
        uchar* id = serNum;
        if ( id[0]==0x4B && id[1]==0xE6 && id[2]==0xD1 && id[3]==0x3B ) {
            Serial.println("Hello Mary!");
        }
        else if (id[0]==0x3D && id[1]==0xB4 && id[2]==0xB7 && id[3]==0xD3) {
            Serial.println("Hello Greg!");
        }
        else {
            Serial.println("Hello unkown guy!");
        }*/

        /*Serial.print("a = ");
        Serial.println(a);*/
        if ( b == false ) { //貓剛進來 代表貓剛進來逼到第一次
            //就給他進來的時間跟進來的貓食重量
            Show_Weight();
            FoodComeWeight = Get_Weight();
            getTime(CatComeTime); 
            digitalClockDisplay(CatComeTime);
            
            delay(10);
            /*if (a == false) {
                a = true ;
                digitalWrite(8, HIGH); 
                delay(1);
            }
            else {
                a = false;
                digitalWrite(8, LOW);
                delay(1);
            }*/
        }
        b = true; //把b改成ture，下次如果還持續逼的話就只要顯示重量就好
    }
 
    MFRC522_Halt(); //command the card into sleep mode

    delay(1);

    //Sec_Count++;
    delay(1000);        //延時1s
}
int timeFormat(int *array){
    int t=0;
    for(int i=2;i>=0;--i){  
      t=t*100+array[i];
    }
    return t;
}
String stringTimeFormat(int *array)
{
    String t="";
    for(int i=6;i>=4;i--)
    {
          if(array[i]<9)
          {
              t = String(t+"0"+array[i]+"/");  
          }
          else
          {
              t = String(t+array[i]+"/");
          } 
    }
    for(int i=2;i>=0;--i)
    {
          if(array[i]<9)
          {
              t = String(t+"0"+array[i]);  
          }
          else
          {
              t = String(t+array[i]);
          }
          if(i>0)
          {
              t = String(t+":");
          }
    }  
    return t;
}
String stringWeight(int Weight)
{
    String stringWeight = String(Weight);
    for(int i = (4-String(Weight).length());i>0;i--)
    {
        stringWeight = String("0"+stringWeight);
    }
    return stringWeight;
}
void ShowMemory() { //看儲存陣列裡有沒有資料
    for (int i = 0; i < MemoryTimes ; i++) {
        if ( TimeCome[0] == -1 ) { //所以檢查第一個是不是-1，因為四個陣列一改就會四個一起改，所以只要檢查一個就可以了
          //如果是-1 代表裡面沒有存任何值就no data然後就不用檢查了 值接break已經知道裡面沒東西了
            Serial.println("No Data Store!");
            break;
        }
        else if ( TimeCome[i] != -1 ) { //裡面有東西把他輸出

            Serial.print("i = ");
            Serial.println(i);
            Serial.print("TimeCome = ");
            Serial.print(TimeCome[i]);
            Serial.print(", TimeDepart = ");
            Serial.print(TimeDepart[i]);
            Serial.print(", FoodCome = ");
            Serial.print(FoodCome[i]);
            Serial.print(", FoodDepart = ");
            Serial.print(FoodDepart[i]);
            Serial.print("\n");

            String Data = String(String(i)+","+stringTimeFormat(CatComeTime)+","+stringTimeFormat(CatNowTime)+","+stringWeight(FoodComeWeight)+","+stringWeight(FoodNowWeight));
            Serial.println(Data);
        }
        else { //這邊的else就是TimeCome[1]之後，只要有裡面值是-1就代表後面沒東西了，因為是一個一個存的然後就break不用檢查後面了
            break;
        }
    }
}

void Show_Weight() {
    Weight = Get_Weight();  //計算放在感測器上的重物重量
    
    delay(10);
    /*if(Weight <= 5) {
        Status = 0;       
    }
    else {
        Status = 1;   
    }*/
    //Status != Status_Pre就是狀態有改變的意思
    //now != pre現在狀態跟上一個狀態不一樣所以就是改變的意思
    //Status_Pre = Status;改變之後馬上把pre改成現在狀態下次的時候就是兩個都一樣了  
    if(Status != Status_Pre) { //Status == 0的話代表沒東西== 1 的話代表有放東西，pre就是上一個也就是說pre == 0  now == 1代表原本沒東西 現在有東西
        if(Status == 1 && Status_Pre == 0) {    //沒物變成有物
            Flag_Up = 1;
        }
        if(Status == 0 && Status_Pre == 1) {    //有物變成沒物
            Flag_Down = 1;
        }
        Status_Pre = Status;         
    }
    
    /*if(Flag_Up == 1) {    //沒物變成有物後秒數會歸0
        Flag_Up = 0;
        Sec_Count = 0;
    }*/
    
    if(Status == 1 || Flag_Down == 1) { //有東西的時候 或是 有物變沒物的時候就會測重顯示重量
        Flag_Down = 0;
//        if(Status == 1)
//            digitalClockDisplay();
//        else
//            digitalClockDisplay();
        Serial.print(Weight); //串口顯示重量
        Serial.print("g "); //顯示單位
        
        
        
    }
}
// BCD 轉 DEC
byte bcdTodec(byte val){
    return ((val / 16 * 10) + (val % 16));
}

// DEC 轉 BCD
byte decToBcd(byte val){
    return ((val / 10 * 16) + (val % 10));
}

// 設定時間
void setTime(byte y, byte m, byte d, byte w, byte h, byte mi, byte s){
    Wire.beginTransmission(DS1307_I2C_ADDRESS);
    Wire.write(0);
    Wire.write(decToBcd(s));
    Wire.write(decToBcd(mi));
    Wire.write(decToBcd(h));
    Wire.write(decToBcd(w));
    Wire.write(decToBcd(d));
    Wire.write(decToBcd(m));
    Wire.write(decToBcd(y));
    Wire.endTransmission();
}

// 取得時間
void getTime(int* array){
    Wire.beginTransmission(DS1307_I2C_ADDRESS);
    Wire.write(0);
    Wire.endTransmission();

    Wire.requestFrom(DS1307_I2C_ADDRESS, NubberOfFields);

    for(int i=0;i<7;++i){
      if(i==0 || i==2){
        array[i]=bcdTodec(Wire.read() & 0x7f);
      }
      else if(i==6){
        array[i]=bcdTodec(Wire.read()) + 2000;
      }
      else{
        array[i]=bcdTodec(Wire.read());
      }
    }

//    s = bcdTodec(Wire.read() & 0x7f);
//    mi = bcdTodec(Wire.read());
//    h = bcdTodec(Wire.read() & 0x7f);
//    w = bcdTodec(Wire.read());
//    d = bcdTodec(Wire.read());
//    m = bcdTodec(Wire.read());
//    y = bcdTodec(Wire.read()) + 2000;
}

// 顯示時間
void digitalClockDisplay(int* array){
    Serial.print(array[(timeUnit)y]);
    Serial.print("/");
    Serial.print(array[(timeUnit)m]);
    Serial.print("/");
    Serial.print(array[(timeUnit)d]);
    Serial.print(" ( ");
    Serial.print(array[(timeUnit)w]);
    Serial.print(" ) ");
    Serial.print(array[(timeUnit)h]);
    Serial.print(":");
    Serial.print(array[(timeUnit)mi]);
    Serial.print(":");
    Serial.println(array[(timeUnit)s]);
}
 
/*
 * Function：ShowCardID
 * Description：Show Card ID
 * Input parameter：ID string
 * Return：Null
 */
void ShowCardID(uchar *id)
{
    int IDlen=4;
    for(int i=0; i<IDlen; i++){
        Serial.print(0x0F & (id[i]>>4), HEX);
        Serial.print(0x0F & id[i],HEX);
    }
    Serial.println("");
}
 
/*
 * Function：ShowCardType
 * Description：Show Card type
 * Input parameter：Type string
 * Return：Null
 */
void ShowCardType(uchar* type)
{
    Serial.print("Card type: ");
    if(type[0]==0x04&&type[1]==0x00)
        Serial.println("MFOne-S50");
    else if(type[0]==0x02&&type[1]==0x00)
        Serial.println("MFOne-S70");
    else if(type[0]==0x44&&type[1]==0x00)
        Serial.println("MF-UltraLight");
    else if(type[0]==0x08&&type[1]==0x00)
        Serial.println("MF-Pro");
    else if(type[0]==0x44&&type[1]==0x03)
        Serial.println("MF Desire");
    else
        Serial.println("Unknown");
}
 
/*
 * Function：Write_MFRC5200
 * Description：write a byte data into one register of MR RC522
 * Input parameter：addr--register address；val--the value that need to write in
 * Return：Null
 */
void Write_MFRC522(uchar addr, uchar val)
{
    digitalWrite(chipSelectPin, LOW);
 
    //address format：0XXXXXX0
    SPI.transfer((addr<<1)&0x7E);
    SPI.transfer(val);
 
    digitalWrite(chipSelectPin, HIGH);
}
 
/*
 * Function：Read_MFRC522
 * Description：read a byte data into one register of MR RC522
 * Input parameter：addr--register address
 * Return：return the read value
 */
uchar Read_MFRC522(uchar addr)
{
    uchar val;
 
    digitalWrite(chipSelectPin, LOW);
 
    //address format：1XXXXXX0
    SPI.transfer(((addr<<1)&0x7E) | 0x80);
    val =SPI.transfer(0x00);
 
    digitalWrite(chipSelectPin, HIGH);
 
    return val;
}
 
/*
 * Function：SetBitMask
 * Description：set RC522 register bit
 * Input parameter：reg--register address;mask--value
 * Return：null
 */
void SetBitMask(uchar reg, uchar mask)
{
    uchar tmp;
    tmp = Read_MFRC522(reg);
    Write_MFRC522(reg, tmp | mask); // set bit mask
}
 
/*
 * Function：ClearBitMask
 * Description：clear RC522 register bit
 * Input parameter：reg--register address;mask--value
 * Return：null
 */
void ClearBitMask(uchar reg, uchar mask)
{
    uchar tmp;
    tmp = Read_MFRC522(reg);
    Write_MFRC522(reg, tmp & (~mask)); // clear bit mask
}
 
/*
 * Function：AntennaOn
 * Description：Turn on antenna, every time turn on or shut down antenna need at least 1ms delay
 * Input parameter：null
 * Return：null
 */
void AntennaOn(void)
{
    uchar temp;
 
    temp = Read_MFRC522(TxControlReg);
    if (!(temp & 0x03))
    {
        SetBitMask(TxControlReg, 0x03);
    }
}
 
/*
 * Function：AntennaOff
 * Description：Turn off antenna, every time turn on or shut down antenna need at least 1ms delay
 * Input parameter：null
 * Return：null
 */
void AntennaOff(void)
{
    ClearBitMask(TxControlReg, 0x03);
}
 
/*
 * Function：ResetMFRC522
 * Description： reset RC522
 * Input parameter：null
 * Return：null
 */
void MFRC522_Reset(void)
{
    Write_MFRC522(CommandReg, PCD_RESETPHASE);
}
 
/*
 * Function：InitMFRC522
 * Description：initilize RC522
 * Input parameter：null
 * Return：null
 */
void MFRC522_Init(void)
{
    digitalWrite(NRSTPD,HIGH);
 
    MFRC522_Reset();
 
    //Timer: TPrescaler*TreloadVal/6.78MHz = 24ms
    Write_MFRC522(TModeReg, 0x8D); //Tauto=1; f(Timer) = 6.78MHz/TPreScaler
    Write_MFRC522(TPrescalerReg, 0x3E); //TModeReg[3..0] + TPrescalerReg
    Write_MFRC522(TReloadRegL, 30);
    Write_MFRC522(TReloadRegH, 0);
 
    Write_MFRC522(TxAutoReg, 0x40); //100%ASK
    Write_MFRC522(ModeReg, 0x3D); //CRC initilizate value 0x6363 ???
 
    //ClearBitMask(Status2Reg, 0x08); //MFCrypto1On=0
    //Write_MFRC522(RxSelReg, 0x86); //RxWait = RxSelReg[5..0]
    //Write_MFRC522(RFCfgReg, 0x7F); //RxGain = 48dB
 
    AntennaOn(); //turn on antenna
}
 
/*
 * Function：MFRC522_Request
 * Description：Searching card, read card type
 * Input parameter：reqMode--search methods，
 * TagType--return card types
 * 0x4400 = Mifare_UltraLight
 * 0x0400 = Mifare_One(S50)
 * 0x0200 = Mifare_One(S70)
 * 0x0800 = Mifare_Pro(X)
 * 0x4403 = Mifare_DESFire
 * return：return MI_OK if successed
 */
uchar MFRC522_Request(uchar reqMode, uchar *TagType)
{
    uchar status;
    uint backBits; //the data bits that received
 
    Write_MFRC522(BitFramingReg, 0x07); //TxLastBists = BitFramingReg[2..0] ???
 
    TagType[0] = reqMode;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);
 
    if ((status != MI_OK) || (backBits != 0x10))
    {
        status = MI_ERR;
    }
 
    return status;
}
 
/*
 * Function：MFRC522_ToCard
 * Description：communicate between RC522 and ISO14443
 * Input parameter：command--MF522 command bits
 * sendData--send data to card via rc522
 * sendLen--send data length
 * backData--the return data from card
 * backLen--the length of return data
 * return：return MI_OK if successed
 */
uchar MFRC522_ToCard(uchar command, uchar *sendData, uchar sendLen, uchar *backData, uint *backLen)
{
    uchar status = MI_ERR;
    uchar irqEn = 0x00;
    uchar waitIRq = 0x00;
    uchar lastBits;
    uchar n;
    uint i;
 
    switch (command)
    {
        case PCD_AUTHENT: //verify card password
        {
            irqEn = 0x12;
            waitIRq = 0x10;
            break;
        }
        case PCD_TRANSCEIVE: //send data in the FIFO
        {
            irqEn = 0x77;
            waitIRq = 0x30;
            break;
        }
        default:
            break;
    }
 
    Write_MFRC522(CommIEnReg, irqEn|0x80); //Allow interruption
    ClearBitMask(CommIrqReg, 0x80); //Clear all the interrupt bits
    SetBitMask(FIFOLevelReg, 0x80); //FlushBuffer=1, FIFO initilizate
 
    Write_MFRC522(CommandReg, PCD_IDLE); //NO action;cancel current command ???
 
    //write data into FIFO
    for (i=0; i<sendLen; i++)
    {
        Write_MFRC522(FIFODataReg, sendData[i]);
    }
 
    //procceed it
    Write_MFRC522(CommandReg, command);
    if (command == PCD_TRANSCEIVE)
    {
        SetBitMask(BitFramingReg, 0x80); //StartSend=1,transmission of data starts
    }
 
    //waite receive data is finished
    i = 2000; //i should adjust according the clock, the maxium the waiting time should be 25 ms???
    do
    {
        //CommIrqReg[7..0]
        //Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
        n = Read_MFRC522(CommIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x01) && !(n&waitIRq));
 
    ClearBitMask(BitFramingReg, 0x80); //StartSend=0
 
    if (i != 0)
    {
        if(!(Read_MFRC522(ErrorReg) & 0x1B)) //BufferOvfl Collerr CRCErr ProtecolErr
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {
                status = MI_NOTAGERR; //??
            }
 
            if (command == PCD_TRANSCEIVE)
            {
                n = Read_MFRC522(FIFOLevelReg);
                lastBits = Read_MFRC522(ControlReg) & 0x07;
                if (lastBits)
                {
                    *backLen = (n-1)*8 + lastBits;
                }
                else
                {
                    *backLen = n*8;
                }
 
                if (n == 0)
                {
                    n = 1;
                }
                if (n > MAX_LEN)
                {
                    n = MAX_LEN;
                }
 
                //read the data from FIFO
                for (i=0; i<n; i++)
                {
                    backData[i] = Read_MFRC522(FIFODataReg);
                }
            }
        }
        else
        {
            status = MI_ERR;
        }
 
    }
 
    //SetBitMask(ControlReg,0x80); //timer stops
    //Write_MFRC522(CommandReg, PCD_IDLE);
 
    return status;
}
 
/*
 * Function：MFRC522_Anticoll
 * Description：Prevent conflict, read the card serial number
 * Input parameter：serNum--return the 4 bytes card serial number, the 5th byte is recheck byte
 * return：return MI_OK if successed
 */
uchar MFRC522_Anticoll(uchar *serNum)
{
    uchar status;
    uchar i;
    uchar serNumCheck=0;
    uint unLen;
 
    //ClearBitMask(Status2Reg, 0x08); //strSensclear
    //ClearBitMask(CollReg,0x80); //ValuesAfterColl
    Write_MFRC522(BitFramingReg, 0x00); //TxLastBists = BitFramingReg[2..0]
 
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);
 
    if (status == MI_OK)
    {
        //Verify card serial number
        for (i=0; i<4; i++)
        {
            serNumCheck ^= serNum[i];
        }
        if (serNumCheck != serNum[i])
        {
            status = MI_ERR;
        }
    }
 
    //SetBitMask(CollReg, 0x80); //ValuesAfterColl=1
 
    return status;
}
 
/*
 * Function：CalulateCRC
 * Description：Use MF522 to caculate CRC
 * Input parameter：pIndata--the CRC data need to be read，len--data length，pOutData-- the caculated result of CRC
 * return：Null
 */
void CalulateCRC(uchar *pIndata, uchar len, uchar *pOutData)
{
    uchar i, n;
 
    ClearBitMask(DivIrqReg, 0x04); //CRCIrq = 0
    SetBitMask(FIFOLevelReg, 0x80); //Clear FIFO pointer
    //Write_MFRC522(CommandReg, PCD_IDLE);
 
    //Write data into FIFO
    for (i=0; i<len; i++)
    {
        Write_MFRC522(FIFODataReg, *(pIndata+i));
    }
    Write_MFRC522(CommandReg, PCD_CALCCRC);
 
    //waite CRC caculation to finish
    i = 0xFF;
    do
    {
        n = Read_MFRC522(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04)); //CRCIrq = 1
 
    //read CRC caculation result
    pOutData[0] = Read_MFRC522(CRCResultRegL);
    pOutData[1] = Read_MFRC522(CRCResultRegM);
}
 
/*
 * Function：MFRC522_Write
 * Description：write block data
 * Input parameters：blockAddr--block address;writeData--Write 16 bytes data into block
 * return：return MI_OK if successed
 */
uchar MFRC522_Write(uchar blockAddr, uchar *writeData)
{
    uchar status;
    uint recvBits;
    uchar i;
    uchar buff[18];
 
    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    CalulateCRC(buff, 2, &buff[2]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);
 
    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }
 
    if (status == MI_OK)
    {
        for (i=0; i<16; i++) //Write 16 bytes data into FIFO
        {
            buff[i] = *(writeData+i);
        }
        CalulateCRC(buff, 16, &buff[16]);
        status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);
 
        if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }
 
    return status;
}
 
/*
 * Function：MFRC522_Halt
 * Description：Command the cards into sleep mode
 * Input parameters：null
 * return：null
 */
void MFRC522_Halt(void)
{
    uchar status;
    uint unLen;
    uchar buff[4];
 
    buff[0] = PICC_HALT;
    buff[1] = 0;
    CalulateCRC(buff, 2, &buff[2]);
 
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff,&unLen);
}
