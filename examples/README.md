# enDjinn examples

Each example directory contains a `Makefile` symlink to
[`base_link.mk`](../base_link.mk), its C sources in `code/`, and any
project-specific build options in `local.cfg.mk`. With a KallistiOS
environment active, build one with:

```sh
make -C examples/enj_hello
```

The top-level `examples/Makefile` builds every example. All examples compile
against pc-enDjinn with `ENJ_TARGET=pc-endjinn`; Dreamcast remains the primary
target, and placeholder host shims mean textures, fonts, sound, rumble, and
modifier volumes do not currently render or behave equivalently on PC.

## enj_hello

A minimal Dreamcast program in under 30 lines of C. It injects and renders the
built-in qfont.

<div>
<img style="height:220px" src="../docs/img/examples/enj_hello.png" alt="enj_hello example screen" />
</div>

## enj_sprite

Loads a generated texture and draws a rotating sprite in fewer than 100 lines
of C.

<div>
<img style="height:220px" src="../docs/img/examples/enj_sprite.png" alt="enj_sprite example screen" />
</div>

## enj_controls

Demonstrates controller-state reading and interaction.

<div>
<img style="height:220px" src="../docs/img/examples/enj_controls.png" alt="enj_controls example screen" />
</div>

## enj_modes

Demonstrates the LIFO mode stack, activation callbacks, and transitions
between a main mode, an information mode, and short animation modes.

<div>
<img style="height:220px" src="../docs/img/examples/enj_modes.png" alt="enj_modes example screen" />
</div>

## enj_modifiers

Uses raw PVR modifier-volume commands alongside enDjinn’s render-list loop and
sh4zam vector types. pc-enDjinn accepts the API for source compatibility but
does not emulate modifier-volume rendering.

## enj_fonts

Demonstrates generated TrueType-based `.enjfont` assets and the font rendering
API.

<div>
<img style="height:220px" src="../docs/img/examples/enj_fonts.png" alt="enj_fonts example screen" />
</div>

## enj_rumbles

Demonstrates the rumble subsystem, including its command rate limiting.

<div>
<img style="height:220px" src="../docs/img/examples/enj_rumbles.png" alt="enj_rumbles example screen" />
</div>

## enj_sounds

Demonstrates conversion, loading, and playback of `.dca` sound-effect assets.

<div>
<img style="height:220px" src="../docs/img/examples/enj_sounds.png" alt="enj_sounds example screen" />
</div>
