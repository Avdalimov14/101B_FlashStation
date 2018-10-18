# 101B_FlashStation

# Prerequisites instructions
1. Clone / Download 101B_FlashStation repo.
2. Replace / Extract ZIP to <code> C:\\</code>.
3. Rename folder name to <code> 101B_FlashStation</code>.
4. Download required release or current dev repo from https://github.com/ezritarazi/101B_Release (If you need credentials ask them from Albert). <br/>
Note: You can find example repo in <code> C:\101B_FlashStation\home\Administrator</code>.
5. Extract ZIP to <code> C:\101B_FlashStation\home\YOUR_USERNAME</code>.
6. Connect an ESP32 device. <br/>
# flashScript instructions
1. Open <code> C:\101B_FlashStation</code> folder.
2. Run mingw32.exe.
3. If the downloaded required repo is a dev one, run:<br/>
 <code> $ ../Administrator/flashScript.sh dev</code><br/>
   If the downloaded required repo is a release, run:<br/>
 <code> $ ../Administrator/flashScript.sh \<version tag></code><br/>
   For example, for version tag = v1.2.231:<br/> 
 <code> $ ../Administrator/flashScript.sh v1.2.231</code><br/>
4. In case everything was O.K, a blue screen will pop within short time.<br/>
  5. Navigate with arrow keys to <code> Serial flasher config</code>
  6. Check your ESP32 COM port number from Device Manager.
  7. Enter it as Default serial port, for example <code> COM13</code>
  8. Use arrow keys to save and then exit.
  9. Now it should build and flash required version.
  10. In case your last echo on Terminal was <code> Hard resetting.. </code> you've done your mission.
 
