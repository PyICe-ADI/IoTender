/*
 * IoTender
 * Steve Martin 11/22/2017
 */
#include <stdint.h>
#include "LTC4162-LAD.h"
#include "LTC4162-LAD_formats.h"
#include "LTC4162-LAD_pec.h"
#include <Wire.h>
#include <ESP8266WiFi.h>
extern "C"
{
    #include "user_interface.h"
}
#define STOP true
#define RESTART                                     // Send empty argument rather than 0?
#define SCL D1
#define SDA D2
#define BULK D5
#define EQUALIZE D6
#define FLOAT D7
#define ABSORB D8
#define DEEP_SLEEP_SECONDS *1e6                     // Deep Sleep Timer in us, maximum 1 hour 11 minutes 34.967296 seconds (32 bits)
#define TIMER_SECONDS *1e3                          // Other Timers in ms
#define TIMER_MINUTES TIMER_SECONDS * 60
#define DEEP_SLEEP_TIME 15 DEEP_SLEEP_SECONDS       // Extend to 30 seconds
#define SOLAR_CHECK_TIMEOUT 5 TIMER_MINUTES         // Extend to 5 minutes
#define VIN_SOLAR_DROPOUT 0.98

uint16_t data, cell_count;
bool solar_panel, solar_panel_timeout, input_power_detected, thermistor_present;
os_timer_t solar_panel_timer;
char vin[10], vout[10], vbat[10], iin[10], ibat[10], die_temp[10], bsr[10], thermistor_voltage[10], tcharge_timer[15], tcv_timer[15], temp[15];
std::string value, html, charger_state;
WiFiClient client;
WiFiServer server(80); //Initialize the server on Port 80
int read_register(uint8_t addr, uint8_t command_code, uint16_t *data, struct port_configuration *pc);
int write_register(uint8_t addr, uint8_t command_code, uint16_t data, struct port_configuration *pc);
int add_table_row(std::string x, std::string y, bool send_it);

LTC4162_chip_cfg_t ltc4162 =
{
    .address            = LTC4162_ADDR_68,
    .read_register      = read_register,
    .write_register     = write_register,
    .port_configuration = NULL
};

void timerCallback(void *pArg)
{
    solar_panel_timeout = true;
}

bool input_power_present()
{
    LTC4162_read_register(&ltc4162, LTC4162_VIN_GT_VBAT, &data);
    return data;
}

bool telemetry_enabled()
{
    LTC4162_read_register(&ltc4162, LTC4162_FORCE_TELEMETRY_ON, &data);
    return data;
}

void ESP8266_sleep()
{
    ESP.deepSleep(DEEP_SLEEP_TIME, WAKE_RF_DEFAULT);                      // WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED.
}

void detect_solar_panel()
{
    float vbat, vinoc;
    uint16_t ibat1, ibat2;
    LTC4162_write_register(&ltc4162, LTC4162_MPPT_EN, false);
    LTC4162_write_register(&ltc4162, LTC4162_INPUT_UNDERVOLTAGE_SETTING, LTC4162_VIN_UVCL_R2U(36));
    delay(0.25 TIMER_SECONDS);
    LTC4162_read_register(&ltc4162, LTC4162_CELL_COUNT, &cell_count);
    LTC4162_read_register(&ltc4162, LTC4162_VBAT, &data);
    vbat = LTC4162_VBAT_FORMAT_I2R(data) * cell_count / 2;
    LTC4162_read_register(&ltc4162, LTC4162_VIN, &data);
    vinoc = LTC4162_VIN_FORMAT_I2R(data);
    LTC4162_write_register(&ltc4162, LTC4162_INPUT_UNDERVOLTAGE_SETTING, LTC4162_VIN_UVCL_R2U(vbat + 2));
    delay(2 TIMER_SECONDS);                                      // Get through battery detection
    
    LTC4162_write_register(&ltc4162, LTC4162_INPUT_UNDERVOLTAGE_SETTING, LTC4162_VIN_UVCL_R2U(VIN_SOLAR_DROPOUT * vinoc));
    delay(0.25 TIMER_SECONDS);
    LTC4162_read_register(&ltc4162, LTC4162_IBAT, &ibat1);
    LTC4162_write_register(&ltc4162, LTC4162_INPUT_UNDERVOLTAGE_SETTING, LTC4162_VIN_UVCL_R2U(vbat + 2));
    delay(0.25 TIMER_SECONDS);
    LTC4162_read_register(&ltc4162, LTC4162_IBAT, &ibat2);
    
    if (ibat2 > ibat1 * 1.05)
    {
        // Serial.println("Possible Solar Panel");
        solar_panel = true;
        LTC4162_write_register(&ltc4162, LTC4162_MPPT_EN, true);
    }
    else
    {   // Serial.println("Low Impedance Input");
        solar_panel = false;
        LTC4162_write_register(&ltc4162, LTC4162_INPUT_UNDERVOLTAGE_SETTING, LTC4162_VIN_UVCL_R2U(vbat + 2));
    }
    // Serial.println("");
}

