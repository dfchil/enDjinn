# Console queue

[Documentation](README.md)

dc-load-ip serves one `kos-tool` client at a time. When several shells,
scripts or agents share one Dreamcast, a second upload can break the first
one's session. enDjinn's [`environ.sh`](../environ.sh) and the
[`tools/dctool/kos-tool`](../tools/dctool/kos-tool) wrapper queue clients on
a lock instead. Both are zsh scripts and need zsh's `zsh/system` module.

## Setting up

`environ.sh` is the maintainer's shell setup. Source it from zsh:

```sh
source /path/to/enDjinn/environ.sh
```

It sources `/opt/toolchains/dc/kos/environ.sh`, adds `/opt/toolchains/dc/bin`
to `PATH`, puts `tools/dctool` first on `PATH`, sets default `DCTOOL_HOST` and
`PFX_HOST` addresses, and finally sources `environ.local.sh` from the same
directory if it exists.

Machine-specific settings belong in `environ.local.sh`, which git ignores.
Copy [`environ.local.sh.example`](../environ.local.sh.example) and set:

| Variable | Meaning |
| --- | --- |
| `DCTOOL_HOST` | Address of the Dreamcast running dc-load-ip. |
| `PFX_HOST` | Address of the PixelFX HDMI mod, used for remote restarts. |
| `PFX_USER`, `PFX_PASSWD` | PixelFX login. Leave empty to be asked when a restart is needed. |

## How the queue works

With `tools/dctool` first on `PATH`, every `kos-tool` command reaches the
wrapper before the real `kos-tool`. Except for `--ready` probes, the wrapper
takes an exclusive lock on `/tmp/dctool-<host>.lock` and then replaces itself
with the real `kos-tool`, so the lock lasts exactly as long as that process.
The host is `DCTOOL_HOST`, or the host given with `-t HOST[:PORT]`.

A client that finds the console busy prints who holds it (from
`/tmp/dctool-<host>.who`) and waits up to `DCTOOL_LOCK_WAIT` seconds (default
900). If the console is still busy it exits with status 75.

The `/tmp` files are not the lock itself and need no cleaning up: the kernel
releases the lock when the process ends, however it ends.

## Holding the console for several runs

`dctool_lock` takes the same lock for the current shell, so that a sequence
of uploads, resets and captures is not interleaved with anyone else's.
`kos-tool` commands started from that shell skip the lock their shell already
holds. `dctool_unlock` releases it, as does closing the shell.

```sh
dctool_lock || return
kos-tool -t "$DCTOOL_HOST" -x bin/my-game.elf
# … more runs that must not be interleaved …
dctool_unlock
```

`dctool_clients` lists running `kos-tool` processes other than `--ready`
probes.

## Checking that the console is ready

`ensure_dctool_ready` checks that dc-load-ip answers `kos-tool --ready`,
waiting up to six seconds. If it does not, it restarts the console through the
PixelFX system-restart endpoint and waits up to 30 seconds for the loader.
It holds the console lock while it does this, and asks for `DCTOOL_HOST` and
the PixelFX details if they are not set.

A `kos-tool` client that was started without the lock is never killed by
default. `ensure_dctool_ready` waits `DCTOOL_WAIT` seconds (default 0) for it
to finish and otherwise reports the console busy with status 75.
`DCTOOL_FORCE=1` stops such clients instead.

A PixelFX restart also disturbs anything else attached to the console, such
as HDMI capture, so it is meant only for a loader that does not respond. For
an ordinary reset use the loader instead: `kos-tool -t "$DCTOOL_HOST" -r`.
