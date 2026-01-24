# ESP8266 Setup Guide

Follow these steps to configure your Arduino IDE and install the required drivers for ESP8266 development.

### 1. Configure Arduino IDE
*   Open **Arduino IDE**.
*   Go to **File** → **Preferences**.
*   In the **Additional Boards Manager URLs** field, add the following URL:
    `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
*   Click **OK**.

### 2. Install ESP8266 Board Package
*   Go to **Tools** → **Board** → **Boards Manager**.
*   Search for **"ESP8266"**.
*   Find **"esp8266 by ESP8266 Community"** and click **Install**.

### 3. Install USB Driver (CP210x)
*   Open the folder **CP210x_Universal_Windows_Driver**.
*   Right-click on the **silabser.inf** file.
*   Select **Install**.
*   The COM port should now be visible under **Tools** → **Port**.