void setup()
{
    pinMode(D0, INPUT_PULLUP); pinMode(D3, OUTPUT); pinMode(D4, OUTPUT); pinMode(EQUALIZE, OUTPUT); pinMode(BULK, OUTPUT); pinMode(ABSORB, OUTPUT); pinMode(D8, OUTPUT); pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(D3, LOW); digitalWrite(D4, LOW); digitalWrite(D8, LOW); digitalWrite(LED_BUILTIN, HIGH);
    Wire.begin(SDA, SCL);                                           // Make an I2C port
  
    if (!input_power_present() and !telemetry_enabled())
        ESP8266_sleep();
        
    os_timer_setfn(&solar_panel_timer, timerCallback, NULL);
    os_timer_arm(&solar_panel_timer, SOLAR_CHECK_TIMEOUT, true);    // This true means repeat
    solar_panel_timeout = true;                                     // Check for solar panel immediately
    LTC4162_write_register(&ltc4162, LTC4162_TELEMETRY_SPEED, LTC4162_TELEMETRY_SPEED_ENUM_TEL_HIGH_SPEED);
    digitalWrite(LED_BUILTIN, LOW);                                 // Blue LED on shows input power present
        
    Serial.begin(115200);                                           // Initialize the serial port to the PC
    while (!Serial);                                                // Wait for serial port to be opened in the case of Leonardo USB Serial
    
    WiFi.mode(WIFI_AP);                                             // Our ESP8266-12E is an AccessPoint
    WiFi.softAP("IoTender", "12345678");                            // Provide the (SSID, password);
    server.begin();                                                 // Start the HTTP Server
    // IPAddress HTTPS_ServerIP = WiFi.softAPIP();                     // Obtain the IP of the Server
    // Serial.print("Server IP is: ");                                 // Print the IP to the monitor window
    // Serial.println(HTTPS_ServerIP);                                 // Should be 192.168.4.1
}

