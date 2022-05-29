# watch
Created for 6.08: Embedded Systems at MIT.

# Video

Demo: <a href="https://youtu.be/DX8V341Kigo" target="_blank">https://youtu.be/DX8V341Kigo</a>

# Specifications

Below is how I addressed each specification:

## Watch Faces

**Requirement:**

* The watch should have two "faces", with details of the design left up to you:
    * A digital readout (for example "8:45") and 
    * An analog readout. The analog readout should have a hour, minute, and second hand. 

**Implementation:**

To create the digital readout, I parsed the timestamp obtained from the GET request using the `strtok` function and saved the hour and minute values in variables.
I used the TFT graphics library to print the hours and minutes onto the screen, separated by a colon.

To create the analog readout, I calculated the angular distance between each hand and the top of the clock. I used trigonometry to find the coordinates of each hand's endpoint, as explained further in the "Other Thoughts" section. Using the TFT graphics library, I drew a circle to represent the clock and three lines of varying styles to distinguish the hands.

## Switching Between Faces

**Requirement:**

* The user should be able swap between the two faces using a button connected to input **pin 5.**. Switching between the two faces should be based on pushing-and-then-releasing the button. One push-release sets it to analog. Another to digital. Another to analog. And so on. It cannot be pushed=analog/unpushed=digital for the faces. The button functionality must be sufficiently responsive that its usage isn't impaired at any time during operation (i.e. you should avoid **blocking** code with the exception of the GET request).

**Implementation:**

I used a state machine design to swap between two faces at the press of a button. If the button is pressed and state = 0, I print the digital readout to the screen and switch state for the next button press. If the button is pressed and state = 1, I print the analog readout and switch state. I update the previous button state with each iteration so the readouts only occur when the button goes from unpressed to pressed.

To address button bouncing, I included a nonblocking debouncing function to provide a more reliable reading of the button state. The state is switched after bouncing has steadied, quantified by checking if the previous button state has changed over a count threshold.

## Server Time Retrieval and Smooth Updating

**Requirement:**

* In order to get absolute time, your device must make periodic HTTP GET requests to the url here: <a href="http://iesc-s3.mit.edu/esp32test/currenttime" target="_blank">http://iesc-s3.mit.edu/esp32test/currenttime</a> which will return the current Boston time in a format like the following: `2018-02-11 18:37:29.776430`
* The device needs to only report the time (don't worry about temperature or anything else in the video), but it can't just repeatedly grab it off the server. Instead *at most* it can grab the time once every one minute, and in the time in between in must update based off of its own internal timer. 

**Implementation:**

To make an HTTP GET request, I create a client object and connect to the host iesc-s3.mit.edu. I formulate the GET request into a char array called `request_buffer`, which is sent to the host of interest. The host iesc-s3.mit.edu parses the request and generates a response, which is placed in the response buffer char array. I use the `strtok` and `atoi` functions to parse the response and later perform operations on the values.

To ensure the device only grabs information from the server once every minute, I initialize an internal timer and only make HTTP GET requests after a certain duration, namely `if(millis() - timer == 60000)`.
The timer is reset every minute.

# Other Thoughts

To create the analog clock, I set the origin to the top of the clock. Each hand is located a certain angular distance clockwise from the origin. For example, because there are 60 minutes around an analog clock, the corresponding angle of the minute hand is $$\theta = \frac{minute}{60}\cdot 2\pi$$

The horizontal distance between the endpoint of the hand and center of the clock is $r \sin\theta$
where $r$ is the radius of the clock. The vertical distance between the endpoint of the hand and center of the clock is $r \cos\theta$.

For the hour hand, I first made sure all hours fit in a 12-hour system. Because the hour hand is offset from each number by the minutes passed, the numerator for the angle of the hour hand includes the integer hour added to the fraction of the next hour passed.

The center of the clock is located at $(x_{center},y_{center})$, where $(0,0)$ is the upper left corner of the screen. To define the coordinates of the endpoints of each hand relative to the screen's origin, I adjusted 
the $x_{end}$ coordinate to $r \sin\theta + x_{center}$ and the $y_{end}$ coordinate to $y_{center} - r \cos\theta$.
I then scaled the $r$ variable for each hand to ensure they have distinct lengths.
