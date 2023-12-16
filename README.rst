velocityper
======

`velocityper` is a command-line utility that allows data to be "stuffed" into
the buffer of a terminal, as if entered by the keyboard.

Imagine you're running a demo and recording your terminal session for
an online tutorial.  You want to talk and sound cool, but how can you
be cool when you're entering insanely precise command strings?  Worse,
when you make mistakes and have to self-correct while debugging your
problems, completely losing cool points by the bucket.  Sure, you
could just "cut-n-paste" snippets, or use a clipboard manager, a
"cut-n-paste" tool with multiple entries.

But you really just want something simpler, something that will enter
the keystrokes for you, making it look natural but without typos.  And
you want to be able to switch easily between canned keystrokes and
real interactive stuff, say when someone asks a question.  You don't
want to be completely locked into a fixed script.

Enter `velocityper`, a tool that does all this and makes your demo smooth
and maximally slick.

- `velocityper` includes parameters that introduce delays to make keystrokes
appear at a natural cadence, mimicking human typing speeds.

- `velocityper` can pause during the demo, waiting until you hit `RETURN` (in
response to a prompt in a different terminal) to tell it to proceed.

- `velocityper` can emit control characters via escape sequences in the
  text, including "\n", "\r", and "\t", as well as "\xYZ" for
  arbitrary hex values.

- `velocityper` allows content in either provided on the command line, or
  via data file.  Files are typically better for longer or more
  complex data, but command line allows `velocityper` to be incorporated
  into shell scripts that can react to current situations.

- `velocityper` allows command-line options in data files, so you can
  control delay times, invoke pauses, and other options.

Units
-----

Options that accept time values default to using milliseconds as the units,
but support the following units as suffixes::

  Short   Long Form     Nanoseconds
  ns      nanoseconds               1
  us      microseconds          1,000
  ms      milliseconds      1,000,000
  s       seconds       1,000,000,000

Leading strings can be used for brevity; for example "nano" can be used to
nanoseconds.

Options
-------

`velocityper` supports a variety of command-line option for tuning its
behavior, and these options can appear in input files.  Several of
them are better suited for such files, but are also supported on the
command line for con- sistency.  See the "FILE FORMAT" section below
for details.

The following options are supported::

     -b | --bump
             Introduce a random addition to the intra-charater wait value
             given with the --wait option, giving the text a means of
             appearing more natural and less automated.  For example, using
             "--wait 40 --bump 60" means a delay of 40ms plus a bump of
             0-60ms, for  complete range of 40-100ms.

     -C <terminal> | --confirm <terminal>
             Provides a managing terminal used for prompting for
             confirmations.

     -D | --debug
             Generate internal debug information.

     -e <delay> | --end <delay>
             Introduce a delay before each line of data.  This can be used to
             allow the viewer to see a full command string before the command
             is executed.

     -F | --force
             Ignore any confirmations requested by the --pause-confirm option.

     -f <filename> | --file <filename>
             Use the given file as content.  See the "FILE FORMAT" section
             below.

     -l <delay> | --line <delay>
             Introduce a delay after each line of data.

     -n | --dry-run
             Echo the content on the current tty, but do not place it into any
             terminal input buffer; delays operate normally.  This can be used
             to practice the demo or to evaluate timing.  It effectively turns
             velocityper into echo(1).

     -P <message> | --pause-confirm <message>
             Instructs velocityper to emit a message on the managing terminal
             and prompt for a return before continuing, allowing the
             demonstration to pause and continue as needed.  Alternatively,
             use -+ message lines in files to prompt for confirmation.

     -p <delay> | --pause <delay>
             Pause the output for the period provided.

     -S <period> | --sleep <period>
             Sleep immediately for the specified period.

     -s | --skip
             Don't make output or do delays.  Multiple invocations of this
             option toggle between skipping and not skipping.  This option can
             be used in input files to turn on and off operations, effectively
             commenting out section of data.

     -T <capname> | --tput <capname>
             Emit the terminal capability associated with the given name.
             This option is meant to mimic the tput(1) command, but currently
             only implements the clear and home capabilities.

     -t <terminal> | --tty <terminal>
             Provides a terminal to use for pushing input data, allowing one
             terminal to handling the prompting while running the
             demonstration in a second terminal.  This option requires super-
             user permissions, since it opens security concerns.

     -w <delay> | --wait <delay>
             Introduce a basic delay after each character, slowing the rate as
             required to make the text entry approximate human typing.  Use
             the --bump to add an random delay in addition to the value given
             using the --wait option, allowing the characters to appear in a
             more natural tempo.


Escape Sequences
----------------

The `velocityper` utility accepts format strings that can contain
escape sequences, which must be preceeded by a backslash.  The
following table lists the sup- porting escape sequences::

      EscSeq  Operation
      \a       Emit alarm ('^G')
      \b       Emit backspace ('^H')
      \d       Emit EOF ('^D')
      \e       Emit escape ('^[')
      \f       Emit formfeed ('^L')
      \n       Emit newline ('^N')
      \p       Pause for period from --pause option
      \P       Pause for confirmation
      \r       Emit carriage return ('^R')
      \t       Emit TAB ('^')
      \u{num}  Emit UTF-8 encoding of the code point
      \xYZ     Emit hex character (0xYZ)
      \x{hex}  Emit a series of two-digit hex values
      \^N      Emit the corresponding control character (e.g. "G" or "g")
      \^[      Emit escape ('^[')
      \^\      Emit file separator (0x1c)
      \^]      Emit group separator (0x1d)
      \^^      Emit record separator (0x1e)
      \^_      Emit unit separator (0x1f)
      \^?      Emit delete (0x7f)
      \^@      Emit NUL character (0x00)

