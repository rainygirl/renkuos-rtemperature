# R Temperature

[English](README.md) | [한국어](README.ko.md)

A Deskbar tray thermometer for Haiku OS. It shows the machine's temperature next
to the clock and nowhere else - no window, no entry in the Deskbar team list.
The tray item is a small BeOS-style thermometer whose mercury column rises with
the temperature, with the reading in degrees Celsius beside it.

![The R Temperature item in the Deskbar tray](docs/deskbar-tray.png)

The number is always drawn in the tray's normal menu text colour; only the
mercury column changes tint as the machine warms up.

## Build with the primary (gcc2) compiler

On an x86 hybrid Haiku, Deskbar is the x86_gcc2 binary and can only load
add-ons of the same architecture. Build this with the default `g++` (gcc2);
building it with the modern gcc makes Deskbar crash when it loads the
replicant. The source is kept gcc2-compatible.

## Build

```
make
```

Needs to be built on Haiku (uses the Be API and `rc`/`xres`/`mimeset`).

## Use

```
./rtemperature            # add it to the tray
./rtemperature --remove   # take it back out
./rtemperature --print    # print the current reading and exit
```

## Where the number comes from

The kernel publishes ACPI thermal zones at `/dev/power/acpi_thermal/N`. That
device returns a short text report, so rtemperature parses its
`Current Temperature` line rather than depending on a struct layout that has
shifted between Haiku versions. The read is one-shot and non-blocking: a second
read on the device blocks, which would freeze Deskbar's window thread.

If the tray shows `--`, this machine exposes no readable ACPI thermal zone.

## License

MIT

## AI disclosure

Parts of R Temperature were developed with the assistance of AI coding tools
(Anthropic's Claude). All code has been reviewed and tested by the author.
