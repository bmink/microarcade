# microarcade

A tiny gaming console with 1-bit graphics and rotary encoders. It is built
from cheap components and the firmware is written completely from scratch (no
3rd party libraries).

## Why rotary encoders?

They're my favorite input devices for small builds. They take up very little
space yet allow you to do just about any type of input. I have long wondered
whether it would be fun to build and play a small console that uses two
rotary encoders for its input, supporting both single- and multi-user play.
(Verdict: it is!)

## Why no 3rd party libraries?

To be clear, we are not talking about bare metal. `microarcade` is based on
ESP32 and so is built using `esp-idf`, and runs on `FreeRTOS`. I'm only
talking about code to talk to the connected peripherals...

The two main types of peripherals the MCU interfaces with are the OLED display
and the rotary encoders. (There is also a separate clock module but that is
very simple to interface with using i2c).

* Rotary encoders are quite simple devices, but it is a surprisingly challenging
(and interesting) problem to build a driver for them that stands up to the
rigors of gaming. Most drivers out there are actually not that accurate in
terms of counting and will lose steps if you turn them fast as well as are
prone to other problems as well. For more information on this, check out
[esp_rotary](https://github.com/bmink/esp_rotary) which is my rotary encoder
driver for esp-idf.

* In the case of the display, I looked into several of the existing popular
graphics display code (eg. `Adafruit-GFX`, `U8g2`) but was surprised by
how inefficiently implemented they are and yet how cumbersome their APIs are
(these two problems go hand in hand). If you instrument their code to log the
commands they send to the display, you will be able to observe this: for
example, data transfers are unnecessarily broken up into several transactions
and many more commands are sent that would actually be necessary. All in
all, many more bytes are sent to the display than necessary and that many
times per second so it all would add up and reduce our per-frame CPU budget
during gameplay. These libraries appear to be a bit cobbled together and copy+pastey and therefore not appropriate for what I wanted here (small & fast
code).


## Parts list

* 1x [ESP32-S3](https://a.co/d/aNbB3Xg)
* 1x [OLED](https://a.co/d/eaWpX4F) -- use the SPI version
* 2x [Rotary encoder](https://a.co/d/dhtsNsp)
* 1x [RTC](https://a.co/d/ggVVWnh)
* 1x [Panel-mount USB C socket](https://a.co/d/aPcbE0N) - use "Screw Nut Mount Type"
* 1x [4-pin right-angle USB C cable](https://a.co/d/cTP49Ir)

The reason there's a separate clock module is that I wanted `microarcade` to be
able to function as a desktop clock so needed something that can keep time when
the device is turned off.


## Wiring diagram

![](wiring/microarcade_wiring.png)

Wiring is quite simple -- everything is hooked up to
gpios / 3.3V / common as appropriate.

There is no battery in the current design, just a USB-C socket. I always have a
power source around so I didn't find it necessary to add a dedicated battery,
and this allows the enclosure to be extra small and lightweight.  Additionally,
having the USB socket on the enclosure go directy into the esp32's USB input
allows me to easily flash new code without needing to open the enclosure or
to add additional sockets. That said, a rechargeable battery could be added
quite easily.


## Enclosure

![](enclosure/microarcade_enclosure.png)

The enclosure is 3D printed to fit the specified parts exactly.
Fusion 360, BambuLab and STL files are provided in the
[enclosure folder](enclosure/).

Tips:
* I print my enclosures using plain matte PLA filament
* It's a good idea to enable "support" in the slicer for the screw tabs
* The display should be attached to the enclosure using M2.5 screws and nuts.
* The screw holes for the lid are sized for M3 screws. They will tap their
own thread as you screw them in for the first time. Do not overtighten.


## Notes on the firmware

## The graphics library

### Framebuffers

Once a module's start function is called, the module is given complete control
over the console, including the display. 

Graphics are done by drawing into a framebuffer, ie. memory buffer that
represents the contents of the screen. In the case of `microarcade`, the
display is 128x64 pixels, and each pixel can be represented by one bit
(pixel on/off) so the framebuffer is 1024 bytes large (1024 * 68 / 8). Once the
app finishes drawing the frame, it sends it to the display (more on this
below).

To elimiate the need for data conversions when transmitting to the display,
in-memory framebuffer bit configuration follows that of the SSD1309 display
controller, and is *vertical*. This means that one byte of framebuffer data
represents a one pixel wide and 8 pixels tall section of the screen.
In most cases, applications don't have to worry about this since they won't
set bits in the framebuffer direcly, but instead use one of the drawing
functions provided.

### Sprites, tilemaps, fonts/text, shapes

Efficient support for all these are supported by `microarcade`.

`disp_blt()` is one of the most core functions in the library and is used not
only to blit sprites into a frame, but also called by most other graphics
functions. Whatever the application needs to draw that is not supporte by
built-in functions -- if it can do so by using one or a few `disp_blt()` calls,
it will be fast.

Sprite buffer data has to be vertically bitmapped. Check out
my CLI sprite & font editor/animator
[sprited](https://github.com/bmink/sprited) for creating your sprites. It can
also dump sprite data in C code syntax (horizontally or vertically mapped),
which can be copy and pasted straight into source files.


### The framebuffer pool

A module may want to allocate an internal copy of a framebuffer for efficiency.
For example, it may want to draw a static background into a framebuffer once
and then keep copying the contents of this framebuffer into the main
framebuffer before overlaying sprites.

A 1K framebuffer generally is too large to be put on the stack so these
copies will have to be allocated on the heap. To reduce the number of memory
allocations and potential fragmentation, a frame buffer pool is provided.
Applications can request a framebuffer using ` disp_getframebuf()`.
Framebuffers are released by calling `disp_releaseframebuf()`. The size of the
framebuffer pool is limited (8 by default), so applications should take care
not to use more than they absolutely need and they must have released all
all framebuffers they held when they exit.


## Display modes

Generally, applications are expected to draw into the frame buffer pointed to
by `curframe` using the provided functions such as `disp_blt()` (for bliting
sprites), `disp_drawbox()` or `disp_puttext()`. Then they are expected to tell
the library to send this frame to the display.

Two modes for sending frames to the display are supported:

### Ad-hoc mode

```C

#define EVENTSTRMAX	16

char eventstr[EVENTSTRMAX];
int eventcnt = 0;

disp_set_mode(DISP_MODE_ADHOC, 0);

while(1) {

	/* Draw into curframe */
	snprintf(evenstr, EVENTSTRMAX, "%d events", eventcnt);
	disp_puttext(curframe, eventstr, &myfont, 0, 0);

	/* Send to display */
	disp_sendswapcurframe();

	/* Wait until next event (however long) */
	wait_for_event();

	++eventcnt;	
}

```

In "ad-hoc mode", there is no timing requirement. The application draws and
sends frames as needed, e.g. when an event occurs. Many seconds can elapse
between screen updates. An example for ad-hoc mode is a menu. The display
doesn't need to be refreshed until and unless the user scrolls the selection
(or selects an item). A screen update is sent with `disp_sendswapcurframe()`.
This function will enqueue `curframe` for transfer to the display, swap the
`curframe` pointer for a new (empty) frame buffer, and immediately return. The
previous `curframe` will be transferred to the display by a separate task,
using DMA, so the application can pretty much immediately return to doing
whatever it needs to do next.


### FPS mode

```C

#define BAR_MAXWIDTH 100

int barwidth = 0;
int inc = 1;

disp_set_mode(DISP_MODE_FPS, 15);	/* 15 frames per second */

while(1) {

	/* Draw into curframe */
	disp_drawbox(curframe, 0, 0, barwidth, 10, DISP_DRAW_ON);

	barwidth += inc;
	if(barwidth == BAR_MAXWIDTH)
		inc = -1;
	else
	if(barwidth == 0)
		inc = 1;

	/* Sleep until it's time to send the next frame, then send to display */
	disp_sleep_sendswapcurframe();
}

```

In "FPS mode", the application first lets the display library know at what
rate the display should be refreshed. This will prompt the library to do some
internal time calculations to set up FPS mode. The application then is
responsible for redrawing the screen and enqueuing it for sending at the rate
it previously specified. It does so by calling `sleep_sendswapcurframe()`.
This function is identical to `sendswapcurframe()`, but first it delays
however long it has to to keep the FPS rate. In other words, as long as the
application is faster than the FPS rate, it can use `sleep_sendswapcurframe()`
to keep in sync with the FPS rate. Should the application take too much
time in drawing a frame, the next call of `sleep_sendswapcurframe()` will
drop the frame and make a note of this. (The internal variable
`disp_dropped_framecnt` can be queried for the number of times a frame
was dropped.)


## The local context

microarcade is a collection of menus and modules. Menus will call modules
(games) and modules themselves will display their own menus at times.
Whenever such a switch occurs, the callee will very likely change things like
display mode, display refresh rate (fps) and rotary configuration.

It such cases it is up to the callee to restore all settings to the way it
found them when it was called. The `local_context_t` struct and the
associated functions `save_lcontext()` and `restore_lcontext()` are provided
to make this task easy.
