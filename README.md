# enDjinn

## What is enDjinn?
It is very obviously a play on the word 'engine', which enDjinn isn't quite, and an invocation of the Middle Eastern mythos of supernatural invisible beings, Djinn, or Genies as they are more commenly known as in the west.

So in short an invisible helper that sets heaven and earth in motion for you without any fuss and guides you along your quest to deliver software for the Dreamcast.

<div style="float:right; clear:none;">
<img style="float:right; height:220px" src="./docs/img/enDjinn.svg" alt="enDjinn logo" />
</div>

Have a look at the [hello_enDjinn](./examples/hello_enDjinn/code/hello_enDjinn.c) example to see how that can play out. Notice that the **Makefile** is just a symlink to the [enDjinn/Makefile.primary](Makefile.primary) that is amended with two lines in the [Makefile.local.cfg](./examples//hello_enDjinn/Makefile.local.cfg). So one symlink and two files in total and a bit of adherence to how enDjinn expects things to be arranged and you're off to make things run on the Dreamcast.

And don't worry about being strong armed into a rigoristic and very specific way to do things, because while one part of the design philosphy is "powerfull zero config features out of the gate", another part is "as much a as possible should be reconfigurable by the user". I'll have more on how to wrangle the make system to your tastes and needs later. 

The other half of enDjinn is the runtime that delivers a gameplay loop driver, a powerful sate machine as demonstrated in the [enDjinn_modes example](./examples/enDjinn_modes/code/enDjinn_modes.c), that tries to alleviate some of quirks of the Dreamcast system while amplifying its strengths. 

I'll add more documentation to all of the above and some features I haven't mentioned it in due time.

My current outline of topics to cover is as follows:


## Build System
### Automagic 
### Absolutely Configurable

### Generators
#### Textures
#### Fonts
#### Sound Effects
#### CDI Images

## Runtime
### State Machine

### Texture Handling 

### Font System

### Hardware Abstraction

#### PVR rendering
#### Controllers
#### File System 


## Helper Methods

## Profiling
### dctrace
### dcprof 