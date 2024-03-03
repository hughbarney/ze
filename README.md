# Ze Emacs

Tiny usable, experimental Editor in approx 500 lines of C.

Ze1 and Ze2 are small Emacs systle buffer gap Editors, with minimal code to explore the workings of the movement and dsiplay functions of Anthonys Editor

For further background see the README files 
for [atto](https://github.com/hughbarney/atto/blob/master/README.md)
and [femto](https://github.com/hughbarney/femto/blob/master/README.md)


## Goals of Ze Emacs

* Explore if the buffer navigation code could be made easier to read and reduced
* Provide just enough editing features to be able to make small changes to files
* Provide two versions that show the impact of managing and not managing long lines of text.  By long line we mean lines of text that exceed the column width of the console.
* Ze1 provides an editor that will work with lines that do not exceed the width of the console
* Ze2 provides an editor that correctly manages long lines
* Both versions can be run on this README file so that you can explore the behaviour when a long line is encountered.
* ze2.c has some long comments that explain the concept of line segments

## Long Line for Testing

Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam hendrerit nisi sed sollicitudin pellentesque. Nunc posuere purus rhoncus pulvinar aliquam. Ut aliquet tristique nisl vitae volutpat. Nulla aliquet porttitor venenatis. Donec a dui et dui fringilla consectetur id nec massa. Aliquam erat volutpat. Sed ut dui ut lacus dictum fermentum vel tincidunt neque. Sed sed lacinia lectus. Duis sit amet sodales felis. Duis nunc eros, mattis at dui ac, convallis semper risus. In adipiscing ultrices tellus, in suscipit massa vehicula eu.


## Starting Ze1 and Ze2

Ze1 and Ze2 can only open one file at a time.  The filename to edit must be specified on the command line.

    $ ze1 filename
    $ ze2 filename

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
    ^X^S  Save current buffer to disk, using the buffers filename

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
  Ze1 and Ze2 are released to the public domain
  hughbarney AT gmail.com 2024
