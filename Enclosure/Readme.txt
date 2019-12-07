3d printing instructions:
The enclosure consists of 3 parts.
  - The mounting plate for the ESP3266 adapter (with either heat stakes or M2x4 standoffs),
  - The actual enclosure to be used with M2x12 bolts.
  - The connector insert (has a tight fit but could also be crazy-glued to the mounting plate)

I recommend to use 0.1mm or smaller layer height so that the M2 thread is reasonably well reproduced.
The ESP adapter is cheaply available at Amazone, Banggood or Ali-Express at under $4 CDN delivered.
It has the necessary level shifters and 3.3V regulator on board an can be plugged in directly to the FY6800.
I used a small 1x4 female connector that snug fits into the printed connector insert on the bottom
side only so that the top 1x4 row of pins of the FY6800 just dangle without connection. A short ribbon
cable connects 1:1 with the adapter.

So far I am pretty happy with the current functionality developed by Jakub Stępniewicz and his collaborators.
There are a few glitches when you are operating the 8266 in AP mode. As it is not possible to connect to a static
scope ID, one needs to first make the scope use its DHCP to connect to the 8266. After that you need to switch
the scope back to static IP using the so found IP address. Havent found a smarter way of dealing with this issue.
But maybe someone has?

Another problem I found and currently investigate is with the FY6800's command for setting the amplitude. It is
correctly transmitted to the AWG but not recognized there.

Other than that, this is a great project and everybody's work on it greatly appreciated.
Keep up the good work! I may contribute more in the future as well :).