File Format
-----------

The --file option allows content to be placed in a file, using the following
syntax::

- Lines that start with a pound character ('#') are comments.

::

    # This is a comment

- Lines that start with dash followed by a plus sign ("-+") are prompt
  confirmations, similar to the -P option.

::

    # This line will prompt before running a command:
         -+ before running command

- Lines that start with a dash ("-") are parsed as command line
  options, allowing the file to set or change option values.

::

    # These lines changes option values:
    -w 10ms -e 1s --bump 100ms
    --line 400ms

- Lines that start with a backslash character ("\") escape any
  interpretation of that line, treating it as just normal text.

::

    \# this is not seen as a comment
    \-- this is not seen as an option

- Any other lines are text that will be placed into the input buffer of
  appropriate tty.
::

    ls -l ~/bin
    df ${HOME}

- Newlines ("\n") are turned into carriage returns ("\r"), suitable for
  terminal input.

- Trailing backslashes will escape a trailing newline, and it will be
  ignored.

::

    # The first line below will not execute the command.  We'll
    # sleep for 5 seconds and then enter it (using the blank line):
    ls -l /usr/src\
    --sleep 5s

    # Above line left intentionally blank.

- The escape sequences listed above will be processed.

::

    # Turn off stuffing and waiting, set pause to one second
    # and emit a series of alternating pluses and minuses:
    --dry-run --wait=0 --pause=1s
    +\p\b-\p\b+\p\b-\p\b+\p\b-\p\b+\p\b-
    \p\b+\p\b-\p\b+\p\b-\p\b+\p\b-\p\b+\b\
    Done
    --dry-run

Examples
--------

This example places six lines of data into the input buffer::

      velocityper -b 70ms -w 30ms -l 150ms "one\rtwo\rthree\rfour\rfive\rsix\r"

This example places three lines of data into the input buffer, pausing one
second after each line::

      velocityper -w 5 -b 95 -p 1s "echo 1\r\pecho 2\r\pecho 3\r"

This example does not stuff data ('-n') but emits on stdout the characters
"cdef" on one line and the UTF-8 smiley face on a second line::

      velocityper -n 'one: \x{63646566}\ntwo: \u{263A}\n'

      
This example uses a file to perform a demo in another terminal::

      sudo velocityper --tty /dev/pts/1 --file my-demo.ks

The file would contain the full demo content::

    #
    # These lines will drive a demo of the JUNOS CLI
    # First we adjust the timers to human-ish values
    -w 20ms -b 70ms -l 200ms
    # Then we pause to wait until I'm ready to start
    # Note: since lines in the file that start with "-" are options,
    # these option values must be quoted as if they were on the
    # command like. '-P ready to start' is not acceptable, since the
    # lack of quotes makes four tokens here, not two.
    # Use "-+ ready to start" to avoid quoting issues.
    -P "ready to start demo"
    configure private
    edit protocols bgp group foo neighbor 1.2.3.4
    # Pause again to explain what's about to happen
    -+ bgp complete
    set apply-macro foo one 1
    set apply-macro foo two 2
    show
    # pause to allow more discussion
    -+ apply complete
    set apply-lock user phil
    up 1
    protect neighbor 1.2.3.4
    # These skip lines are used to comment out a section of data,
    # which can be done using comments, but if the section is
    # lengthy, then adding two "--skip" lines might be easier.
    --skip
    show
    --skip
    # At each of these pauses, I can talk as well as type on
    # the terminal, mixing canned and interactive content.
    # But I might need to restore some state (e.g. location) before
    # hitting RETURN in the other terminal.
    -+ protect complete
    show | compare
    -+ done

Another example file::

    # Use this file with the -C or -t options to use a distinct
    # tty for confirmations.
    # This line gives some timer option values
    -w 15 -b 40 -e 300
    # This line issues a simple command on the tty
    show system information
    # Prompt to make a delay so the presenter can talk
    -+ before 'show version'
    # This time we pause before the newline so the presenter
    # can talk about the command before the output appears.
    # This requires the line to end with an escape so the
    # end-of-line newline is ignored.  After the prompt, we
    have a blank line, and that newline enters this command
    show version\
    -+ before newline

    -+ after newline

Historical Notes
----------------

This command will not work under OpenBSD, due to removal of TIOCSTI:

    https://undeadly.org/cgi?action=article&sid=20170701132619

On many operating systems, the terminal input buffer has a finite size
limit, some as low as 1024 (e.g. MAX_INPUT on MacOS).  This may impact
the ability of velocityper to place data into the input buffer without
another process actively reading from the terminal.

Author
------

`velocityper` was written by Phil Shafer <phil@freebsd.org>.
