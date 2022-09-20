<p align="center">
    <a href="https://projectmoon.dance"><img src="https://projectmoon.dance/themes/moondance/assets/images/navbar-logo.png">
</p>

<p align=center>
<a href="https://projectmoon.dance"><img src="https://projectmoon.dance/storage/app/media/tfg-full-logo.png">
</p>

<p align=center>
    <a href="https://github.com/TeamRizu/Texture-Font-Generator-2020-Squirrel/releases"><img src="https://img.shields.io/github/downloads/TeamRizu/Texture-Font-Generator-2020-Squirrel/total?label=Total%20Downloads"/></a>
    <a href="https://github.com/TeamRizu/Texture-Font-Generator-2020-Squirrel/releases"><img src="https://img.shields.io/github/downloads/TeamRizu/Texture-Font-Generator-2020-Squirrel/latest/total?label=Latest%20Version%20Downloads"/></a>
    <a href="LICENSE"><img src="https://img.shields.io/github/license/teamrizu/Texture-Font-Generator-2020-Squirrel"/></a>
</p>

This is a modernisation of the Texture Font Generator for Project OutFox, which supports Stepmania 3.x and 5.x, with incorrect glyphs and archiac codepages removed.

The glyphs now follow a more unicode block/UTF-8 friendly method to generate.

To Compile - You can select 'Release' on the type and then compile. It's mainly limited to the old win32 standard, but these are still compatible as of July 2021

Adding new glyphs/missing glyphs. The fontpages need to be added back to the engine in 5.0/5.1 as they do not exist yet. This is a simple process of mirroring the codepages from Texture Font GeneratorDlg.cpp and adding it to the FontCharMaps.cpp within SM itself.

Ping me on the moondance server if you have any issues with backporting!

Squirrel/Scrat

## Special Thanks

- All original SM devs/contributors
- Hanubeki for keeping me sane through the initial development and for giving me the ideas
- MSDN for giving me a hand with the MFC back end 'fixing'
