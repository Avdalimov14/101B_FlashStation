# 101B_FlashStation

# Prerequisites instructions
1. Download 101B_FlashStation repo.
2. Extract ZIP to C:\.
3. Remove the "-master" from extracted folder name.
4. Download required release or current dev repo from https://github.com/ezritarazi/101B_Release (If you need credentials ask them from Albert). <br/>
5. Extract ZIP to C:\101B_FlashStation\home\Administrator.
6. Connect a ESP32 device.
# flashScript instructions
1. Navigate to C:\101B_FlashStation.
2. Open mingw32.exe.
3. If the downloaded required repo is a dev one, run:<br/>
 <code> $ ./flashScript.sh dev </code><br/>
   If the downloaded required repo is a release, run:<br/>
 <code> $ ./flashScript.sh <version tag> </code><br/>
   For example, for version tag = v1.2.231:<br/> 
 <code> $ ./flashScript.sh v1.2.231 </code><br/>
4. In case everything was O.K, a blue screen will pop within short time.<br/>
  5. Navigate with arrow keys to <code> Serial flasher config </code>
  6. Check your ESP32 COM port number from Device Manager.
  7. Enter it as Default serial port, for example <code> COM13 </code>
  8. Use arrow keys to save and then exit.
  9. Now it should build and flash required version.
  10. In case your last echo on Terminal was <code> Hard resetting... </code> you've done your mission.
 
