# The wolf avatar

A white wolf, drawn in place of the default avatar on boards built with
`CONFIG_MUSE_AVATAR_WOLF` (the M5Stack StopWatch's overlay turns it on). Your
own avatar in `components/muse/avatar/muse_pixel.c` still comes first.

- `source/wolf_body.gif` and `source/wolf_head.gif` are the art: five
  320 x 320 frames each (standing, eyes shut, standing, a pose, standing).
- `tools/gen_wolf_avatar.py` turns them into `wolf_sprites.c` and
  `wolf_sprites.h`: three frames of each, cut out of their background and
  scaled to the avatar's 64 x 64 grid, with one 48-colour palette. Run it
  again after changing the art; don't edit those two files by hand.
- `muse_pixel_wolf.c` animates them. While Muse boots, the head is the logo:
  it grows in with its eyes shut, opens them and now and then tilts. Then
  the wolf hops in. Idle, it breathes and blinks; listening, its glow and
  rings follow your voice; thinking, it sways under thought dots; speaking,
  its mouth opens with the reply; on an error it shakes beside a "!";
  petted, it hops among hearts; powering off, it shuts its eyes and dims.

To see every animation as a GIF:

```sh
python3 tools/muse/make_gifs.py --wolf gifs-wolf
```

and on the StopWatch's screen in the simulator, built with
`-DMUSE_SIM_AVATAR=wolf` (see `simulator/README.md`).

The wolf art belongs to the gadget's owner. The Apache License doesn't cover
it or the frames made from it (`source/`, `wolf_sprites.c`).
