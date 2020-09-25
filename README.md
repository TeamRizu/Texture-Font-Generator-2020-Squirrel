Texture-Font-Generator-2020-Squirrel

This is a modernisation of the Texture Font Generator for Stepmania 5.x, with incorrect glyphs and archiac codepages removed.

The glyphs now follow a more UTF-8 friendly method to generate.

To Compile - You will need to mess around quite heavily with MFC to get this to compile in VS2019, but it does compile correctly.

Adding new glyphs/missing glyphs. The fontpages need to be added back to the engine in 5.0/5.1 as they do not exist yet. This is a simple process of mirroring the codepages from Texture Font GeneratorDlg.cpp and adding it to the FontCharMaps.cpp within SM itself.

Ping me on the moondance server if you have any issues with backporting!

Squirrel/Scrat
