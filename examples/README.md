# enDjinn examples

Currently two examples are proved. 
The first is the bare bones [hello_enDjinn](./hello_enDjinn/) 
that is a 'get a rotating sprite on screen in fewer than 100 lines'. 
It implicitly also shows of the power of the underlying enDjinn make system, that is activated simply 
by symlinking the [enDjinn/Makefile.primary](../Makefile.primary) file to a **Makefile** in your project directory.

The second one is the more involved [mode_orchestration](./mode_orchestration/) that adds interactivity while demonstrating the controller system and also tries to demonstrate the built in state handling in the enj_mod_% subsystem. 
