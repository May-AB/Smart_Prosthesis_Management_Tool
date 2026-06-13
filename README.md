## Smart Prosthesis Management Tool Project

This is a personal fork of a project that began as a team university project in cooperation with Haifa3D, a non-profit organisation that develops personalized low-cost prosthesis.

### Developed By:
**Avigail Yampolsky, Elisheva Hammer, and May Abraham**

---

## Project Overview
This project focuses on developing a touchscreen-based server for managing a smart prosthesis controller using an ESP32. The touchscreen interface serves as the UI, enabling users to:
- Customize prosthetic parameters
- Execute preprogrammed movements
- Adjust sensor behavior

However, it does not directly control the prosthesis itself.

The prosthesis controller (see prosthesis mock files under the **ESP32** folder) sends its current system settings to the touchscreen server in **YAML format**. This YAML file contains all relevant data, including:
- Available sensors and motors
- Screen access passwords
- Functions to be executed

When no BLE connection is available, the screen provides an option to load a **mock YAML file**, allowing users to add, modify, and debug different screens.

The YAML file contains **five major fields**:
- General
- Sensors
- Motors
- Functions  
- Communications - not required for the tool's operation.

An example YAML file can be found in the **assets** folder.

---

## Management Tool Modes



<p align="center">
    <img src="https://github.com/user-attachments/assets/01ce6054-2f77-4731-abbf-d00071be1b55" width="300" />
</p>

The manager tool operates in three distinct modes:

<p align="center">
    <img src="https://github.com/user-attachments/assets/73905bed-a963-48f4-a9b6-d2b356759ced" width="300" />
</p>

### 1. **Daily Mode**
Designed for regular use, this mode is divided into three tabs:
- **Home Tab**: Allows users to execute preprogrammed movements.
- **Status Tab**: Displays the current status of all sensors.
- **Setup Tab**: Enables turning sensors on and off.


<p align="center">
    <img src="https://github.com/user-attachments/assets/7a57372c-259c-4cfd-aa3e-4f3297e30ca6" width="300" />
    <img src="https://github.com/user-attachments/assets/f60f4d9c-9cb4-4f83-87a5-26b1855be5e0" width="300" />
    <img src="https://github.com/user-attachments/assets/d6bf44ca-5a89-4efb-ae01-2377b729756f" width="300" />
</p>

### 2. **Tech Mode**
This mode is password-protected (defined in the YAML file). It includes all three tabs from **Daily Mode**, plus an additional **Tech Tab**, which allows:
- Reading and modifying motor and sensor-related data.
- Manually turning motors on and off for testing.


<p align="center">
    <img src="https://github.com/user-attachments/assets/146894b4-ae62-49be-9a25-9c1b196dd0dd" width="300" />
    <img src="https://github.com/user-attachments/assets/b96051f5-1fa1-4f64-8181-0b5eb90011a4" width="300" />
       <img src="https://github.com/user-attachments/assets/a6cc9fb8-ed2c-4ceb-93b8-3c0325fff605" width="300" />
</p>

### 3. **Debug Mode**
Also password-protected, this mode includes all four tabs from **Tech Mode** plus a **Debug Tab**, which allows live plotting of sensor or motor output data for debugging purposes.

<p align="center">
    <img src="https://github.com/user-attachments/assets/70185578-0e03-4371-baad-865772bee6cf" width="300" />
    <img src="https://github.com/user-attachments/assets/3a9b9e71-9963-4c44-81b6-c0b945ca1878" width="300" />
</p>

---

## BLE Communication

### **Initialization & Connection**
- On startup, the management controller displays a **BLE connection screen** and attempts to connect using the predefined **UUID**.
- The prosthesis controller parses the **YAML file** and sends the parsed data to the **management controller**, which stores it in a structured dictionary.
- If reconnection is needed, a button on this screen allows restarting the connection process.


<p align="center">
    <img src="https://github.com/user-attachments/assets/2a61eb70-498f-4705-9d52-9681ed4350d3" width="300" />
</p>



### **Request's interpretation**
All requests are transmitted as a **byte array** in hexadecimal format. The byte array includes additional metadata for handling and parsing. It follows the structure below:

- **First 4 bytes**: Request or response type, using predefined enums (see Request Types section). This determines how the request is processed.
- **Bytes 5-8**: Current message number. If a message is too long to send in one part, it is split into multiple messages.
- **Bytes 9-12**: Total number of messages expected for the request. Used to track message sequences.
- **Bytes 13-16**: Length of the actual message payload (not the total byte array length). This represents the size of the `char*` message.
- **Bytes 17-(MAX_MSG_LEN+17)**: The message itself, parsed as a `char*`.
- **Last 4 bytes**: Expected checksum value for data integrity verification.

The byte array is interpreted using the following predefined structure:

```cpp
struct msgInterpeterStruct{
  int reqType; // Type of request.
  int curMsgCount;  // Current message index in the request 
  int totMsgCount;  // Total number of expected messages for the request
  int msgLength; // Length of the message data 
  char msg[MAX_MSG_LEN];  // The message itself   
  int checksum; // Error-checking value
};
```

Where **MAX_MSG_LEN** is a predefined maximum message size. If a message exceeds this limit, it will be split into multiple messages and sent sequentially.
Additionally, when needed, each **motor, sensor, and parameter** is identified by its respective index in the corresponding vector.

### **Request Types**

- **EMERGENCY_STOP** – A high-priority request running on a separate task, triggered by pressing the **BOOT button** on the management controller. When activated, the management tool sends a request to halt all motors. This remains functional as long as a BLE connection is active.

- **CHANGE_SENSOR_STATE_REQ** – Requests enabling or disabling specific sensors. Multiple sensors and states (1 = ON, 0 = OFF) can be updated simultaneously based on user input in **Daily Mode**.

- **CHANGE_SENSOR_PARAM_REQ** – Requests updating a sensor's parameter value. Multiple parameter modifications can be sent at once, specifying the **sensor ID, parameter ID, and desired value**, based on user input in **Tech Mode**.

- **CHANGE_MOTOR_PARAM_REQ** – Requests changing a motor’s **safety threshold** value. This request requires the **motor ID** and is initiated based on user input in **Tech Mode**.

- **GEST_REQ** – Requests executing a **predefined movement (gesture)**. The movement name is retrieved from the YAML file under the **function field** and categorized as a "gesture" type. The prosthesis must have a matching gesture defined with the same name.

- **YML_SENSOR_REQ, YML_MOTORS_REQ, YML_FUNC_REQ, YML_GENERAL_REQ** – These requests are sent sequentially upon establishing a connection. To accommodate larger YAML files, each YAML field is transmitted separately. Each request is sent **only after** the previous one has been fully processed to prevent data loss.

---

## Folder Description
- **ESP32**: Source code for the ESP32 firmware. Contains source code for managment tool and the mock prosthesis code. 
- **Unit Tests**: Tests for individual hardware components (input/output devices).
- **Parameters**: Contains descriptions of configurable parameters.
- **Assets**: Contains resources such as libraries, yaml file and lvgl configuration file.

---
## Firmware

### Management Tool (1x ESP32-3248S032 N/R/C)
This board includes the following features:
- USB Type-C
- ST7789 Display
- XPT2046 Touchscreen / GT911
- TF Card Interface
- I2C: 2 x JST1.0 4p connectors
- Power & Serial: JST 1.25 4p connector
- Speaker: JST 1.25 2p connector
- Battery Interface: JST 1.25 2p connector

### Mock Prosthesis
1x Any ESP32 with BLE connectivity.

---
## Arduino/ESP32 Libraries Used
- **ArduinoJson** by Benoit Blanchon - 7.1.0
- **NimBLE-Arduino** by h2zero - 2.1.2
- **YAMLDuino** by tobozo - 1.4.2
- **TAMC_GT911** by TAMC - 1.0.2
- **lvgl** by kisvegabor - 8.3.3
- **GFX Library for Arduino** by Moon On Our Nation - 1.2.9
- **Touch_GT911** (Manually installed, see assets)
- **SmartProsthesisShared** (Created for this project, see assets)

Project was compiled using core driver **ESP32** by Espressif-2.0.17 and partition scheme "No OTA (2MB APP/2MB SPIFFS)" .

