---
name: playground
description: writing a very long text, and the machine switch to see what is refused
tools: repeat, shutdown
---
`repeat` writes a text (1 to 100 characters) the number of times you give (1 to 10000), separated by spaces.
The result can be very long, and a result that is too long is cut: the end of it says how much was left out.

`shutdown` pretends to switch the machine off. It is classified as dangerous, so the runtime refuses to run
it unless dangerous tools are allowed; if you get that error, tell the user and do not try to get around it.
