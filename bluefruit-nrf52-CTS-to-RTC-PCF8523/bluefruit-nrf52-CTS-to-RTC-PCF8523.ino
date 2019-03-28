/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* RTC code is from the Adafruit arduino 1.2.0 fork of the Jeelabs RTCLib.
 *  
 *  This sketch demonstrates Setting the RTC PCF8523 from the CTS time, 
 *  aquired from the CTS BLE service.  This is for the RTC used in the 
 *  Adalogger FeatherWing. The client Current Time Service uses the
 * BLEClientCts API(). After uploading, go to iOS setting and connect
 * to Bluefruit52, and then press PAIR.
 * 
 * Note: Currently only iOS act as a CTS server, Android does not. The
 * easiest way to test this sketch is using an iOS device.
 * 
 * Current Time Service info:
 *   https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.current_time.xml
 *   https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.current_time.xml
 *   https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.local_time_information.xml
 */

#include <bluefruit.h>
#include <Wire.h>
#include "RTClib.h"

// BLE Client Current Time Service
BLEClientCts  bleCTime;

// Realtime Clock 
RTC_PCF8523 rtc;

void setup()
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.initialized()) {
    Serial.println("RTC is NOT running!");
    // The following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

    //Uncomment the line below to set the RTC time to an old date,
  // before querying the BLE Current Time Service, so you can tell
  // if the RTC was synced properly.  This is useful when you have
  // already set the RTC to the current time and are testing the 
  // CTS sync process.
  //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  Serial.println("Bluefruit52 BLE Client Current Time Example");
  Serial.println("-------------------------------------------\n");

  Serial.println("Go to iOS's Bluetooth settings and connect to Bluefruit52");
  Serial.println("It may appear up as 'Accessory' depending on your iOS version.");

  // Config the peripheral connection with maximum bandwidth
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Bluefruit52");
  Bluefruit.setConnectCallback(connect_callback);
  Bluefruit.setDisconnectCallback(disconnect_callback);

  // Configure CTS client
  bleCTime.begin();

  // Callback invoked when iOS device time changes
  // To test this go to Setting -> Date & Time -> Toggle Time Zone "Set Automatically"
  // Or change the time manually etc ...
  bleCTime.setAdjustCallback(cts_adjust_callback);

  // Set up and start advertising
  startAdv();
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_GENERIC_CLOCK);

  // Include CTS client UUID
  Bluefruit.Advertising.addService(bleCTime);

  // Includes name
  Bluefruit.Advertising.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void loop()
{
  //Print the RTC time before the bleCTime is checked
  rtcPrintTime();
  
  // If CTS service is not yet discovered
  if ( !bleCTime.discovered() && !Bluefruit.connPaired() ) return;

  // Get Time from iOS once per second
  // Note it is not advised to update this quickly
  // Application should use local clock and update time after 
  // a long period (e.g an hour or day)
  bleCTime.getCurrentTime();

  printTime();

  delay(1000);
}

void connect_callback(uint16_t conn_handle)
{
  Serial.println("Connected");
  
  Serial.print("Discovering CTS ... ");
  if ( bleCTime.discover(conn_handle) )
  {
    Serial.println("Discovered");
    
    // iOS requires pairing to work, it makes sense to request security here as well
    Serial.print("Attempting to PAIR with the iOS device, please press PAIR on your phone ... ");
    if ( Bluefruit.requestPairing() )
    {
      Serial.println("Done");
      Serial.println("Enabling Time Adjust Notify");
      bleCTime.enableAdjust();

      Serial.println("Get Current Time chars value");
      bleCTime.getCurrentTime();

      Serial.println("Get Local Time Info chars value");
      bleCTime.getLocalTimeInfo();
      Serial.println();
      delay(1000);
      syncRTCtoCTS();

      Serial.println();
    }

    Serial.println();
  }
}

void cts_adjust_callback(uint8_t reason)
{
  const char * reason_str[] = { "Manual", "External Reference", "Change of Time Zone", "Change of DST" };

  Serial.println("iOS Device time changed due to ");
  Serial.println( reason_str[reason] );
}

void printTime(void)
{
  const char * day_of_week_str[] = { "n/a", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
  
  Serial.printf("%04d-%02d-%02d ", bleCTime.Time.year, bleCTime.Time.month, bleCTime.Time.day);
  Serial.printf("%02d:%02d:%02d ", bleCTime.Time.hour, bleCTime.Time.minute, bleCTime.Time.second);
  Serial.print(day_of_week_str[bleCTime.Time.weekday]);
  
  int utc_offset =  bleCTime.LocalInfo.timezone*15; // in 15 minutes unit
  Serial.printf(" (UTC %+d:%02d, ", utc_offset/60, utc_offset%60);
  Serial.printf("DST %+.1f)", ((float) bleCTime.LocalInfo.dst_offset*15)/60 );
  Serial.println();
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 * https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/cores/nRF5/nordic/softdevice/s140_nrf52_6.1.1_API/include/ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) reason;

  Serial.println();
  Serial.println("Disconnected");
}

void syncRTCtoCTS()
{
   Serial.println("Syncing RTC from CTS...");
   
   // This line sets the RTC with the date/time from the Current Time Service:
    rtc.adjust(DateTime(bleCTime.Time.year, bleCTime.Time.month, bleCTime.Time.day, bleCTime.Time.hour, bleCTime.Time.minute, bleCTime.Time.second));
}

void rtcPrintTime()
{
    DateTime now = rtc.now();
    Serial.print("                                                  RTC   ");                                    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
     Serial.print("  ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    
    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
}
