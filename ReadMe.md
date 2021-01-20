# Traffic light system

The project is an implementation of a traffic light system with interactive pedestrian lights.

---

## Project link

Click **[here](https://drive.google.com/drive/u/0/folders/1tDfyxjgeiJjBi3QTH6Mg0XAPvPP81F75)** for the project drive

## Connections and wiring

| **LED**          | Traffic North | Traffic North |
| ---------------- | ------------- | ------------- |
|Traffic Red       |     PE0       |     PD0       |
|Traffic Yellow    |     PE1       |     PD1       |
|Traffic Green     |     PE2       |     PD2       |
|Pedestrian Red    |     PE4       |     PF1       |
|Pedestrian Green  |     PE3       |     PF3       |
|Pedestrian Button |     PF4\*     |     PD3\*     |

\* The buttons were connected with pull-up resistors and were configured as such in their configuration settings

## Global variables and constants

```c
#define FREQUENCY   16000000

char NorthState = 'G';
char EastState = 'R';
```

## Timers

### Configurations used

| **Timer** |                            Role                                    |
| :-------: | :----------------------------------------------------------------: |
|  Timer0   |   Alternates with Timer1 for main traffic system                   |
|  Timer1   |   Alternates with Timer0 for main traffic system                   |
|  Timer2   |   Controls the pedestrian green light delay                        |
|  Timer3   |   Allows one second delays between each pedestrian button press    |

| **Timer** | Configuration |  Size  | Interrupt |
| --------- | ------------- | ------ | --------- |
|  Timer0   |   One shot    | 32-bit |  Time-out |
|  Timer1   |   One shot    | 32-bit |  Time-out |
|  Timer2   |   One shot    | 32-bit |    None   |
|  Timer3   |   One shot    | 32-bit |    None   |

### Basic concept

The system alternates between Timer0 and Timer1 for the main traffic lights control system as follows:

1. Timer0 starts by delaying for 5 seconds while lighting the green LED for the North/South traffic lights.
   - If the North/South pedestrian button was pressed the green light is interrupted and set to Red while the green pedestrian LED lights up for 2 uninterruptible seconds.
   - If the East/West pedestrian button is pressed, nothing happens right away but the interrupt is delayed till the next East/West green light LED duration.
   - If both were pressed at the same time the North/South pedestrian system interrupts the traffic and the East/West is delayed for it's correct time.
   - In the case the same button was pressed more than once or was pressed during the 1 second delay between presses then the interrupt is cancelled and only starts registering after the one second ends
2. Timer1 then starts after while lighting the yellow North/South LED for 3 uninterruptible seconds.
3. After that, Timer0 starts a one second uninterruptible delay for the Red North/South LED.
4. The same happens again for the East/West traffic lights but the timers alternate starting from Timer1 for the green LED for 5 seconds.
   - If the East/West pedestrian button was pressed the green light is interrupted and set to Red while the green pedestrian LED lights up for 2 uninterruptible seconds.
   - If the North/South pedestrian button is pressed, nothing happens right away but the interrupt is delayed till the next North/South green light LED duration.
   - If both were pressed at the same time the East/West pedestrian system interrupts the traffic and the North/South is delayed for it's correct time.
   - In the case the same button was pressed more than once or was pressed during the 1 second delay between presses then the interrupt is cancelled and only starts registering after the one second ends
5. Timer0 counts 3 seconds for the yellow East/West LED.
6. Timer1 then starts counting 1 second Red East/West LED.
7. The loop starts from point 1 all over again.

### UART

The system sends the current state of the system through UART0 during the correct states:

- Sends "CARS NORTH SOUTH" as soon as the North/South traffic system lights green
- Sends "CARS EAST WEST" as soon as the East/West traffic system lights green
- Sends "PEDESTRIAN NORTH SOUTH" as soon as the North/South pedestrian button is pressed (assuming the 1 second delay is over)
- Sends "PEDESTRIAN NORTH SOUTH" as soon as the East/West pedestrian button is pressed (assuming the 1 second delay is over)

---