void loop()
{
    input_power_detected = input_power_present();
    if (!input_power_detected and !telemetry_enabled())
        ESP8266_sleep();
        
    if (!input_power_detected)
    {
        solar_panel_timeout = true;     // Do instant solar panel detection on next input power
        solar_panel = false;
    }
        
    if (solar_panel_timeout and input_power_detected)
    {
        //Serial.println("Checking for solar panel...");
        detect_solar_panel();
        solar_panel_timeout = false;
    }
    yield();  // or delay(0);

    LTC4162_read_register(&ltc4162, LTC4162_CELL_COUNT, &data);
    if (data != LTC4162_CELL_COUNT_ENUM_UNKNOWN)
        cell_count = data;
    //else
    //    retain last valid value of cell_count
    
    LTC4162_read_register(&ltc4162, LTC4162_VBAT, &data);
    dtostrf(LTC4162_VBAT_FORMAT_I2R(data) * cell_count, 5, 3, vbat);
    
    LTC4162_write_register(&ltc4162, LTC4162_INPUT_UNDERVOLTAGE_SETTING, LTC4162_VIN_UVCL_R2U(17));
    
    LTC4162_read_register(&ltc4162, LTC4162_VIN, &data);
    dtostrf(LTC4162_VIN_FORMAT_I2R(data), 5, 3, vin);
    
    LTC4162_read_register(&ltc4162, LTC4162_VOUT, &data);
    dtostrf(LTC4162_VOUT_FORMAT_I2R(data), 5, 3, vout);
    
    LTC4162_read_register(&ltc4162, LTC4162_IBAT, &data);
    dtostrf(LTC4162_IBAT_FORMAT_I2R(data), 5, 3, ibat);
    
    LTC4162_read_register(&ltc4162, LTC4162_IIN, &data);
    dtostrf(LTC4162_IIN_FORMAT_I2R(data), 5, 3, iin);
    
    LTC4162_read_register(&ltc4162, LTC4162_DIE_TEMP, &data);
    dtostrf(LTC4162_DIE_TEMP_FORMAT_I2R(data), 5, 3, die_temp);
    
    LTC4162_read_register(&ltc4162, LTC4162_THERMISTOR_VOLTAGE, &data);
    dtostrf(LTC4162_NTCS0402E3103FLT_I2R(data), 5, 3, thermistor_voltage);

    thermistor_present = data < LTC4162_NTCS0402E3103FLT_R2I(-45); // Missing thermistor, less than because NTC!
    // if (thermistor_present)
        // LTC4162_write_register(&ltc4162, LTC4162_EN_SLA_TEMP_COMP, true);
    // else
        // LTC4162_write_register(&ltc4162, LTC4162_EN_SLA_TEMP_COMP, false);
    
    LTC4162_read_register(&ltc4162, LTC4162_BSR, &data);
    dtostrf(LTC4162_BSR_FORMAT_U2R(data) * cell_count * 1000, 5, 3, bsr);
    
    LTC4162_read_register(&ltc4162, LTC4162_TCHARGETIMER, &data);
    sprintf(tcharge_timer, "%dh %dm %ds", data/3600, data%3600/60, data%60);
    
    LTC4162_read_register(&ltc4162, LTC4162_TCVTIMER, &data);
    sprintf(tcv_timer, "%dh %dm %ds", data/3600, data%3600/60, data%60);
       
    LTC4162_read_register(&ltc4162, LTC4162_CHARGER_STATE, &data);
    switch (data)
    {
    case 1:
        charger_state = "Shorted Battery";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    case 2:
        charger_state = "Open Battery";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    case 4:
        charger_state = "Max Time Fault";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    case 8:
        charger_state = "C/X Termination";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    case 16:
        charger_state = "Timer Termination";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    case 32:
        charger_state = "NTC Pause";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    case 64:
        charger_state = "CC/CV Charge";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, HIGH);
        break;
    case 128:
        charger_state = "Precharge";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    case 256:
        charger_state = "Suspended";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    case 2048:
        charger_state = "Bat Detection";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    case 4096:
        charger_state = "Bat Detect Failed";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    default:
        charger_state = "";
        digitalWrite(BULK, LOW);
        digitalWrite(ABSORB, LOW);
        digitalWrite(EQUALIZE, LOW);
        digitalWrite(FLOAT, LOW);
        break;
    }
    
    LTC4162_write_register(&ltc4162, LTC4162_THERMAL_REG_START_TEMP, LTC4162_DIE_TEMP_FORMAT_R2I(109));
    LTC4162_write_register(&ltc4162, LTC4162_THERMAL_REG_END_TEMP, LTC4162_DIE_TEMP_FORMAT_R2I(111));

    client = server.available();
    if (!client)
        return;
    
    String request = client.readStringUntil('\r');
    // Serial.println(request);
    client.flush();                                                     //clear previous info in the stream
    
    if (request.indexOf("/TEL_ON") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_FORCE_TELEMETRY_ON, true);
    if (request.indexOf("/TEL_OFF") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_FORCE_TELEMETRY_ON, false);
        
    if (request.indexOf("/BSR_ON") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_RUN_BSR, true);
    if (request.indexOf("/BSR_OFF") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_RUN_BSR, false);

    if (request.indexOf("/CX_ON") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_EN_C_OVER_X_TERM, true);
    if (request.indexOf("/CX_OFF") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_EN_C_OVER_X_TERM, false);
        
    if (request.indexOf("/ENABLE_ON") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_SUSPEND_CHARGER, false);
    if (request.indexOf("/ENABLE_OFF") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_SUSPEND_CHARGER, true);
        
    if (request.indexOf("/JEITA_ON") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_EN_JEITA, true);
    if (request.indexOf("/JEITA_OFF") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_EN_JEITA, false);
        
    if (request.indexOf("/SHIP_ON") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_ARM_SHIP_MODE, true);
    if (request.indexOf("/SHIP_OFF") != -1)
        LTC4162_write_register(&ltc4162, LTC4162_ARM_SHIP_MODE, false);

    // Serial.println("Somebody has connected :)");                    //Read what the browser has sent into a String class and print the request to the monitor
    client.print(F("HTTP/1.1 200\r\n"));
    client.print(F("Content-Type: text/html; charset=utf-8\r\n\r\n"));
    client.print(F("<!DOCTYPE HTML>"));
    client.print(F("<html>"));
    client.print(F("<meta http-equiv=\"refresh\" content=\"2; url=http://192.168.4.1/\">")); // Update rate is 2 Second, gives more time for user to enter button click?
    client.print(F("<font face=\"Helvetica Narrow\" size=\"10\">"));
    client.print(F("<head>"));
    client.print(F("<style>"));
    client.print(F("table {border-collapse:collapse; width:100%;}"));
    client.print(F("th, td {border:1px solid Grey; text-align:left;  padding:1px;}"));
    client.print(F("tr:nth-child(even) {background-color: #ced0ef;}"));
    client.print(F("tr:nth-child(odd) {background-color: #ceeeef;}"));
    client.print(F("tr:first-child {background-color: #fdff89;}"));
    client.print(F("</style>"));
    
    client.print(F("<style>"));
    client.print(F(".button_red {width: 48%; background-color:HotPink; border:3px solid DarkRed; box-shadow: -7px 7px grey; color:black; padding:5px 50px; text-align:center; text-decoration:none; font-size:50px; margin:10px 10px; cursor:pointer; border-radius: 12px}"));
    client.print(F(".button_red:active {width: 48; background-color:HotPink; box-shadow:-1px 1px grey; transform:translateY(7px)} translateX(-7px)}"));
    client.print(F("</style>"));

    client.print(F("<style>"));
    client.print(F(".button_green {width: 48%; background-color:Lime; border:3px solid Darkgreen; box-shadow: -7px 7px grey; color:black; padding:5px 50px; text-align:center; text-decoration:none; font-size:50px; margin:10px 10px; cursor:pointer; border-radius: 12px}"));
    client.print(F(".button_green:active {width: 48%; background-color:Lime; box-shadow:-1px 1px grey; transform:translateY(7px)} translateX(-7px)}"));
    client.print(F("</style>"));
    
    client.print(F("</head>"));
    
    client.print(F("<body bgcolor=\"Bisque\">"));
    client.print(F("<h2 align=\"center\">IoTender&trade;</h2>"));
    client.print(F("<table>"));

    add_table_row("<b>PARAMETER</b>", "<b>VALUE</b>", true);
    add_table_row("Battery Voltage", value.assign(vbat) + "V", true);
    add_table_row("Input Voltage", value.assign(vin) + "V", true);
    // add_table_row("Output Voltage", value.assign(vout) + "V", true);
    add_table_row("Battery Current", value.assign(ibat) + "A", true);
    add_table_row("Input Current", value.assign(iin) + "A", true);
    add_table_row("Die Temperature", value.assign(die_temp) + "&deg;C", true);
    if (thermistor_present)
        add_table_row("Thermistor Temp", value.assign(thermistor_voltage) + "&deg;C", true);
    add_table_row("Battery Impedance", value.assign(bsr) + "m&Omega;", true);
    add_table_row("Charger State", charger_state, true);
    LTC4162_read_register(&ltc4162, LTC4162_CHARGE_STATUS, &data);
    switch (data)
    {
        case 0:
            value = "Charger Off";          break;
        case 1:
            value = "Constant Voltage";     break;
        case 2:
            value = "Constant Current";     break;
        case 4:
            value = "Input Current";        break;
        case 8:
            value = "Input Voltage";        break;
        case 16:
            value = "Thermal Regulation";   break;
        case 32:
            value = "Dropout";              break;
        default:
            value = "None";                 break;
    }
    add_table_row("Regulation Loop", value, true);
    add_table_row("Charge Time", value.assign(tcharge_timer, 11), true);
    add_table_row("C.V. Time", value.assign(tcv_timer, 11), true);
    
    if (input_power_present())
        add_table_row("Power Source",  solar_panel ? "Solar Panel" : "Wall Adapter", true);
    else
        add_table_row("Power Source", "None", true);
    
    LTC4162_read_register(&ltc4162, LTC4162_EN_CHG, &data);
    add_table_row("Charger Enabled", data ? "<font color=\"blue\"><b><i>True</i></b></font>" : "<font color=\"red\"><b><i>False</i></b></font>", true);

    client.print(F("</table>"));   
    
    LTC4162_read_register(&ltc4162, LTC4162_FORCE_TELEMETRY_ON, &data);
    if (data)
        client.print(F("<button class=\"button_green\" onclick=location.href=\"/TEL_OFF\"; style=\"position:absolute; left:0%\">TELEMETRY</button>"));
    else
        client.print(F("<button class=\"button_red\" onclick=location.href=\"/TEL_ON\"; style=\"position:absolute; left:0%\">TELEMETRY</button>"));

    LTC4162_read_register(&ltc4162, LTC4162_RUN_BSR, &data);
    if (data)
        client.print(F("<button class=\"button_green\" onclick=location.href=\"/BSR_OFF\"; style=\"position:absolute; right:0%\">GET B.S.R.</button>"));
    else
        client.print(F("<button class=\"button_red\" onclick=location.href=\"/BSR_ON\"; style=\"position:absolute; right:0%\">GET B.S.R.</button>"));

    client.print(F("<br><br>"));
        
    LTC4162_read_register(&ltc4162, LTC4162_EN_C_OVER_X_TERM, &data);
    if (data)
        client.print(F("<button class=\"button_green\" onclick=location.href=\"/CX_OFF\"; style=\"position:absolute; left:0%\">C/X TERM</button>"));
    else
        client.print(F("<button class=\"button_red\" onclick=location.href=\"/CX_ON\"; style=\"position:absolute; left:0%\">C/X TERM</button>"));

    LTC4162_read_register(&ltc4162, LTC4162_SUSPEND_CHARGER, &data);
    if (data)
        client.print(F("<button class=\"button_red\" onclick=location.href=\"/ENABLE_ON\"; style=\"position:absolute; right:0%\">ENABLE</button>"));
    else
        client.print(F("<button class=\"button_green\" onclick=location.href=\"/ENABLE_OFF\"; style=\"position:absolute; right:0%\">ENABLE</button>"));

    client.print(F("<br><br>"));
        
    LTC4162_read_register(&ltc4162, LTC4162_EN_JEITA, &data);
    if (data)
        client.print(F("<button class=\"button_green\" onclick=location.href=\"/JEITA_OFF\"; style=\"position:absolute; left:0%\">TEMP COMP</button>"));
    else
        client.print(F("<button class=\"button_red\" onclick=location.href=\"/JEITA_ON\"; style=\"position:absolute; left:0%\">TEMP COMP</button>"));

    LTC4162_read_register(&ltc4162, LTC4162_ARM_SHIP_MODE, &data);
    if (data)
        client.print(F("<button class=\"button_green\" onclick=location.href=\"/SHIP_OFF\"; style=\"position:absolute; right:0%\">SHIP MODE</button>"));
    else
        client.print(F("<button class=\"button_red\" onclick=location.href=\"/SHIP_ON\"; style=\"position:absolute; right:0%\">SHIP MODE</button>"));
       
    client.print(F("</body></font></html>\n"));

    // Serial.println("Client disconnected");
}

