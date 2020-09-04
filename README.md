# What on Earth is a crossfeed filter?

Headphones have extreme stereo separation--the right ear doesn't get to hear
much of what's going on on the left. This leads to the impression the music's
coming from inside your head, and sounds especially weird when instruments are
panned hard to one side or the other.

Crossfeed filters aim to fix this by letting the channels mix a little, but in
a controlled way. The goal is to mimic what happens naturally when listening
to music on speakers.

# Why do we need yet another crossfeed filter?

I'd tried using existing crossfeed filters, but _hated_ the sound. Besides the
loss of high frequencies, they just sound weird to me. The theory seemed
sound, but the execution left something to be desired.

I decided to try something weird--to design a filter that could change the
stereo image without impacting overall frequency response. Monaural signals
actually pass through completely unchanged

# How can I try this?

Mac users can use the supplied command line player, which uses Apple's
built-in libraries to handle audio files and so supports a decent range of
formats.

This player is basically the crudest thing that let me test it myself--it may
contain bugs and it's definitely missing features. Even so, I've spent
countless hours listening to music through it, so it's probably good enough to
decide if you like the filter's effect!

An example:

    $ ./crossfeed-player -g -6 /.ITunes\ Music/Ingrid\ Michaelson/Be\ OK/01\ Be\ OK.m4a

This starts playing "Be OK" (on my system, where my iTunes Music folder is
in a non-standard place) with the volume reduced 6dB.

I recommend letting it reduce volume by at least a couple decibels to prevent
clipping. I think the theoretical worst increase in peaks possible is ~3.26dB.

Recursively scan and shuffle an entire directory:

	# ./crossfeed-player -s -g -8 /.Itunes\ Music

While the program's running, it responds to a few commands:
* q: Quit
* <: Previous song
* >: Next song
* c: Toggle crossfeed filter on/off, in case you're not sure what you're
  supposed to be hearing
* /: Decrease volume
* *: Increase volume (These make more sense if you have a number pad)

If you do try this, please let me know what you think of it!
