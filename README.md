# Simple Table for IBus
It's easy as 1, 2, 3!

0. Install this package

0. Create a text file that looks like this:
	```
	[trigger]
	trigger = \\

	[combining-map]
	` = U+0300
	' = U+0301
	^ = U+0302
	~ = U+0303

	# bar above
	_ = U+0304

	# breve
	( = U+0306

	# dot above
	. = U+0307

	# diaeresis
	: = U+0308

	# ring above
	o = U+030A

	# double acute
	" = U+030B

	# reverse hat
	v = U+030C

	# cedilla, ogonek
	, = U+0327;U+0328

	[arbitrary]
	# right arrow
	-> = U+2192
	rarrow = U+2192

	# left arrow
	<- = U+2190
	larrow = U+2190

	# Squared
	^2 = U+00B2

	# Registered trade mark
	(r) = U+00AE
	(R) = U+00AE

	# Copyright
	(c) = U+00A9
	(C) = U+00A9

	# Smiley face
	sm = U+263A
	:) = U+263A

	# Frowny face
	:( = U+2639
	fr = U+2639

	# Heart
	<3 = U+2665

	# Checkmark
	ch = U+2713
	```

0. Use it via ```Choose Map File``` and you can type \:) to get ☺ or \o: to get ö.

In the course of your normal typing, nothing will happen until you type the trigger \. If you want to type an actual \, just hit Enter afterwards. If you think there's a character that you use even less often than \ then you can specify it as the trigger in the ```trigger``` section above.
