To generate the "file.fur.inc" file, we use `xxd` on Linux.

    xxd -cols 32 -include path-to-some-file.fur file.fur.inc

Then manually edit the resulting .inc file:
* Change the array type to `static const uint8_t`
* Change the length type to `UINT`

The .fur file is supposed to be created from within the Animator project at the root of this repository.
I've included a random .fur file in this directory for convenience during development.
