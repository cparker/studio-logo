# Studio Technology Logo

A 3D printed model of the Disney Studio Technology Logo.  Uses individually addressible WS2812B LEDs and an ESP32 microcontroller.

![logo_o1.png](./logo_small.png)


## Parts
* vellum (tracing paper) to diffuse the LEDs https://amzn.to/3JvHVWc 
* magnets : Holds the covers on https://amzn.to/3FLLyq5
* LED strips : https://bit.ly/407a7Wx 
* ESP32 microcontroller : https://bit.ly/3JvHVWc
* sticker paper : for the inside https://amzn.to/3Zb8XZ1  (or just glue regular white paper)

Parts to Print
* 1x [main frame](./cad/frame.stl)
* 1x [front cover](./cad/studio-logo-face.stl)
* 1x [stand](./cad/studio-logo-stand.stl)
* 1x [L Ear](./cad/ear-L.stl)
* 1x [R Ear](./cad/ear-r.stl)
* 1x [L Ear Cover](./cad/studio-logo-ear-cover-l.stl)
* 1x [R Ear Cover](./cad/studio-logo-ear-cover-r.stl)
* 1x [L Ear Bracket](./cad/studio-logo-ear-brack-l.stl)
* 1x [R Ear Bracket](./cad/studio-logo-ear-brack-r.stl)
* 5x [LED Brackets](./cad/led_bracket.stl)

Optionally print the backs (looks much better with the backs on)
* 1x [back cover](./cad/frame_back.stl)
* 3x [frame back mounts](./cad/main-mack-mounts.stl)
* 1x [L ear back](./cad/studio-logo-ear-back-L.stl)
* 1x [R ear back](./cad/studio-logo-ear-back-r.stl)
* 4x [ear back mounts](./cad/ear-back-mount.stl)

Cut sticker paper to fit and line the inside or glue white paper to the inside.  This will help diffuse the light.

Glue magnets into the holes for attaching the front and back covers.  For glueing the magnets I use E6000 https://amzn.to/3ndiRvU .

Cut and glue vellum to the inside of the front cover and ear covers.

Glue the LED strip brackets to the inside of the frame (see cad model photos for guidence)

Wire up the LEDs and ESP32

Mount the LED strips to the brackets (hot glue or screws)

Upload the code to the ESP32
