# Snap Store

## Setting up a development environment

To install necessary build dependencies:

`apt install meson ninja-build libsnapd-glib-dev libsoup2.4-dev libgtk-3-dev`

Snap Desktop Store is built using [Meson][] and [Ninja][]. To build te project:

`meson --prefix $PWD/install build/
ninja -C build/ all install`

[meson]: http://mesonbuild.com
[ninja]: https://ninja-build.org/

### Testing

See the [Testing guide](TESTING.md).

### Enabling debug output

## Evaluating pull requests

## Reaching out

We'd love the help!

- Submit pull requests against [snap-store](https://github.com/ubuntu/snap-store/pulls)
- Make sure to read the [contribution guide](CONTRIBUTING.md)
- Find us on the forum https://discourse.ubuntu.com/c/desktop/snap-store
- Discuss with us using IRC in #ubuntu-desktop on Freenode.
