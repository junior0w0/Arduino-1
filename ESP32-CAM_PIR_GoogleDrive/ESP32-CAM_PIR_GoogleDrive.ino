
// const char* ssid     = "Anik";    
// const char* password = "putra123";  
// const char* ssid     = "Kost Bu Sar";    
// const char* password = "jakalbawah";  
const char* ssid     = "visus";    
const char* password = "visus123";  
int pinPIR = 12;   
int pinLED = 33;
String myScript = "https://script.google.com/macros/s/AKfycbwsP3nwh8UAhAcuxhP9ni3LMXWeWI9Gf1GqLAS2lnHZzwd53DxXsKuxmg8YocqZBokC/exec";   

String myFoldername = "&myFoldername=ESP32-CAM";    
String myFilename = "&myFilename=ESP32-CAM.jpg";   
String myImage = "&myFile=";

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"  
#include "esp_camera.h"

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  
    
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  /*SETUP PIN*/
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  //
  // WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
  //            Ensure ESP32 Wrover Module or other board with PSRAM is selected
  //            Partial images will be transmitted if image exceeds buffer size
  //   
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.


  /* Mengecek apakah ada PSRAM, jika ada kualitas dan ukuran buffer memori akan meningkat. */
  if(psramFound()){  
    config.frame_size = FRAMESIZE_UXGA; // Mengatur ukuran frame ke UXGA.
    config.jpeg_quality = 15; // Menetapkan kualitas, kisarannya dari 0 sampai 63, semakin kecil nilai variabel, semakin baik kualitas foto.
    config.fb_count = 2; // Menetapkan jumlah buffer memori.
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  
  esp_err_t err = esp_camera_init(&config); // Membuat variabel untuk mengecek error.
  if (err != ESP_OK) // Apabila configurasi gagal, perangkat akan memulai ulang.
  {
    Serial.printf("Camera init failed with error 0x%x", err); 
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();// Membuat variabel pointer untuk mengakses konfigurasi sensor.
  
  if (s->id.PID == OV3660_PID) // Mengisikan model kamera.
  {
    s->set_vflip(s, 1); // Membalikkan gambar.
    s->set_brightness(s, 2); // Menaikkan tingkat kecerahan.
    s->set_saturation(s, -2); // Mengurangi saturasi.
  }

  s->set_framesize(s, FRAMESIZE_XGA);    


 
  ledcAttachPin(4, 4);  // Menautkan pin GPIO 4 ke Flash LED.
  ledcSetup(4, 5000, 8); // LED akan menyala dengan frekuensi 5000hz dengan resolusi 8 bit. LED akan mengedip dengan interval 0.2 detik dalam waktu 1 detik. Resolusi bit akan memengaruhi intensitas flash.
  
  WiFi.mode(WIFI_STA); // Menetapkan mode WiFi ke mode station, yaitu mode standard.
  
  for (int i=0;i<2;i++) {
    WiFi.begin(ssid, password);    // WiFi memulai untuk menjalin koneksi menggunakan SSID dan Password.
  
    delay(1000);
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    long int StartTime=millis(); // Menyimpan waktu mulai dalam millis.
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;    // Mengakhiri LOOP dalam 5 detik.
    } 
  
    if (WiFi.status() == WL_CONNECTED) {    
      Serial.println("");
      Serial.println("STAIP address: ");
      Serial.println(WiFi.localIP()); 
      Serial.println("");
  
      for (int i=0;i<5;i++) {  
        ledcWrite(4,1);
        delay(200);
        ledcWrite(4,0);
        delay(200);    
      }
  
      break;
    }
  } 

  if (WiFi.status() != WL_CONNECTED) // Memulai ulang perangkat ketika WiFi tidak terkoneksi.
   {  
    ESP.restart();  
  } 
          
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);

  SendCapturedImage2GoogleDrive(); // Memanggil metode untuk mengirim foto ke Google Drive.

  pinMode(pinPIR, INPUT);  // Mengatur pin PIR sebagai input.
  pinMode(pinLED, OUTPUT);  // Mengatur pin LED sebagai output.
}

void blink(int blink) {
  for (int i = 0; i < blink; i++) // Membuat perulangan untuk mengedipkan LED.
  {
    digitalWrite(pinLED, LOW); // LED mati.
    delay(50);
    digitalWrite(pinLED, HIGH); // LED menyala.
    delay(50);
  }
}

void loop()
{
  int v = digitalRead(pinPIR); // Membaca input dari PIR.
  Serial.println(v);
  if (v==1) // Jika gerakan terdeteksi, maka PIR akan mengirimkan sinyal HIGH yang nantinya akan terbaca sebagai nilai 1.
  {
    SendCapturedImage2GoogleDrive();
    //Serial.println(SendCapturedImage2GoogleDrive());  
    blink(10); // Mengedipkan LED sebanyak 10 kali.
    delay(5000); 
  }
  else
    delay(1000);
}


