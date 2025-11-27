# enDjinn

## What is enDjinn?
It is very obviously a play on the word 'engine', which enDjinn isn't quite, and a nod to the north african concept of invisible spirits, Djinn, who are the basis for the more western concept of Genies. So in short a, nearly, invisible helper that sets heaven and earth in motion for you and guides you along your quest to deliver software for the Dreamcast. 

<img style="float:right; height:220px" src="./docs/img/enDjinn.svg" alt="enDjinn logo" />

Have a look at the [hello_enDjinn](./examples/hello_enDjinn/code/hello_enDjinn.c) example to see how that can play out. Notice that the **Makefile** is a symlink to the [enDjinn/Makefile.primary](Makefile.primary) that is amended with two lines in the [Makefile.local.cfg](./examples//hello_enDjinn/Makefile.local.cfg). So two files in total and a bit of adherence to how enDjinn expects things to be setup at default and you're off to make things run on the Dreamcast.
And don't worry about being strong armed into a rigoristic and very specific way to do things, because while one part of the design philosphy is "powerfull zero config features out of the gate", another part is "as much a as possible should be reconfigurable by the user". I'll have more on how to wrange the make system to your tastes and needs later. 

The other half of enDjinn is the runtime that delivers a gameplay loop driver, a powerful sate machine as demonstrated in the [mode_orchestration example](./examples/mode_orchestration/code/mode_orchestration.c), that tries to alleviate some of quirks of the Dreamcast system while amplifying its strengths. 

I'll add more documentation to all of the above and some features I haven't mentioned it in due time. 

<!-- 

## Build System
### Automagic 
### Absolutely Configurable

## Runtime
### State Machine

### Texture Handling 

### Hardware Abstraction

#### PVR rendering 
#### Controllers 
#### File System 


## Helper Methods
-->
