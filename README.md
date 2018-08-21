# mod-midi-merger

A small Jack-internal client to merge MIDI events from several inputs
to one output.

When a new port is registered at the Jack server, this client will connect to it, if

* it is a MIDI port with outgoing events
* the name does not start with `effect_`
* if it does not belong to itself


## Build
```bash
$ mkdir build && cd build
$ cmake ..
$ make
```

to install the shared library in `/usr/lib/jack/mod-midi-merger.so` run

```bash
$ make install
```

## Usage

You can start `mod-midi-merger-test` for a normal Jack client or load
`mod-midi-merger.so` as an internal client. Either way it shows up as:

```bash
$ jack_lsp --aliases
...
midi-merger:in
   MIDI in
midi-merger:out
   MIDI out
```

## Advanced

Advance build usage examples:

```bash
$ cmake -DCMAKE_BUILD_TYPE=Debug
$ make DESTDIR=/opt install
```

or

```bash
$ cmake "-DCMAKE_C_CLANG_TIDY=/usr/bin/clang-tidy;-checks=*" ..
$ make
```