/* Metode mengirim gambar ke Drive */
String SendCapturedImage2GoogleDrive() {
  const char* myDomain = "script.google.com"; // Mengatur domain tujuan.
  String getAll="", getBody = ""; //
  
  camera_fb_t * fb = NULL; // Membuat pointer untuk mengakses memori frame buffer. Diberi nilai NULL karena nantinya akan digunakan sebagai parameter untuk mengecek apakah pengambilan gambar berhasil.
  fb = esp_camera_fb_get();  // Mengambil gambar.
  if(!fb) // Jika pointer bernilai NULL, maka kode di bawah akan dijalankan.
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart(); // Memulai ulang perangkat.
  }  
  
  Serial.println("Connect to " + String(myDomain)); 
  WiFiClientSecure client_tcp; // Membuat objek dari 
  client_tcp.setInsecure();   //run version 1.0.5 or above
  
  if (client_tcp.connect(myDomain, 443)) //Set IP domain yang digunakan dan Portnya.
  {
    Serial.println("Connection successful");
    
    char *input = (char *)fb->buf; // Membuat variabel pointer, kali ini untuk mengakses buffer.
    char output[base64_enc_len(3)]; // Membuat character
    String imageFile = "data:image/jpeg;base64,"; // Membuat string untuk data url sebagai source HTTPS.
    for (int i=0;i<fb->len;i++) // Melakukan perulangan sebanyak bit frame buffer.
    {
      base64_encode(output, (input++), 3); //Melakukan encoding. Variable output digunakan sebagai tempat untuk menyimpan hasil. Variable input sebagai sasaran encoding. 3 adalah panjang input buffer.
      if (i%3==0) imageFile += urlencode(String(output)); // Ketika mencapai perulangan kelipatan tiga, maka output akan di encoding lagi, kali ini encoding url.
    }
    String Data = myFoldername+myFilename+myImage; // Membuat string dimana nama folder, nama file, dan nama image digabungkan menjadi satu.
    

    /* REQUEST HTTPS */
    client_tcp.println("POST " + myScript + " HTTP/1.1"); //Melakukan aksi POST ke url google script
    client_tcp.println("Host: " + String(myDomain)); //Mengatur host sebagai google.script
    client_tcp.println("Content-Length: " + String(Data.length()+imageFile.length())); //Mengatur panjang sebagai ukuran data + ukuran file image.
    client_tcp.println("Content-Type: application/x-www-form-urlencoded"); //Mengatur tipe konten sebagai application/x-www-form-urlencoded, sehingga data menjadi key.
    client_tcp.println("Connection: keep-alive");//Mengatur koneksi untuk tetap terhubung.
    client_tcp.println();
    
    client_tcp.print(Data);//Mengirim value data.
    int Index;
    for (Index = 0; Index < imageFile.length(); Index = Index+1000) //Melakukan perulangan sampai index melebihi panjang byte image. Ditambah 1000 karena ukuran length masih dalam byte.
    {
      client_tcp.print(imageFile.substring(Index, Index+1000)); //Mengirim substring dari imagefile, dengan parameter index dan index+1000. substring(Index, Index+1000) akan me-return karakter pada string, dimulai pada indeks ke Index sampai indeks+1000.
    }
    esp_camera_fb_return(fb); // Mengosongkan frame buffer.
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTime = millis(); // Menambahkan millis ke startime.
    boolean state = false;
    
    while ((startTime + waitTime) > millis()) // Akan melakukan perulangan selama 10 detik.
     {
      Serial.print(".");
      delay(100);      
      while (client_tcp.available()) //Jika koneksi tersedia, maka perulangan akan berjalan.
      {
        char c = client_tcp.read(); //Membaca http body dari koneksi.
        if (state==true) getBody += String(c); //Memasukkan body ke variabel bila state sama dengan true.    
        if (c == '\n') //Jika body tidak ada.
        {
          if (getAll.length()==0) state=true; //Menjadikan variabel state sebagai true.
          getAll = ""; //Mengosongkan variabel.
        } else if (c != '\r')//Jika tidak berada di awal line.
          getAll += String(c);//Memasukkan nilai dari c ke getAll.
        startTime = millis();//Memasukkan value dari millis ke startTime.
      }
       if (getBody.length()>0) break;//Jika panjang body http sudah tidak kosong, maka perulangan dihentikan.
    }
    client_tcp.stop();//Menghentikan koneksi.
    Serial.println(getBody);
  }
  else //Mengirim pesan gagal apabila tidak terkoneksi
  {
    getBody="Connected to " + String(myDomain) + " failed.";
    Serial.println("Connected to " + String(myDomain) + " failed.");
  }
  return getBody;//mereturn value getBody
}


/*ENCODING URl sumber = https://github.com/zenmanenergy/ESP8266-Arduino-Examples */


String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
}
