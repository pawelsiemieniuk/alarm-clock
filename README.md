# Alarm Clock for MSP430-EasyWeb2

 Program was compiled and sent to board using IAR Embedded Workbench. Required libraries for LCD are included in [used_libraries](/used_libraries). Documentation and diagram of MSP430-EasyWeb2 used for creating this project can be found in [docs](/docs).

 Functionalities:
 * Displaying time on LCD in format 00:00:00:00 (On start, time is set to value defined in main.c file).
 * Saving up to 3 alarms and turning on buzzer few times at specified hour.
 
 Usage:
 * Click 4th button (first on the right) to turn on alarm setting mode. Now display shows time for first alarm slot.
 * Clicking buttons 1-3 changes, starting from left: hour, minute, second, of edited alarm.
 * To save alarm click again 4th button.
 * Repeating process creates new alarms up to 3, 4th alarm will overwrite 1st.

 Project was made with Szymon Siłkin on Computer Organization and Architecture class at Bialystok University of Technology.