/*! read_register function wraps C++ method LT_SMBus::readWord and places the returned data in *data.
 * Prototype: virtual uint16_t LT_SMBus::readWord(uint8_t address, uint8_t command);
 * Function should return 0 on success and a non-0 error code on failure. The API functions will return your error code.
 */
int read_register(uint8_t address,                          //!< IC's SMBus address (7-bit format).
                  uint8_t command_code,                     //!< IC's Register address (SMBus command code) for read.
                  uint16_t *data,                           //!< Memory location to store whole register data read from communication interface.
                  struct port_configuration *pc             //!< Pointer to additional implementation-specific port configuration struct, if required.
                 )
{
    (void)pc;                                   //Unneeded parameter in this implementation.
    uint8_t the_byte;
    Wire.beginTransmission((int)address);
    Wire.write(command_code);
    Wire.endTransmission(RESTART);
    Wire.requestFrom((int)address,(int)3,(int)STOP);
    the_byte = Wire.read();
    *data = (Wire.read() << 8) | the_byte;
    if(Wire.read() != pec_read_word(address, command_code, *data)) // PEC error indicates I2C port is out of sorts.
    {                                                           // Only successful clearing of port has been full chip reset.
        pinMode(SDA, INPUT_PULLUP);                             // To be more polite we could have a PEC error counter just
        pinMode(SCL, OUTPUT);                                   // in case an occasional PEC error clears itself up.
        for (int i=0; i<64; i++){                               // This block flushes out the 4162 assuming it's jammed up too.
            digitalWrite(SCL, LOW);
            digitalWrite(SCL, LOW);
            digitalWrite(SCL, LOW);
            digitalWrite(SCL, HIGH);
            digitalWrite(SCL, HIGH);
            digitalWrite(SCL, HIGH);
        }
        pinMode(D0, OUTPUT);                                    // Hammer the reset line low.
        digitalWrite(D0, LOW);                                  // This seems to clear up the I2C port every time.
    }
    return 0;
}

/*! write_register function wraps C++ method LT_SMBus::writeWord.
 * Prototype: virtual void LT_SMBus::writeWord(uint8_t address, uint8_t command, uint16_t data);
 * Function should return 0 on success and a non-0 error code on failure. The API functions will return your error code.
 */
int write_register(uint8_t address,                             //!< IC's SMBus address (7-bit format).
                   uint8_t command_code,                        //!< IC's Register address (SMBus command code) for write.
                   uint16_t data,                               //!< Whole register data to be written to communication interface.
                   struct port_configuration *pc                //!< Pointer to additional implementation-specific port configuration struct, if required.
                  )
{
    (void)pc;                                   //Unneeded parameter in this implementation.
    Wire.beginTransmission((int)address);
    Wire.write(command_code);
    Wire.write(data & 0xff);
    Wire.write((data >> 8) & 0xff);
    Wire.endTransmission(STOP);
    return 0;
}

int add_table_row(std::string column1_value, std::string column2_value, bool send_it)
{
    html = "";
    html += "<tr><td>";
    html += column1_value;
    html +="</td><td>";
    html += column2_value;
    html +="</td></tr>";
    if (send_it)
        client.print(html.c_str());
    return 0;
}



