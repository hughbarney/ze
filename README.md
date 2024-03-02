# Ze Emacs

Tiny usable Editor in 507 lines of C.

Ze Emacs is the smallest in a family of Emacs type editors inspired by MicroEmacs, Nano, Pico and my earlier project known as Perfect Emacs [1].

* Ze is a minimal single window editor in 507 lines of C with refactored navigation code
* Zep is a single window minimal editor in less than 800 lines of C. (uses ncurses).
* Zepto is a single window minimal editor in less than 1000 lines of C that uses VT100 escape sequences instead of ncurses.
* Atto  is the smallest fuctional Emacs in less than 2000 lines of C.
* FemtoEmacs is an Atto based Emacs with the FemtoLisp extension language.

> A designer knows he has achieved perfection not when there is nothing left to add, but when there is nothing left to take away.
> -- <cite>Antoine de Saint-Exupery</cite>

## Goals of Ze Emacs

* Explore if the buffer navigation code could be made easier to read and reduced
* Provide just enough editing features to be able to make small changes to files
* Smallest code footprint to demonstrate the buffer editor concept without the distraction of more advanced editor features.
* Be easy to understand without requiring extensive study (to encourage further experimentation).

## Why the name Ze ?

The small Emacs naming scheme appears to use sub-unit prefixes in decending order with each further reduction of functionality. The Nano and Pico Emacs editors have been around for a while.

* Nano means 10 to the power of minus 9
* Pico means 10 to the power of minus 12 
* Femto means 10 to power of minus 15
* Atto means 10 to power of minus 18
* Zepto means 10 to the power of minus 21
* Zep is smaller version of Zepto Emacs 
* Ze is an even smaller version of Zep with rewritten buffer navigation logic

In Defining Atto as the lowest functional Emacs I have had to consider the essential feature set that makes Emacs, 'Emacs'.  I have defined this point as a basic Emacs command set and key bindings; the ability to edit multiple files (buffers), and switch between them; edit the buffers in mutliple windows, cut, copy and paste; forward and reverse searching, a replace function and basic syntax hilighting. The proviso being that all this will fit in less than 2000 lines of C.

Ze has the smallest possible feature set to make a viable file editor. Ze supports basic movement around the file, character insertion, deletion, backspace.  Although Ze uses a subset of the Emacs keyboard command set; it cant really be considered to be an Emacs in that it does not support the editing of multiple files in multiple windows.

## Derivation

Femto, Atto, Zepto and Zep are based on the public domain code of Anthony Howe's editor (commonly known as Anthony's Editor or AE, [2]).  Rather than representing a file as a linked list of lines, the AE Editor uses the concept of a Buffer-Gap [4,5,6].  A Buffer-Gap editor stores the file in a single piece of contiguous memory with some extra unused space known as the buffer gap.  On character insertion and deletion the gap is first moved to the current point.  A character deletion then extends the gap by moving the gap pointer back by 1 OR the gap is reduced by 1 when a character is inserted.  The Buffer-Gap technique is elegant and significantly reduces the amount of code required to load a file, modify it and redraw the display.  The proof of this is seen when you consider that Atto supports almost the same command set that Pico supports,  but Pico requires almost 17 times the amount of code.

## Comparisons with Other Emacs Implementations

    Editor         Binary   BinSize     KLOC  Files

    zepto          zepto      25962     1.052     9
    atto           atto       33002     1.9k     12
    pEmacs         pe         59465     5.7K     16
    Esatz-Emacs    ee         59050     5.7K     14
    GNOME          GNOME      55922     9.8k     13
    Zile           zile      257360    11.7k     48
    Mg             mg        585313    16.5K     50
    uEmacs/Pk      em        147546    17.5K     34
    Pico           pico      438534    24.0k     29
    Nano           nano      192008    24.8K     17
    jove           jove      248824    34.7k     94
    Qemacs         qe        379968    36.9k     59
    ue3.10         uemacs    171664    52.4K     16
    GNUEmacs       emacs   14632920   358.0k    186

## Starting Zep

Zep can only open one file at a time.  The filename to edit must be specified on the command line.

    $ zep filename

## Zep Key Bindings
    C-A   begining-of-line
    C-B   backward-character
    C-D   delete-char
    C-E   end-of-line
    C-F   forward Character
    C-H   backspace
    C-I   handle-tab
    C-J   newline
    C-L   refresh display
    C-M   Carrage Return
    C-N   next line
    C-P   previous line
    C-V   Page Down
    C-X   CTRL-X command prefix

    esc-<   Start of file
    esc->   End of file
    esc-v   Page Up

    ^X^C  Exit. Any unsaved files will require confirmation.
    ^X^S  Save current buffer to disk, using the buffer's filename as the name of

    Home  Beginning-of-line
    End   End-of-line
    Del   Delete character under cursor
    Ins   Toggle Overwrite Mode
    Left  Move left
    Right Move point right
    Up    Move to the previous line
    Down  Move to the next line
    Backspace delete caharacter on the left

## Copying
  Ze is released to the public domain.
  hughbarney AT gmail.com 2024

## References
    [1] Perfect Emacs - https://github.com/hughbarney/pEmacs
    [2] Anthony's Editor - https://github.com/hughbarney/Anthony-s-Editor
    [3] MG - https://github.com/rzalamena/mg
    [4] Jonathan Payne, Buffer-Gap: http://ned.rubyforge.org/doc/buffer-gap.txt
    [5] Anthony Howe,  http://ned.rubyforge.org/doc/editor-101.txt
    [6] Anthony Howe, http://ned.rubyforge.org/doc/editor-102.txt

