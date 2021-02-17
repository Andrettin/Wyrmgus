# Wyrmgus Debian package

Dependencies: Debian Bullseye with installed devscripts package.

To build packages either clone the repository to `wyrmgus` directory or use
`--check-dirname-level 0` flag.

```bash
cd debian
mk-build-deps -i control
debuild -uc -us
```
